#!/usr/bin/env python3
from pathlib import Path
proj = Path(__file__).resolve().parent
build = proj / 'build'
qemu_flash = bytearray(0x200000)
with open(build / 'bootloader' / 'bootloader.bin','rb') as f:
    data = f.read(); qemu_flash[0x0:len(data)] = data
with open(build / 'partition_table' / 'partition-table.bin','rb') as f:
    data = f.read(); qemu_flash[0x8000:0x8000+len(data)] = data
with open(build / 'digi-dash.bin','rb') as f:
    data = f.read(); qemu_flash[0x10000:0x10000+len(data)] = data
with open(build / 'spiffs.bin','rb') as f:
    data = f.read(); qemu_flash[0xe0000:0xe0000+len(data)] = data
with open(build / 'qemu_flash.bin','wb') as f:
    f.write(qemu_flash)
print('wrote', build / 'qemu_flash.bin')
