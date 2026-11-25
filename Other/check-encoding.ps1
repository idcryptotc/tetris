# Check files for UTF-8 without BOM - AGENTS.md rule 6: sources and README
# must be UTF-8 without BOM (BOM / non-UTF-8 breaks the build / Cyrillic).
# Usage: .\check-encoding.ps1 <files...>   (no args: all .ps1 next to this script)
# Exit code: 0 - all files OK, 1 - at least one BAD.
# The script itself is ASCII on purpose: Windows PowerShell 5.1 reads BOM-less
# .ps1 files as ANSI, so a Cyrillic script would corrupt itself.
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Files
)
if ($null -eq $Files -or $Files.Count -eq 0) {
    $Files = @(Get-ChildItem (Split-Path -Parent $MyInvocation.MyCommand.Path) -Filter *.ps1 | ForEach-Object { $_.FullName })
}
$validator = [System.Text.Encoding]::GetEncoding('utf-8', [System.Text.EncoderFallback]::ExceptionFallback, [System.Text.DecoderFallback]::ExceptionFallback)
$bad = 0
foreach ($file in $Files) {
    if (-not (Test-Path -LiteralPath $file)) {
        Write-Host "BAD: no such file: $file"
        $bad++
        continue
    }
    $bytes = [System.IO.File]::ReadAllBytes($file)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        Write-Host "BAD: has BOM: $file"
        $bad++
        continue
    }
    try {
        $validator.GetString($bytes) | Out-Null
        Write-Host "OK: UTF-8 without BOM: $file"
    }
    catch {
        Write-Host "BAD: not UTF-8: $file"
        $bad++
    }
}
if ($bad -gt 0) {
    Write-Host "Summary: $bad file(s) failed"
    exit 1
}
Write-Host "Summary: all files are UTF-8 without BOM"