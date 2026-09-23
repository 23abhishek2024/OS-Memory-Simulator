# PowerShell Run Script for Virtual Memory Engine
if (!(Test-Path "vmem_simulator.exe")) {
    Write-Host "vmem_simulator.exe not found. Building first..." -ForegroundColor Yellow
    .\build.ps1
}

if (Test-Path "vmem_simulator.exe") {
    .\vmem_simulator.exe $args
}
