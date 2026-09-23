# PowerShell Build Script for Virtual Memory Engine
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Compiling Virtual Memory & Page Replacement Engine...   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

if (!(Test-Path "logs")) {
    New-Item -ItemType Directory -Path "logs" | Out-Null
}

$sources = @(
    "src/FIFOReplacement.cpp",
    "src/LRUReplacement.cpp",
    "src/LFUReplacement.cpp",
    "src/MFUReplacement.cpp",
    "src/OptimalReplacement.cpp",
    "src/ClockReplacement.cpp",
    "src/PageTable.cpp",
    "src/TLB.cpp",
    "src/VirtualMemoryEngine.cpp",
    "src/BeladyAnomalyDetector.cpp",
    "src/BenchmarkRunner.cpp",
    "src/Visualizer.cpp",
    "src/main.cpp"
)

$cmd = "g++ -std=c++14 -O2 -Iinclude " + ($sources -join " ") + " -o vmem_simulator.exe"
Write-Host "Executing: $cmd" -ForegroundColor Gray

Invoke-Expression $cmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] Build completed successfully: vmem_simulator.exe" -ForegroundColor Green
    Write-Host "Run .\vmem_simulator.exe or .\run.ps1 to launch interactive mode." -ForegroundColor Yellow
} else {
    Write-Host "`n[ERROR] Build failed with exit code $LASTEXITCODE." -ForegroundColor Red
}
