#!/bin/sh
# GQ_DOS 一键运行脚本 (Linux/macOS)
cd "$(dirname "$0")" || exit 1
[ -f ovmf/OVMF_CODE.fd ] || { echo "[错误] 缺少 ovmf/OVMF_CODE.fd"; exit 1; }
cp -f ovmf/OVMF_VARS.fd ovmf_vars_run.fd
exec qemu-system-x86_64 -m 512M -cpu qemu64,+x2apic \
  -drive if=pflash,format=raw,unit=0,file=ovmf/OVMF_CODE.fd,readonly=on \
  -drive if=pflash,format=raw,unit=1,file=ovmf_vars_run.fd \
  -drive format=raw,file=gqdos.img,if=ide
