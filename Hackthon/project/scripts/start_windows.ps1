$ErrorActionPreference = 'Stop'
$projectPath = Split-Path -Parent $PSScriptRoot
if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
    Write-Host 'Ubuntu on WSL is required. In an Administrator PowerShell run:' -ForegroundColor Yellow
    Write-Host '  wsl --install -d Ubuntu'
    Write-Host 'Restart Windows if requested, open Ubuntu once, and create your Ubuntu username/password.'
    exit 1
}
$raw = & wsl.exe --list --quiet 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host 'WSL is not ready. Run: wsl --install -d Ubuntu' -ForegroundColor Yellow
    exit 1
}
$distributions = @($raw | ForEach-Object { ($_ -replace "`0", '').Trim() } | Where-Object { $_ -match '^Ubuntu' })
if ($distributions.Count -eq 0) {
    Write-Host 'Install Ubuntu first: wsl --install -d Ubuntu' -ForegroundColor Yellow
    Write-Host 'Then open Ubuntu once and finish its username/password setup.'
    exit 1
}
$distro = $distributions[0]
Write-Host "Starting PipeTalk using $distro..." -ForegroundColor Cyan
# --exec bypasses the WSL default shell, which otherwise interprets Windows
# backslashes as escape characters. Forward slashes are accepted by wslpath.
$projectPathForWsl = $projectPath.Replace('\', '/')
$converted = & wsl.exe -d $distro --exec wslpath -u $projectPathForWsl
if ($LASTEXITCODE -ne 0) { throw 'Could not translate the project folder to an Ubuntu path.' }
$linuxPath = ($converted | Select-Object -Last 1).Trim()
# Passing the script path as an argument handles spaces without shell interpolation.
& wsl.exe -d $distro --exec bash "$linuxPath/start.sh"
exit $LASTEXITCODE
