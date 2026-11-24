$ErrorActionPreference = 'SilentlyContinue'
Write-Host '--- Tetris.exe processes:'
Get-CimInstance Win32_Process -Filter "Name='Tetris.exe'" | ForEach-Object {
    Write-Host ("PID {0}  {1}" -f $_.ProcessId, $_.CommandLine)
}
Write-Host '--- listeners on 7777-7786:'
$found = $false
foreach ($port in 7777..7786) {
    $conn = Get-NetTCPConnection -LocalPort $port -State Listen
    if ($conn) {
        $found = $true
        foreach ($c in $conn) {
            Write-Host ("port {0} listening (PID {1})" -f $port, $c.OwningProcess)
        }
    }
}
if (-not $found) { Write-Host 'no listeners' }
