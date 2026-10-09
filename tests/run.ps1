$ErrorActionPreference = 'Stop'
$testDir = Join-Path $env:TEMP 'sar-verificacion'
$compiler = Get-ChildItem "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avr-gcc\*\bin\avr-g++.exe" | Select-Object -Last 1
if (!$compiler) { throw 'Instalar Arduino AVR Boards.' }
$objcopy = Join-Path $compiler.DirectoryName 'avr-objcopy.exe'
New-Item -ItemType Directory -Force -Path $testDir | Out-Null
if (!(Test-Path (Join-Path $testDir 'node_modules\avr8js'))) {
    & npm install --prefix $testDir avr8js --ignore-scripts --no-audit --no-fund
    if ($LASTEXITCODE) { throw 'Fallo instalacion avr8js.' }
}
$elf = Join-Path $testDir "integracion.elf"
$bin = Join-Path $testDir "integracion.bin"
& $compiler.FullName -mmcu=atmega328p -Os -std=gnu++11 "-I$PSScriptRoot\mocks" (Join-Path $PSScriptRoot 'integracion.cpp') -o $elf
if ($LASTEXITCODE) { throw 'Fallo compilacion.' }
& $objcopy -O binary $elf $bin
if ($LASTEXITCODE) { throw 'Fallo conversion.' }
& node (Join-Path $PSScriptRoot 'run.cjs') (Join-Path $testDir 'node_modules\avr8js') $bin
if ($LASTEXITCODE) { throw 'Fallo prueba SAR.' }
