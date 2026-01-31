# Read the debug log file
$logFile = "$env:TEMP\LiveSpice_AB_Tester.log"

if (Test-Path $logFile) {
    Write-Host "=== LiveSpice A/B Tester Debug Log ===" -ForegroundColor Cyan
    Write-Host ""
    
    $content = Get-Content $logFile
    
    # Show last 100 lines
    if ($content -is [array]) {
        $content[-100..-1] | ForEach-Object { Write-Host $_ }
    } else {
        Write-Host $content
    }
    
    Write-Host ""
    Write-Host "Log file location: $logFile" -ForegroundColor Green
} else {
    Write-Host "Log file not found at: $logFile" -ForegroundColor Red
    Write-Host "Make sure the A/B Tester has loaded at least one plugin." -ForegroundColor Yellow
}
