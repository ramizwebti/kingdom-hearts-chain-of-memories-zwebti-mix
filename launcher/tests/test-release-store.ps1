$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$testOutput = Join-Path $repositoryRoot 'build/launcher-tests'
[void][System.IO.Directory]::CreateDirectory($testOutput)
$compiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$binary = Join-Path $testOutput 'ReleaseStoreTests.exe'
& $compiler /nologo /langversion:5 /target:exe ('/out:' + $binary) `
    /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll `
    (Join-Path $repositoryRoot 'launcher/ReleaseStore.cs') (Join-Path $PSScriptRoot 'ReleaseStoreTests.cs')
if ($LASTEXITCODE -ne 0) { throw 'The updater tests did not compile.' }
& $binary
if ($LASTEXITCODE -ne 0) { throw 'The updater tests failed.' }
