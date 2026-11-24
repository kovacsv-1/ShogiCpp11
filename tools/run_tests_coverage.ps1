# Instrumented unit tests + gcov line coverage for src/core.
# From project root:  powershell -File tools\run_tests_coverage.ps1
# .gcov notes → gcov\   (not the project root)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot -Parent)

New-Item -ItemType Directory -Force -Path bin\Debug, obj\coverage, docs, gcov | Out-Null

Write-Host "=== Compile (--coverage) ==="
g++ -std=c++11 -Wall -Isrc --coverage -O0 -g -c tests\test_core.cpp -o obj\coverage\test_core.o
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$cores = @("board", "engine", "game", "handicap", "learn", "learn_data", "notation", "piece", "rules")
foreach ($f in $cores) {
    $src = "src\core\$f.cpp"
    if (-not (Test-Path $src)) { continue }
    g++ -std=c++11 -Wall -Isrc --coverage -O0 -g -c $src -o "obj\coverage\$f.o"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "=== Link ==="
$objs = Get-ChildItem obj\coverage\*.o | ForEach-Object { $_.FullName }
g++ --coverage -o bin\Debug\test_core_cov.exe @objs -lgcov
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "=== Run tests ==="
Remove-Item obj\coverage\*.gcda -ErrorAction SilentlyContinue
& bin\Debug\test_core_cov.exe
$testRc = $LASTEXITCODE
& bin\Debug\test_core_cov.exe | Tee-Object -FilePath bin\Debug\test_core_last_run.txt | Out-Null

Write-Host "=== gcov (notes → gcov\) ==="
# Clear previous notes; gcov writes next to cwd, then we move into gcov\.
Remove-Item gcov\*.gcov -ErrorAction SilentlyContinue
Remove-Item *.gcov -ErrorAction SilentlyContinue

$raw = New-Object System.Collections.Generic.List[string]
foreach ($f in $cores) {
    if (Test-Path "obj\coverage\$f.gcda") {
        $raw.Add((& gcov -b -o obj\coverage "src\core\$f.cpp" 2>&1 | Out-String))
    }
}
if (Test-Path "obj\coverage\test_core.gcda") {
    $raw.Add((& gcov -b -o obj\coverage "tests\test_core.cpp" 2>&1 | Out-String))
}
Get-ChildItem -Path . -Filter *.gcov -File -ErrorAction SilentlyContinue |
    Move-Item -Destination gcov\ -Force
# Also gather any notes gcov dropped under obj\coverage
Get-ChildItem -Path obj\coverage -Filter *.gcov -File -ErrorAction SilentlyContinue |
    Move-Item -Destination gcov\ -Force

# Parse only project files (skip STL / MinGW headers).
$rows = @()
foreach ($block in $raw) {
    $file = $null
    foreach ($line in ($block -split "`r?`n")) {
        if ($line -match "^File '([^']+)'") {
            $file = $Matches[1] -replace '\\', '/'
        } elseif ($line -match "Lines executed:([0-9.]+)% of (\d+)") {
            $pct = [double]$Matches[1]
            $of = [int]$Matches[2]
            if ($file -and ($file -match 'src/core/[^/]+\.cpp$' -or $file -match 'tests/[^/]+\.cpp$')) {
                $rows += [pscustomobject]@{ File = $file; Pct = $pct; Of = $of }
            }
            $file = $null
        }
    }
}

$report = @()
$report += "Shogi project line coverage (gcov)"
$report += "Command: tools/run_tests_coverage.ps1"
$report += "Note: percentages are for instrumented translation units exercised by test_core."
$report += "Detailed .gcov notes: gcov\"
$report += ""
$report += "=== Summary (project sources only) ==="
foreach ($r in ($rows | Sort-Object File)) {
    $report += ("{0,6:N2}%  of {1,4} lines  {2}" -f $r.Pct, $r.Of, $r.File)
}
if ($rows.Count -gt 0) {
    $totalOf = ($rows | Measure-Object -Property Of -Sum).Sum
    $hit = ($rows | ForEach-Object { $_.Pct * $_.Of / 100.0 } | Measure-Object -Sum).Sum
    $overall = if ($totalOf -gt 0) { 100.0 * $hit / $totalOf } else { 0 }
    $report += ""
    $report += ("Overall (weighted by line count): {0:N2}% of {1} lines" -f $overall, $totalOf)
}

$reportPath = "docs\coverage_report.txt"
$report | Set-Content -Path $reportPath -Encoding UTF8
Write-Host ""
Write-Host "Wrote $reportPath"
$report | ForEach-Object { Write-Host $_ }
Write-Host "Wrote bin\Debug\test_core_last_run.txt"
Write-Host "Wrote gcov\*.gcov"
exit $testRc
