param(
    [ValidateSet('', 'training', 'crowd', 'mixed', 'axel', 'jafar')]
    [string] $Practice = '',
    [switch] $Story,
    [string] $ProfileRoot = ''
)
# Windows PowerShell 5.1. No build tools, downloads or administrator access.
# Profiles are separate from versioned application files so updates keep saves.
Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$packageRoot = [System.IO.Path]::GetFullPath($PSScriptRoot)
$runtimeRoot = Join-Path $packageRoot 'runtime'
if ([string]::IsNullOrWhiteSpace($ProfileRoot)) { $ProfileRoot = $packageRoot }
$ProfileRoot = [System.IO.Path]::GetFullPath($ProfileRoot)
$dataRoot = Join-Path $ProfileRoot 'data'
$romDestination = Join-Path $dataRoot 'B8CE.gba'
$expectedSha1 = '10729bd884f8fdca7a310b6d606c52e46657aa48'
$romSize = 33554432L
$archiveLimit = 67108864L
$projectTitle = 'Kingdom Hearts Chain of Memories: Zwebti Mix'

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.Windows.Forms.Application]::EnableVisualStyles()
# The native launcher uses this ephemeral event only to distinguish a script
# startup failure from an error already explained by a Windows dialog.
if (-not [string]::IsNullOrEmpty($env:ZWEBTI_LAUNCHER_READY_EVENT)) {
    $readyEvent = $null
    try {
        $readyEvent = [System.Threading.EventWaitHandle]::OpenExisting($env:ZWEBTI_LAUNCHER_READY_EVENT)
        [void]$readyEvent.Set()
    } catch { }
    finally { if ($null -ne $readyEvent) { $readyEvent.Dispose() } }
}
$script:launchCancelled = $false
$script:startupForm = $null
$script:startupStatus = $null
$script:startupImage = $null
$script:startupIcon = $null

function Show-StartupStatus {
    $form = New-Object System.Windows.Forms.Form
    $form.Text = $projectTitle
    $form.ClientSize = New-Object System.Drawing.Size(540, 365)
    $form.StartPosition = [System.Windows.Forms.FormStartPosition]::CenterScreen
    $form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
    $form.MaximizeBox = $false
    $form.MinimizeBox = $false
    $form.ControlBox = $false
    $form.BackColor = [System.Drawing.Color]::FromArgb(248, 246, 239)
    $title = New-Object System.Windows.Forms.Label
    $title.Text = $projectTitle
    $title.Font = New-Object System.Drawing.Font('Segoe UI', 16, [System.Drawing.FontStyle]::Bold)
    $title.Location = New-Object System.Drawing.Point(22, 17)
    $title.Size = New-Object System.Drawing.Size(492, 90)
    $form.Controls.Add($title)
    $status = New-Object System.Windows.Forms.Label
    $status.Text = 'Preparing your game...'
    $status.Location = New-Object System.Drawing.Point(24, 250)
    $status.Size = New-Object System.Drawing.Size(492, 40)
    $form.Controls.Add($status)
    $progress = New-Object System.Windows.Forms.ProgressBar
    $progress.Style = [System.Windows.Forms.ProgressBarStyle]::Marquee
    $progress.MarqueeAnimationSpeed = 25
    $progress.Location = New-Object System.Drawing.Point(24, 315)
    $progress.Size = New-Object System.Drawing.Size(400, 14)
    $form.Controls.Add($progress)
    $cancel = New-Object System.Windows.Forms.Button
    $cancel.Text = 'Cancel'
    $cancel.Location = New-Object System.Drawing.Point(438, 308)
    $cancel.Size = New-Object System.Drawing.Size(78, 29)
    $cancel.Add_Click({ $script:launchCancelled = $true })
    $form.Controls.Add($cancel)
    $form.CancelButton = $cancel
    $iconPath = Join-Path $packageRoot 'branding\kingdom-hearts-chain-of-memories-zwebti-mix.ico'
    if ([System.IO.File]::Exists($iconPath)) {
        try {
            $script:startupIcon = New-Object System.Drawing.Icon($iconPath)
            $form.Icon = $script:startupIcon
        } catch { }
    }
    $logoPath = Join-Path $packageRoot 'branding\kingdom-hearts-chain-of-memories-zwebti-mix-logo.png'
    if ([System.IO.File]::Exists($logoPath)) {
        try {
            $script:startupImage = [System.Drawing.Image]::FromFile($logoPath)
            $logo = New-Object System.Windows.Forms.PictureBox
            $logo.Image = $script:startupImage
            $logo.Location = New-Object System.Drawing.Point(22, 12)
            $logo.Size = New-Object System.Drawing.Size(496, 220)
            $logo.SizeMode = [System.Windows.Forms.PictureBoxSizeMode]::Zoom
            $form.Controls.Add($logo)
            $title.Visible = $false
        } catch { }
    }
    $script:startupForm = $form
    $script:startupStatus = $status
    $form.Show()
    [System.Windows.Forms.Application]::DoEvents()
}

