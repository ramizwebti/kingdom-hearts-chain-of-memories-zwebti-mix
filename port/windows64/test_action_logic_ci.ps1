$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$developmentRoot = Join-Path $repositoryRoot 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development'
$testOutput = Join-Path $developmentRoot 'native/out/ci'
$temporaryOutput = Join-Path $developmentRoot 'temp/ci'
[void][System.IO.Directory]::CreateDirectory($testOutput)
[void][System.IO.Directory]::CreateDirectory($temporaryOutput)

# Use the C compiler already installed on the Windows CI image. No game dump,
# extracted game headers, SDL SDK, download or connection token is required.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not [System.IO.File]::Exists($vswhere)) { throw 'Visual Studio discovery tool was not found.' }
$installation = @(& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if ($LASTEXITCODE -ne 0 -or $installation.Count -ne 1 -or [string]::IsNullOrWhiteSpace($installation[0])) {
    throw 'An installed Visual C++ toolchain was not found.'
}
$vcvars = Join-Path $installation[0].Trim() 'VC/Auxiliary/Build/vcvars64.bat'
if (-not [System.IO.File]::Exists($vcvars)) { throw 'Visual C++ environment setup was not found.' }
$vcvars32 = Join-Path $installation[0].Trim() 'VC/Auxiliary/Build/vcvars32.bat'
if (-not [System.IO.File]::Exists($vcvars32)) { throw 'Visual C++ x86 environment setup was not found.' }
$python = (Get-Command python -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source

function Quote-BatchPath([string] $value) {
    if ($value -match '["%\r\n]') { throw 'A compiler input path contains unsupported batch characters.' }
    return '"' + $value + '"'
}

$controllerSources = @('port/src/port_action.c', 'port/src/port_action_routes.c', 'port/src/port_action_catalog.c')
$tests = @(
    @{ Name = 'routes'; Sources = @('port/tools/test_action_routes.c') + $controllerSources; Defines = @() },
    @{ Name = 'invariants'; Sources = @('port/tools/test_action_combat.c') + $controllerSources; Defines = @() },
    @{ Name = 'pressure'; Sources = @('port/tools/test_action_pressure.c'); Defines = @() },
    @{ Name = 'feedback'; Sources = @('port/tools/test_action_feedback.c', 'port/src/port_action_feedback.c'); Defines = @() },
    @{ Name = 'fx'; Sources = @('port/tools/test_action_fx.c', 'port/src/port_action_fx.c'); Defines = @('PORT_ACTION_FX_NO_RENDER') }
)
$includes = @('port/include', 'include/card', 'include') | ForEach-Object {
    '/I' + (Quote-BatchPath (Join-Path $repositoryRoot $_))
}
$lines = @(
    '@echo off',
    'setlocal DisableDelayedExpansion',
    ('call ' + (Quote-BatchPath $vcvars) + ' >nul'),
    'if errorlevel 1 exit /b 1',
    ('pushd ' + (Quote-BatchPath $testOutput)),
    'if errorlevel 1 exit /b 1'
)
foreach ($test in $tests) {
    $arguments = @('cl', '/nologo', '/TC', '/std:c11', '/O2', '/W4', '/D_CRT_SECURE_NO_WARNINGS') + $includes
    foreach ($define in $test.Defines) { $arguments += '/D' + $define }
    foreach ($source in $test.Sources) {
        $path = Join-Path $repositoryRoot $source
        if (-not [System.IO.File]::Exists($path)) { throw ('Missing combat test source: ' + $source) }
        $arguments += Quote-BatchPath $path
    }
    $binary = Join-Path $testOutput ($test.Name + '.exe')
    $arguments += '/Fe' + (Quote-BatchPath $binary)
    $arguments += @('/link', '/INCREMENTAL:NO')
    $lines += @(
        ('echo Checking Kingdom Hearts Chain of Memories combat: ' + $test.Name),
        ($arguments -join ' '),
        'if errorlevel 1 exit /b 1',
        (Quote-BatchPath $binary),
        'if errorlevel 1 exit /b 1'
    )
}
$lines += @(
    'echo Checking production Win32 targeting, collision and companion adapters',
    ('call ' + (Quote-BatchPath $vcvars32) + ' >nul'),
    'if errorlevel 1 exit /b 1',
    ((Quote-BatchPath $python) + ' ' + (Quote-BatchPath (Join-Path $repositoryRoot 'port/windows64/test_action_adapters.py')) + ' --compiler msvc'),
    'if errorlevel 1 exit /b 1',
    'popd',
    'exit /b 0'
)
$batch = Join-Path $testOutput 'check-combat.cmd'
[System.IO.File]::WriteAllLines($batch, $lines, [System.Text.Encoding]::ASCII)
$report = Join-Path $testOutput 'combat-logic-report.json'
[System.IO.File]::WriteAllText($report, '{"passed":false,"status":"started"}', [System.Text.Encoding]::UTF8)

# Limit temporary files to this development process and its children. The
# user's global Windows environment and antivirus settings are never changed.
$start = New-Object System.Diagnostics.ProcessStartInfo
$start.FileName = $env:ComSpec
$start.Arguments = '/d /s /c ""' + $batch + '""'
$start.WorkingDirectory = $testOutput
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$start.EnvironmentVariables['TEMP'] = $temporaryOutput
$start.EnvironmentVariables['TMP'] = $temporaryOutput
$process = [System.Diagnostics.Process]::Start($start)
try {
    # Drain both streams while the compiler runs, so CI logs retain compiler
    # diagnostics and individual assertion results without filling a pipe.
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $compilerOutput = $stdout.GetAwaiter().GetResult()
    $compilerError = $stderr.GetAwaiter().GetResult()
    if ($compilerOutput) { Write-Output $compilerOutput.TrimEnd() }
    if ($compilerError) { Write-Output $compilerError.TrimEnd() }
    if ($process.ExitCode -ne 0) { throw 'The ROM-free combat checks failed to compile or run.' }
} finally { $process.Dispose() }
$result = @{
    passed = $true
    tests = @($tests | ForEach-Object { @{ name = $_.Name; passed = $true } })
    adapters = @{ passed = $true; bits = 32; report = 'adapters/action-adapters-report.json' }
}
[System.IO.File]::WriteAllText($report, ($result | ConvertTo-Json -Depth 4), [System.Text.Encoding]::UTF8)
Write-Output 'PASS: Kingdom Hearts Chain of Memories controller routes, 100000 invariants, pressure, feedback, bounded FX and production Win32 adapters.'
