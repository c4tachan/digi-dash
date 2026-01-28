#!/bin/bash
cd /home/catachan/projects/digi-dash/firmware
/home/catachan/qemu-esp-develop/build/qemu-system-xtensa \
  -machine esp32s3 \
  -drive file=build/qemu_flash.bin,if=mtd,format=raw \
  -serial file:/tmp/qemu_render.log \
  -display none &

QEMU_PID=$!
echo "QEMU started with PID $QEMU_PID"
sleep 8
kill $QEMU_PID 2>/dev/null
wait $QEMU_PID 2>/dev/null

echo ""
echo "====== Rendering Statistics ======"
grep "pixels drawn" /tmp/qemu_render.log | tail -3
echo ""
echo "====== Last 40 lines ======"
tail -40 /tmp/qemu_render.log
