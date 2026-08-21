# GQ_DOS 构建脚本

# 关闭 MSYS2 路径转换：项目路径含空格时，原生工具参数会被拆坏
export MSYS2_ARG_CONV_EXCL := *

CROSS := D:/msys64/opt/cross/bin/x86_64-elf
CC    := $(CROSS)-gcc
NASM  := D:/msys64/usr/bin/nasm

QEMU      := D:/msys64/mingw64/bin/qemu-system-x86_64
MFORMAT   := D:/msys64/mingw64/bin/mformat
MMD       := D:/msys64/mingw64/bin/mmd
MCOPY     := D:/msys64/mingw64/bin/mcopy
OVMF_CODE := D:/tools/ovmf/OVMF_CODE.fd
OVMF_VARS := D:/tools/ovmf/OVMF_VARS.fd
LIMINE    := D:/tools/limine/limine-binary

CFLAGS := -g -O2 -std=gnu11 -ffreestanding -fno-stack-protector -fno-stack-check \
	-fno-pic -fno-pie -fno-lto -m64 -march=x86-64 -mabi=sysv \
	-mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel \
	-ffunction-sections -fdata-sections -Wall -Wextra \
	-Iinclude -Isrc/kernel -Isrc/font

LDFLAGS := -nostdlib -static -Wl,-z,max-page-size=0x1000 -Wl,--gc-sections -Wl,--build-id=none -lgcc

SRCS   := $(wildcard src/kernel/*.c)
ASMS   := $(wildcard src/kernel/*.asm)
OBJS   := $(patsubst %.c,%.o,$(SRCS)) $(patsubst %.asm,%.o,$(ASMS))

KERNEL := dist/kernel
IMAGE  := dist/gqdos.img

.PHONY: all run clean

all: $(IMAGE)

$(KERNEL): $(OBJS) linker.ld
	mkdir -p dist
	$(CC) $(CFLAGS) -T linker.ld $(OBJS) $(LDFLAGS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.asm
	$(NASM) -f elf64 $< -o $@

# 超软盘方案：整盘 FAT32（无分区表），OVMF 按可移动介质路径引导
$(IMAGE): $(KERNEL) limine.conf
	mkdir -p dist
	dd if=/dev/zero bs=1M count=0 seek=64 of=$@
	$(MFORMAT) -F -i $@
	$(MMD) -i $@ ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	$(MCOPY) -i $@ $(KERNEL) ::/boot/kernel
	$(MCOPY) -i $@ limine.conf ::/boot/limine/limine.conf
	$(MCOPY) -i $@ $(LIMINE)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI

run: $(IMAGE)
	mkdir -p dist
	cp -f $(OVMF_VARS) dist/ovmf_vars.fd
	$(QEMU) -m 512M -cpu qemu64,+x2apic \
		-drive if=pflash,format=raw,unit=0,file=$(OVMF_CODE),readonly=on \
		-drive if=pflash,format=raw,unit=1,file=dist/ovmf_vars.fd \
		-drive format=raw,file=$(IMAGE),if=ide \
		-serial stdio -no-reboot -no-shutdown

clean:
	rm -f $(OBJS) $(KERNEL) $(IMAGE) dist/ovmf_vars.fd
