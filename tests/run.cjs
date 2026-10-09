// Ejecutar con node run.cjs <ruta-avr8js> <binario-AVR>.
const fs = require('node:fs');
const { CPU, avrInstruction } = require(process.argv[2]);
const bytes = fs.readFileSync(process.argv[3]);
const program = new Uint16Array(16384);
for (let i = 0; i < bytes.length; i += 2) program[i / 2] = bytes[i] | ((bytes[i + 1] || 0) << 8);
const cpu = new CPU(program);
while (cpu.cycles < 50000000 && ![0xa5, 0xee].includes(cpu.data[0x3e])) {
  avrInstruction(cpu);
  cpu.tick();
}
if (cpu.data[0x3e] !== 0xa5) {
  throw new Error(`Prueba fallida o timeout: caso=${cpu.data[0x4a]}, ciclos=${cpu.cycles}`);
}
console.log(`PASS: compatibilidad SAR, sensores, servos y luces con perifericos simulados; ${cpu.cycles} ciclos.`);
