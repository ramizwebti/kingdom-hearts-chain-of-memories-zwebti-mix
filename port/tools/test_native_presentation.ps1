param(
    [string] $LauncherPath = (Join-Path $PSScriptRoot '../../mod/action-edition/launch.ps1'),
    [string] $TestRoot = (Join-Path ([System.IO.Path]::GetTempPath()) 'zwebti-presentation-tests')
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile(
    [System.IO.Path]::GetFullPath($LauncherPath), [ref] $tokens, [ref] $parseErrors)
if ($parseErrors.Count -gt 0) { throw 'The launcher has syntax errors.' }
$definition = $ast.Find({ param($node)
    $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -eq 'Set-NativePresentation'
}, $false)
if ($null -eq $definition) { throw 'Set-NativePresentation was not found.' }
# Load only this function: the launcher UI and player directories never run.
. ([scriptblock]::Create($definition.Extent.Text))
# Its marker uses the launcher's project caption. Read that literal constant
# from the same AST without executing the UI/setup portion of the launcher.
$caption = $ast.Find({ param($node)
    $node -is [System.Management.Automation.Language.AssignmentStatementAst] -and
    $node.Left -is [System.Management.Automation.Language.VariableExpressionAst] -and
    $node.Left.VariablePath.UserPath -eq 'projectTitle'
}, $false)
if ($null -eq $caption) { throw 'The launcher project caption was not found.' }
$projectTitle = [string] $caption.Right.SafeGetValue()
if ($projectTitle -ne 'Kingdom Hearts Chain of Memories: Zwebti Mix') {
    throw 'The presentation fixture must remain scoped to Chain of Memories.'
}

$runRoot = Join-Path ([System.IO.Path]::GetFullPath($TestRoot)) ([Guid]::NewGuid().ToString('N'))
[void][System.IO.Directory]::CreateDirectory($runRoot)
$encoding = [System.Text.Encoding]::GetEncoding(28591)
$script:checks = 0

function Assert-True([bool] $condition, [string] $message) {
    if (-not $condition) { throw $message }
    $script:checks++
}

function Assert-Bytes([string] $path, [byte[]] $expected, [string] $message) {
    Assert-True ([Convert]::ToBase64String([System.IO.File]::ReadAllBytes($path)) -ceq
        [Convert]::ToBase64String($expected)) $message
}

function New-Fixture([string] $name, [byte[]] $bytes) {
    $folder = Join-Path $runRoot $name
    [void][System.IO.Directory]::CreateDirectory($folder)
    [System.IO.File]::WriteAllBytes((Join-Path $folder 'kh_com.ini'), $bytes)
    return $folder
}

function Assert-NoTemporary([string] $folder) {
    Assert-True (@(Get-ChildItem -LiteralPath $folder -Filter 'presentation-*.tmp').Count -eq 0) `
        'A temporary presentation file was left behind.'
}

$originalText = '; caf' + [char]233 + "`r`n" +
    "  hd_text`t= 1`r`nscale_mode=2`r`nfilter=3`r`nhd_hud=1`r`n" +
    'music_volume=77' + "`r`nopaque=" + [char]128 + [char]255 + "`r`n"
$original = $encoding.GetBytes($originalText)
$folder = New-Fixture 'existing-settings' $original
$save = Join-Path $folder 'unchanged.srm'
[System.IO.File]::WriteAllBytes($save, [byte[]](3, 0, 255, 17))
Set-NativePresentation $folder
$expectedText = $originalText.Replace("hd_text`t= 1", "hd_text`t= 0").
    Replace('scale_mode=2', 'scale_mode=0').Replace('filter=3', 'filter=0').Replace('hd_hud=1', 'hd_hud=0')
Assert-Bytes (Join-Path $folder 'kh_com.ini') ($encoding.GetBytes($expectedText)) `
    'Migration changed unrelated bytes, whitespace, or CRLF newlines.'
Assert-Bytes (Join-Path $folder 'kh_com-before-v05.ini') $original 'The backup does not match the original bytes.'
Assert-Bytes $save ([byte[]](3, 0, 255, 17)) 'An unrelated save was changed.'
Assert-True ([System.IO.File]::Exists((Join-Path $folder 'zwebti-presentation-v05.txt'))) `
    'A successful migration did not write its marker.'
Assert-NoTemporary $folder

# A later player preference change must survive every subsequent launch.
$later = $encoding.GetBytes($originalText.Replace('music_volume=77', 'music_volume=42'))
[System.IO.File]::WriteAllBytes((Join-Path $folder 'kh_com.ini'), $later)
Set-NativePresentation $folder
Assert-Bytes (Join-Path $folder 'kh_com.ini') $later 'A repeated launch overwrote player preferences.'
Assert-True (@(Get-ChildItem -LiteralPath $folder -Filter 'kh_com-before-v05*.ini').Count -eq 1) `
    'A repeated launch created another backup.'

# An older backup belongs to the player; migration must choose a fresh name.
$folder = New-Fixture 'existing-backup' $original
$olderBackup = [byte[]](9, 8, 7, 255)
[System.IO.File]::WriteAllBytes((Join-Path $folder 'kh_com-before-v05.ini'), $olderBackup)
Set-NativePresentation $folder
Assert-Bytes (Join-Path $folder 'kh_com-before-v05.ini') $olderBackup 'An existing backup was overwritten.'
$newBackups = @(Get-ChildItem -LiteralPath $folder -Filter 'kh_com-before-v05-*.ini')
Assert-True ($newBackups.Count -eq 1) 'Migration did not create one uniquely named backup.'
Assert-Bytes $newBackups[0].FullName $original 'The uniquely named backup lost original bytes.'
Assert-NoTemporary $folder

# Missing keys retain LF style, the UTF-8 BOM bytes, and a missing final newline.
$sparse = [byte[]](239, 187, 191) + $encoding.GetBytes("; minimal`nvolume=63")
$folder = New-Fixture 'missing-keys' $sparse
Set-NativePresentation $folder
$expectedSparse = $sparse + $encoding.GetBytes("`nhd_text=0`nscale_mode=0`nfilter=0`nhd_hud=0`n")
Assert-Bytes (Join-Path $folder 'kh_com.ini') $expectedSparse 'Missing-key migration changed the byte format.'
Assert-Bytes (Join-Path $folder 'kh_com-before-v05.ini') $sparse 'Missing-key migration lost its original backup.'
Assert-NoTemporary $folder

# Unsupported settings encodings fail without touching the original.
$unsupported = [byte[]](255, 254, 104, 0, 100, 0)
$folder = New-Fixture 'unsupported-encoding' $unsupported
$failed = $false
try { Set-NativePresentation $folder } catch { $failed = $true }
Assert-True $failed 'An unsupported encoding was silently migrated.'
Assert-Bytes (Join-Path $folder 'kh_com.ini') $unsupported 'An unsupported encoding was modified.'
Assert-True (-not [System.IO.File]::Exists((Join-Path $folder 'zwebti-presentation-v05.txt'))) `
    'An unsuccessful migration wrote a marker.'
Assert-NoTemporary $folder

# Windows allows reading this file but blocks an atomic replacement.
$folder = New-Fixture 'locked-config' $original
$lock = [System.IO.File]::Open((Join-Path $folder 'kh_com.ini'),
    [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
$failed = $false
try {
    try { Set-NativePresentation $folder } catch { $failed = $true }
} finally { $lock.Dispose() }
Assert-True $failed 'A locked config unexpectedly allowed replacement.'
Assert-Bytes (Join-Path $folder 'kh_com.ini') $original 'A failed replacement damaged the live settings.'
Assert-True (-not [System.IO.File]::Exists((Join-Path $folder 'zwebti-presentation-v05.txt'))) `
    'A failed replacement wrote a marker.'
Assert-NoTemporary $folder

Write-Output ("PASS: {0} checks on PowerShell {1}; fixtures: {2}" -f
    $script:checks, $PSVersionTable.PSVersion, $runRoot)
