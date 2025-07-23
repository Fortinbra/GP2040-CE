# Script to generate the board list for GitHub Actions workflow_dispatch options
# Run this script and copy the output to update .github/workflows/cmake.yml

Write-Host "Current board configurations in configs/ directory:"
Write-Host ""

$boards = Get-ChildItem -Path "configs" -Directory | Select-Object -ExpandProperty Name | Sort-Object

Write-Host "GitHub Actions workflow_dispatch options format:"
Write-Host "        options:"
Write-Host "          - `"`""
foreach ($board in $boards) {
    Write-Host "          - $board"
}

Write-Host ""
Write-Host "JSON array format for reference:"
$jsonArray = '["' + ($boards -join '", "') + '"]'
Write-Host $jsonArray

Write-Host ""
Write-Host "Total boards: $($boards.Count)"
