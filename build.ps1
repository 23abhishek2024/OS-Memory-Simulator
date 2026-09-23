# Build Script for OS Memory Simulator
Write-Host "Compiling OS Memory Simulator..." -ForegroundColor Cyan

$cmd = "g++ -std=c++14 -O2 -Iinclude src/PageReplacement.cpp src/VirtualMemory.cpp src/main.cpp -o vmem_simulator.exe"
Invoke-Expression $cmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "[SUCCESS] Compilation completed: vmem_simulator.exe" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Compilation failed." -ForegroundColor Red
}