function Update-StartupStatus([string] $message) {
    if ($null -ne $script:startupStatus) { $script:startupStatus.Text = $message }
    [System.Windows.Forms.Application]::DoEvents()
}

function Close-StartupStatus {
    if ($null -ne $script:startupForm) {
        $script:startupForm.Close()
        $script:startupForm.Dispose()
        $script:startupForm = $null
        $script:startupStatus = $null
    }
    if ($null -ne $script:startupImage) {
        $script:startupImage.Dispose()
        $script:startupImage = $null
    }
    if ($null -ne $script:startupIcon) {
        $script:startupIcon.Dispose()
        $script:startupIcon = $null
    }
}

function Show-LaunchMessage([string] $message, [bool] $isError = $false) {
    $icon = [System.Windows.Forms.MessageBoxIcon]::Information
    if ($isError) { $icon = [System.Windows.Forms.MessageBoxIcon]::Error }
    [void][System.Windows.Forms.MessageBox]::Show(
        $message, $projectTitle,
        [System.Windows.Forms.MessageBoxButtons]::OK, $icon)
}

function Choose-PlayMode {
    $form = New-Object System.Windows.Forms.Form
    $form.Text = $projectTitle
    $form.ClientSize = New-Object System.Drawing.Size(600, 570)
    $form.StartPosition = [System.Windows.Forms.FormStartPosition]::CenterScreen
    $form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
    $form.MaximizeBox = $false
    $form.MinimizeBox = $false
    $form.BackColor = [System.Drawing.Color]::FromArgb(248, 246, 239)
    $image = $null
    $icon = $null
    try {
        $logoPath = Join-Path $packageRoot 'branding\kingdom-hearts-chain-of-memories-zwebti-mix-logo.png'
        if ([System.IO.File]::Exists($logoPath)) {
            $image = [System.Drawing.Image]::FromFile($logoPath)
            $logo = New-Object System.Windows.Forms.PictureBox
            $logo.Image = $image
            $logo.Location = New-Object System.Drawing.Point(24, 12)
            $logo.Size = New-Object System.Drawing.Size(552, 270)
            $logo.SizeMode = [System.Windows.Forms.PictureBoxSizeMode]::Zoom
            $form.Controls.Add($logo)
        } else {
            $title = New-Object System.Windows.Forms.Label
            $title.Text = $projectTitle
            $title.Font = New-Object System.Drawing.Font('Segoe UI', 18, [System.Drawing.FontStyle]::Bold)
            $title.Location = New-Object System.Drawing.Point(24, 25)
            $title.Size = New-Object System.Drawing.Size(552, 90)
            $form.Controls.Add($title)
        }
        $iconPath = Join-Path $packageRoot 'branding\kingdom-hearts-chain-of-memories-zwebti-mix.ico'
        if ([System.IO.File]::Exists($iconPath)) {
            $icon = New-Object System.Drawing.Icon($iconPath)
            $form.Icon = $icon
        }
        $heading = New-Object System.Windows.Forms.Label
        $heading.Text = 'How would you like to play?'
        $heading.Font = New-Object System.Drawing.Font('Segoe UI', 11, [System.Drawing.FontStyle]::Bold)
        $heading.Location = New-Object System.Drawing.Point(26, 300)
        $heading.Size = New-Object System.Drawing.Size(548, 27)
        $form.Controls.Add($heading)
        $storyOption = New-Object System.Windows.Forms.RadioButton
        $storyOption.Text = 'Story - start or continue your adventure'
        $storyOption.Checked = $true
        $storyOption.Location = New-Object System.Drawing.Point(27, 342)
        $storyOption.Size = New-Object System.Drawing.Size(546, 25)
        $form.Controls.Add($storyOption)
        $practiceOption = New-Object System.Windows.Forms.RadioButton
        $practiceOption.Text = 'Practice Arena - jump straight into combat'
        $practiceOption.Location = New-Object System.Drawing.Point(27, 380)
        $practiceOption.Size = New-Object System.Drawing.Size(546, 25)
        $form.Controls.Add($practiceOption)
        $presets = New-Object System.Windows.Forms.ComboBox
        $presets.DropDownStyle = [System.Windows.Forms.ComboBoxStyle]::DropDownList
        $presets.Location = New-Object System.Drawing.Point(47, 417)
        $presets.Size = New-Object System.Drawing.Size(524, 25)
        [void]$presets.Items.AddRange(@('Training - combos and magic', 'Crowd - Shadow waves',
            'Mixed - Soldiers and Shadows', 'Boss - Axel', 'Boss - Jafar'))
        $presets.SelectedIndex = 0
        $presets.Enabled = $false
        $practiceOption.Add_CheckedChanged({ $presets.Enabled = $practiceOption.Checked })
        $form.Controls.Add($presets)
        $note = New-Object System.Windows.Forms.Label
        $note.Text = 'Practice uses separate data. Your Story saves stay in their own folder.'
        $note.Location = New-Object System.Drawing.Point(27, 455)
        $note.Size = New-Object System.Drawing.Size(545, 31)
        $form.Controls.Add($note)
        $start = New-Object System.Windows.Forms.Button
        $start.Text = 'Start game'
        $start.Location = New-Object System.Drawing.Point(375, 508)
        $start.Size = New-Object System.Drawing.Size(111, 32)
        $start.DialogResult = [System.Windows.Forms.DialogResult]::OK
        $form.Controls.Add($start)
        $cancel = New-Object System.Windows.Forms.Button
        $cancel.Text = 'Cancel'
        $cancel.Location = New-Object System.Drawing.Point(497, 508)
        $cancel.Size = New-Object System.Drawing.Size(76, 32)
        $cancel.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
        $form.Controls.Add($cancel)
        $form.AcceptButton = $start
        $form.CancelButton = $cancel
        if ($form.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { return $null }
        $presetNames = @('training', 'crowd', 'mixed', 'axel', 'jafar')
        return [pscustomobject]@{ Practice = $(if ($practiceOption.Checked) {
            $presetNames[$presets.SelectedIndex]
        } else { '' }) }
    }
    finally {
        $form.Dispose()
        if ($null -ne $image) { $image.Dispose() }
        if ($null -ne $icon) { $icon.Dispose() }
    }
}

function Prepare-PracticeData([string] $romSource) {
    $practiceRoot = Join-Path $ProfileRoot 'practice-data'
    [void][System.IO.Directory]::CreateDirectory($practiceRoot)
    $practiceRom = Join-Path $practiceRoot 'B8CE.gba'
    if (-not (Test-RomFile $practiceRom)) {
        $temporary = Join-Path $practiceRoot ('rom-import-' + [Guid]::NewGuid().ToString('N') + '.tmp')
        try {
            [System.IO.File]::Copy($romSource, $temporary, $false)
            if (-not (Test-RomFile $temporary)) { throw 'The Practice Arena game file could not be copied correctly.' }
            if ([System.IO.File]::Exists($practiceRom)) {
                $preserved = Join-Path $practiceRoot ('B8CE-preserved-' + [Guid]::NewGuid().ToString('N') + '.gba')
                [System.IO.File]::Move($practiceRom, $preserved)
            }
            [System.IO.File]::Move($temporary, $practiceRom)
        }
        finally {
            if ([System.IO.File]::Exists($temporary)) { [System.IO.File]::Delete($temporary) }
        }
    }
    $practiceConfig = Join-Path $practiceRoot 'kh_com.ini'
    if (-not [System.IO.File]::Exists($practiceConfig)) {
        [System.IO.File]::Copy((Join-Path $packageRoot 'kh_com.ini'), $practiceConfig, $false)
    }
    Set-NativePresentation $practiceRoot
    Initialize-ZwebtiSession $practiceRoot
    return [pscustomobject]@{ Root = $practiceRoot; Rom = $practiceRom }
}

function Initialize-ZwebtiSession([string] $folder) {
    # Action rules already select the separate local modified-rules list.
    # Its scope is explained in README; acknowledge the upstream first-start
    # notice so a smooth "Cheats" toast does not cover the cartridge artwork.
    # This marker does not enable official achievement evaluation/submission
    # or change any earned progress, and is confined to this package's data.
    $notice = Join-Path $folder 'rebel_mode.txt'
    if (-not [System.IO.File]::Exists($notice)) {
        [System.IO.File]::WriteAllText($notice, 'unlocked' + "`r`n",
            (New-Object System.Text.UTF8Encoding($false)))
    }
}

function Set-NativePresentation([string] $folder) {
    # Apply this version's pixel defaults once, then preserve the player's
    # later graphics choices. Keep every unrelated byte of the existing INI.
    $marker = Join-Path $folder 'zwebti-presentation-v05.txt'
    if ([System.IO.File]::Exists($marker)) { return }
    $path = Join-Path $folder 'kh_com.ini'
    $original = [System.IO.File]::ReadAllBytes($path)
    # The game uses an ASCII-keyed byte INI. Latin-1 is a reversible byte map,
    # preserving comments, newline style and any unrelated non-ASCII bytes.
    $encoding = [System.Text.Encoding]::GetEncoding(28591)
    $text = $encoding.GetString($original)
    if ($text.IndexOf([char]0) -ge 0) {
        throw 'The settings file uses an unsupported encoding. Your original file has not been changed.'
    }
    $newline = "`n"
    if ($text.Contains("`r`n")) { $newline = "`r`n" }
    foreach ($key in @('hd_text', 'scale_mode', 'filter', 'hd_hud')) {
        $pattern = '(?m)^([ \t]*' + $key + '[ \t]*=[ \t]*)[^\r\n]*'
        if ([System.Text.RegularExpressions.Regex]::IsMatch($text, $pattern)) {
            $text = [System.Text.RegularExpressions.Regex]::Replace($text, $pattern, '${1}0')
        } else {
            if ($text.Length -gt 0 -and -not $text.EndsWith("`n")) { $text += $newline }
            $text += $key + '=0' + $newline
        }
    }
    $backup = Join-Path $folder 'kh_com-before-v05.ini'
    if ([System.IO.File]::Exists($backup)) {
        $backup = Join-Path $folder ('kh_com-before-v05-' + [Guid]::NewGuid().ToString('N') + '.ini')
    }
    $temporary = Join-Path $folder ('presentation-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        # Replace atomically preserves the old config at the unique backup path.
        # Pass a real backup name: Windows PowerShell 5.1 converts a null string
        # argument to an empty path, which makes File.Replace fail on Windows.
        [System.IO.File]::WriteAllBytes($temporary, $encoding.GetBytes($text))
        [System.IO.File]::Replace($temporary, $path, $backup)
        [System.IO.File]::WriteAllText($marker,
            $projectTitle + ' native pixel defaults applied. Later graphics changes are preserved.' + "`r`n",
            (New-Object System.Text.UTF8Encoding($false)))
    }
    finally {
        if ([System.IO.File]::Exists($temporary)) { [System.IO.File]::Delete($temporary) }
    }
}

function Test-RomFile([string] $path) {
    if (-not [System.IO.File]::Exists($path)) { return $false }
    $stream = $null
    $sha1 = $null
    try {
        $stream = [System.IO.File]::OpenRead($path)
        if ($stream.Length -ne $romSize) { return $false }
        $sha1 = [System.Security.Cryptography.SHA1]::Create()
        $hash = [System.BitConverter]::ToString($sha1.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
        return $hash -eq $expectedSha1
    }
    catch { return $false }
    finally {
        if ($null -ne $stream) { $stream.Dispose() }
        if ($null -ne $sha1) { $sha1.Dispose() }
    }
}

function Read-ValidatedRom([System.IO.Stream] $stream) {
    # Bound actual decompressed bytes, rather than trusting ZIP metadata.
    $memory = New-Object System.IO.MemoryStream
    $sha1 = $null
    try {
        $buffer = New-Object byte[] 65536
        while (($count = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
            if ($memory.Length + $count -gt $romSize) { return $null }
            $memory.Write($buffer, 0, $count)
        }
        if ($memory.Length -ne $romSize) { return $null }
        $memory.Position = 0
        $sha1 = [System.Security.Cryptography.SHA1]::Create()
        $hash = [System.BitConverter]::ToString($sha1.ComputeHash($memory)).Replace('-', '').ToLowerInvariant()
        if ($hash -ne $expectedSha1) { return $null }
        # A unary comma prevents PowerShell from enumerating 32 million bytes.
        return ,$memory.ToArray()
    }
    finally {
        $memory.Dispose()
        if ($null -ne $sha1) { $sha1.Dispose() }
    }
}

function Read-RomCandidate([string] $path) {
    $stream = $null
    $archive = $null
    try {
        $file = Get-Item -LiteralPath $path -ErrorAction Stop
        $extension = $file.Extension.ToLowerInvariant()
        if ($extension -eq '.gba') {
            if ($file.Length -ne $romSize) { return $null }
            $stream = [System.IO.File]::OpenRead($file.FullName)
            $bytes = Read-ValidatedRom $stream
            if ($null -ne $bytes) { return ,$bytes }
            return $null
        }
        if ($extension -ne '.zip' -or $file.Length -gt $archiveLimit) { return $null }
        $archive = [System.IO.Compression.ZipFile]::OpenRead($file.FullName)
        if ($archive.Entries.Count -gt 256) { return $null }
        foreach ($entry in $archive.Entries) {
            if ([System.IO.Path]::GetExtension($entry.Name).ToLowerInvariant() -ne '.gba' -or
                $entry.Length -ne $romSize) { continue }
            $entryStream = $null
            try {
                $entryStream = $entry.Open()
                $bytes = Read-ValidatedRom $entryStream
                if ($null -ne $bytes) { return ,$bytes }
            }
            finally {
                if ($null -ne $entryStream) { $entryStream.Dispose() }
            }
        }
        return $null
    }
    catch { return $null } # unreadable or malformed candidates do not interrupt the search
    finally {
        if ($null -ne $stream) { $stream.Dispose() }
        if ($null -ne $archive) { $archive.Dispose() }
    }
}

function Save-Rom([byte[]] $bytes) {
    $temporary = Join-Path $dataRoot ('rom-import-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        [System.IO.File]::WriteAllBytes($temporary, $bytes)
        if (-not (Test-RomFile $temporary)) { throw 'The game dump could not be copied correctly.' }
        if ([System.IO.File]::Exists($romDestination)) {
            # Preserve even an invalid existing package copy; never overwrite a save or an installation.
            $backup = Join-Path $dataRoot ('B8CE-preserved-' + [Guid]::NewGuid().ToString('N') + '.gba')
            [System.IO.File]::Move($romDestination, $backup)
        }
        [System.IO.File]::Move($temporary, $romDestination)
        return $romDestination
    }
    finally {
        if ([System.IO.File]::Exists($temporary)) { [System.IO.File]::Delete($temporary) }
    }
}

function Find-ExistingRom {
    $queue = New-Object 'System.Collections.Generic.Queue[object]'
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $roots = @(
        (Join-Path $packageRoot 'roms'),
        $packageRoot,
        (Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Downloads'),
        ([Environment]::GetFolderPath('Desktop')),
        ([Environment]::GetFolderPath('MyDocuments'))
    )
    foreach ($root in $roots) {
        if ($root -and [System.IO.Directory]::Exists($root) -and $seen.Add($root)) {
            $queue.Enqueue([pscustomobject]@{ Path = $root; Depth = 0 })
        }
    }
    $candidates = 0
    $directories = 0
    while ($queue.Count -gt 0 -and $candidates -lt 200 -and $directories -lt 800) {
        Update-StartupStatus 'Looking for your USA game file. You can select it if nothing is found.'
        if ($script:launchCancelled) { return $null }
        $current = $queue.Dequeue()
        $directories++
        # Shallow breadth-first search only; never traverse drives, reparse
        # points, hidden application folders, or every file in a large tree.
        $items = @(Get-ChildItem -LiteralPath $current.Path -ErrorAction SilentlyContinue |
            Select-Object -First 512)
        foreach ($item in $items) {
            if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) { continue }
            if ($item.PSIsContainer) {
                if ($current.Depth -lt 3 -and $item.FullName -ne $dataRoot -and
                    $item.FullName -ne $runtimeRoot -and $seen.Add($item.FullName)) {
                    $queue.Enqueue([pscustomobject]@{ Path = $item.FullName; Depth = $current.Depth + 1 })
                }
                continue
            }
            if ($item.Extension.ToLowerInvariant() -notin @('.gba', '.zip')) { continue }
            [System.Windows.Forms.Application]::DoEvents()
            if ($script:launchCancelled) { return $null }
            $candidates++
            $bytes = Read-RomCandidate $item.FullName
            if ($null -ne $bytes) { return Save-Rom $bytes }
            if ($candidates -ge 200) { break }
        }
    }
    return $null
}

function Choose-Rom {
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Title = $projectTitle + ' - select your USA game file'
    $dialog.Filter = 'GBA game dump or ZIP (*.gba;*.zip)|*.gba;*.zip|All files (*.*)|*.*'
    $downloads = Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Downloads'
    if ([System.IO.Directory]::Exists($downloads)) { $dialog.InitialDirectory = $downloads }
    $dialog.CheckFileExists = $true
    $dialog.Multiselect = $false
    try {
        while ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
            $bytes = Read-RomCandidate $dialog.FileName
            if ($null -ne $bytes) { return Save-Rom $bytes }
            Show-LaunchMessage ('That file does not contain the supported, unmodified USA game dump. ' +
                'Please select your USA .gba file or a ZIP containing it.')
        }
        return $null
    }
    finally { $dialog.Dispose() }
}

function Quote-NativeArgument([string] $value) {
    # Windows CommandLineToArgvW rules, including paths with spaces and
    # trailing backslashes. No command shell interprets these arguments.
    $quoted = [System.Text.RegularExpressions.Regex]::Replace($value, '(\\*)"', '$1$1\"')
    $quoted = [System.Text.RegularExpressions.Regex]::Replace($quoted, '(\\+)$', '$1$1')
    return '"' + $quoted + '"'
}

try {
    if (-not [Environment]::Is64BitOperatingSystem) {
        throw ($projectTitle + ' needs a 64-bit Windows PC. This package supports Windows 10 and Windows 11.')
    }
    $executable = Join-Path $runtimeRoot 'rekhcom64.exe'
    $guest = Join-Path $runtimeRoot 'guest-us.bin'
    foreach ($required in @($executable, $guest, (Join-Path $runtimeRoot 'SDL3.dll'))) {
        if (-not [System.IO.File]::Exists($required)) {
            throw ('The package is incomplete. Extract the entire ' + $projectTitle + ' ZIP and run Kingdom Hearts Chain of Memories - Zwebti Mix.exe from the extracted folder. Keep the runtime folder beside it.')
        }
    }
    if ($Story -and $Practice) { throw 'Choose either Story or a Practice Arena preset.' }
    if (-not $Story -and -not $Practice) {
        $mode = Choose-PlayMode
        if ($null -eq $mode) { exit 0 }
        $Practice = $mode.Practice
    }
    [void][System.IO.Directory]::CreateDirectory($dataRoot)
    $config = Join-Path $dataRoot 'kh_com.ini'
    if (-not [System.IO.File]::Exists($config)) {
        [System.IO.File]::Copy((Join-Path $packageRoot 'kh_com.ini'), $config, $false)
    }
    Set-NativePresentation $dataRoot
    Initialize-ZwebtiSession $dataRoot

    $rom = $null
    Show-StartupStatus
    if (Test-RomFile $romDestination) { $rom = $romDestination }
    if ($null -eq $rom) {
        Write-Host 'Looking for your Chain of Memories USA game file...'
        Write-Host 'First launch checks this folder, Downloads, Desktop and Documents.'
        Write-Host 'If it cannot find the game, a file selection window will open.'
        $rom = Find-ExistingRom
    }
    [System.Windows.Forms.Application]::DoEvents()
    Close-StartupStatus
    if ($script:launchCancelled) { exit 0 }
    if ($null -eq $rom) {
        Write-Host 'Select your unmodified USA .gba file, or the ZIP containing it.'
        $rom = Choose-Rom
    }
    if ($null -eq $rom) { exit 0 }

    $gameDataRoot = $dataRoot
    $gameRom = $rom
    if ($Practice) {
        $practiceData = Prepare-PracticeData $rom
        $gameDataRoot = $practiceData.Root
        $gameRom = $practiceData.Rom
    }

    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = $executable
    $start.WorkingDirectory = $runtimeRoot
    $start.UseShellExecute = $false
    # A shipping launch must not inherit development battles, cheats or timing
    # fixtures from another process. Both play modes start with clean flags.
    foreach ($key in @($start.EnvironmentVariables.Keys)) {
        if ($key -like 'KHCOM_*') {
            $start.EnvironmentVariables.Remove($key)
        }
    }
    $start.EnvironmentVariables['KHCOM_ASSETS'] = Join-Path $runtimeRoot 'assets'
    if ($Practice) { $start.EnvironmentVariables['KHCOM_PRACTICE'] = $Practice }
    $start.Arguments = (Quote-NativeArgument $guest) + ' --data ' +
        (Quote-NativeArgument $gameDataRoot) + ' --rom ' + (Quote-NativeArgument $gameRom)
    Write-Host ('Starting ' + $projectTitle + '. Use the game window to play.')
    $process = [System.Diagnostics.Process]::Start($start)
    $process.WaitForExit()
    $code = $process.ExitCode
    $process.Dispose()
    if ($code -ne 0) {
        Show-LaunchMessage ($projectTitle + ' stopped before closing normally.' + "`r`n`r`n" +
            'Diagnostic log: ' + (Join-Path $gameDataRoot 'kh_com.log') + "`r`n`r`n" +
            'Open START_HERE.html for launch help. If you need help, send the exact message or this log.') $true
    }
    exit $code
}
catch {
    Close-StartupStatus
    Show-LaunchMessage ($_.Exception.Message + "`r`n`r`n" +
        'Extract the package into a folder you can write to, such as Downloads. Open START_HERE.html for launch help.') $true
    exit 1
}
