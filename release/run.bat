@echo off
REM GQ_DOS 一键运行脚本 (Windows)
setlocal
cd /d "%~dp0"
if not exist ovmf\OVMF_CODE.fd (
    echo [错误] 缺少 ovmf\OVMF_CODE.fd
    exit /b 1
)
copy /Y ovmf\OVMF_VARS.fd ovmf_vars_run.fd >nul 2>&1
where qemu-system-x86_64 >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 qemu-system-x86_64，请先安装 QEMU 并加入 PATH
    exit /b 1
)
qemu-system-x86_64 -m 512M -cpu qemu64,+x2apic ^
  -drive if=pflash,format=raw,unit=0,file=ovmf\OVMF_CODE.fd,readonly=on ^
  -drive if=pflash,format=raw,unit=1,file=ovmf_vars_run.fd ^
  -drive format=raw,file=gqdos.img,if=ide
