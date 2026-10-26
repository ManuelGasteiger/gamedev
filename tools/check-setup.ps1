# Writes setup-report.txt in the repo root: what this machine has for the game pipeline.
# Run: powershell -ExecutionPolicy Bypass -File tools\check-setup.ps1

$root = Split-Path $PSScriptRoot -Parent
$report = [System.Collections.Generic.List[string]]::new()
function Note([string]$key, $value) { $report.Add(('{0,-26} {1}' -f $key, (@($value) -join ', '))) }
function FirstLine([string]$exe, [string[]]$arguments) {
    if (-not (Get-Command $exe -ErrorAction SilentlyContinue)) { return 'not found' }
    try { "$((& $exe @arguments 2>&1) | Select-Object -First 1)".Trim() } catch { 'error' }
}

Note 'date' (Get-Date -Format 'yyyy-MM-dd HH:mm')
Note 'git' (FirstLine git '--version')
Note 'git-lfs' (FirstLine git 'lfs', 'version')
Note 'uv' (FirstLine uv '--version')
Note 'python' (FirstLine python '--version')
Note 'node' (FirstLine node '--version')
Note 'ffmpeg' (FirstLine ffmpeg '-version')
Note 'claude' (FirstLine claude '--version')

$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$ram = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB)
Note 'cpu' "$($cpu.Name.Trim()), $($cpu.NumberOfLogicalProcessors) threads"
Note 'ram' "$ram GB"
Note 'gpu' (Get-CimInstance Win32_VideoController | ForEach-Object { "$($_.Name) (driver $($_.DriverVersion))" })
Note 'nvidia-smi' (FirstLine nvidia-smi '--query-gpu=name,memory.total,driver_version', '--format=csv,noheader')
Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Used -ne $null -and ($_.Used + $_.Free) -gt 0 } | ForEach-Object {
    Note "disk $($_.Name):" ('{0:N0} GB free of {1:N0} GB' -f ($_.Free / 1GB), (($_.Used + $_.Free) / 1GB))
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $instances = & $vswhere -all -prerelease -include packages -format json | ConvertFrom-Json
    foreach ($vs in $instances) {
        Note 'visual studio' "$($vs.displayName) $($vs.installationVersion)"
        $msvc = Join-Path $vs.installationPath 'VC\Tools\MSVC'
        Note '  msvc toolsets' $(if (Test-Path $msvc) { (Get-ChildItem $msvc -Directory).Name } else { 'none' })
        $wanted = $vs.packages | Where-Object { $_.id -match 'Workload\.NativeGame|Unreal|Workload\.NativeDesktop' } | ForEach-Object { $_.id } | Sort-Object -Unique
        Note '  game/c++ components' $(if ($wanted) { $wanted } else { 'none' })
    }
} else { Note 'visual studio' 'vswhere not found' }

$sdk = "${env:ProgramFiles(x86)}\Windows Kits\10\Include"
Note 'windows sdk' $(if (Test-Path $sdk) { (Get-ChildItem $sdk -Directory).Name } else { 'none' })

$ue = Get-ChildItem 'HKLM:\SOFTWARE\EpicGames\Unreal Engine' -ErrorAction SilentlyContinue | ForEach-Object { "$($_.PSChildName) at $((Get-ItemProperty $_.PSPath).InstalledDirectory)" }
Note 'unreal (registry)' $(if ($ue) { $ue } else { 'none' })
$launcherDat = 'C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat'
if (Test-Path $launcherDat) {
    $apps = (Get-Content $launcherDat -Raw | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -like 'UE_*' } | ForEach-Object { "$($_.AppName) at $($_.InstallLocation)" }
    Note 'unreal (launcher)' $(if ($apps) { $apps } else { 'none' })
} else { Note 'unreal (launcher)' 'launcher data not found' }

$uninstall = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
             'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*',
             'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*'
$apps = Get-ItemProperty $uninstall -ErrorAction SilentlyContinue |
    Where-Object { $_.DisplayName -match 'Epic Games Launcher|Unreal|Blender|EmberGen|JangaFX|Wwise|Audiokinetic|Houdini|Substance|Zibra|Rider' } |
    ForEach-Object { "$($_.DisplayName) $($_.DisplayVersion)".Trim() } | Sort-Object -Unique
Note 'installed apps' $(if ($apps) { $apps } else { 'none matched' })

$blender = Get-ChildItem 'D:\Programme\Blender\blender.exe', "$env:ProgramFiles\Blender Foundation\*\blender.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
Note 'blender' $(if ($blender) { FirstLine $blender.FullName '--version' } else { 'blender.exe not found' })

$out = Join-Path $root 'setup-report.txt'
$report | Set-Content -Path $out -Encoding utf8
$report
Write-Host "`nWritten to $out"
