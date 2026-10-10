# GQ_DOS

一个从零开始编写的 UEFI 操作系统内核（x86-64 长模式）。

> 当前版本：**v0.4.0-alpha**（遵循 SemVer）

## 这是什么

GQ_DOS 是一个"从零编写"的个人操作系统项目。核心目标只有两个：

1. **让电脑开机，在屏幕上打印文字** ✅ 已达成
2. **写出自己的文件系统** 🔶 基本达成 —— FAT32 已能挂载、列目录、读文件、创建空文件、删除文件；**向文件写入内容尚未实现**

关键原则：

- **无任何预编译内核**：显示驱动、中断处理、磁盘驱动、文件系统全部自己编写。
- 引导使用 [Limine](https://github.com/limine-bootloader/limine)（它只是加载器，不是内核）。
- 目标平台：QEMU + OVMF（UEFI，x86-64），单核、无用户态。

## 当前进度

- [x] **阶段 0**：交叉编译工具链（`x86_64-elf-gcc`）
- [x] **阶段 1**：开机打印文字（帧缓冲 + 8x8 字体控制台 + 极简 printf）
- [x] **阶段 2**：中断 / 时钟 / 键盘（IDT + ISR/IRQ + PIT + APIC + PS/2 键盘）
- [x] **阶段 3**：文件系统（FAT32 只读：挂载 / 列目录 / 读文件）
- [x] **阶段 4a**：DOS 风格 shell（8 条命令）+ 文件创建 / 删除
- [ ] **阶段 4b**：向文件写入内容（`fat32` 目前没有写数据的接口，`mkfile` 只创建 0 字节文件）
- [ ] **阶段 5**（可选，未承诺）：子目录与长文件名、多任务、网络栈

现在开机后会直接进入一个可以交互的 shell：

```
========================================
           GQ_DOS v0.4
========================================

GQ_DOS shell ready. Type 'help' for commands.
Filesystem mounted (FAT32).
> dir
  HELLO.TXT 29 bytes
> type HELLO.TXT
Hello from GQ_DOS filesystem!
> mkfile TEST.TXT
Created TEST.TXT
> del TEST.TXT
Deleted TEST.TXT
```

## 特性

### 引导与内存

- 更高半内核（链接于 `0xffffffff80000000`）
- Limine Boot Protocol base revision 6，请求 framebuffer 与 HHDM
- 超软盘镜像方案：整盘 FAT32、无分区表，OVMF 按可移动介质引导

### 显示与输出

- 帧缓冲文字控制台：32bpp、自动换行、向上滚动、光标、退格
- 可配置前景/背景色（RGB → 帧缓冲像素格式转换）
- 极简 `printf`：`%c %s %d %i %u %x %X %p %%`（支持 `l` 长度修饰，如 `%lu` / `%lx`）
- COM1 串口输出（配合 `limine.conf` 的 `serial: yes`，方便 CI / 无窗口调试）

### 中断与设备

- IDT + 异常处理（保留 `err_code` 与完整寄存器现场）
- NASM 汇编的 ISR/IRQ 入口桩（`ISR_ERR` / `ISR_NOERR` / `IRQ` 宏生成）
- 8259 PIC 重映射、8254 PIT 定时器（全局 tick 计数）
- 本地 APIC 与 EOI（QEMU 以 `-cpu qemu64,+x2apic` 运行）
- PS/2 键盘驱动：扫描码 → ASCII、修饰键与 Shift 处理

### 存储与文件系统

- ATA PIO 磁盘驱动（按 LBA 读写扇区）
- **自写 FAT32**：BPB 解析、FAT 链遍历、簇读取、目录项枚举
- 文件操作：`lookup` / `read_file` / `dir_list` / `create` / `delete`
  （`create` 目前只建立 0 字节目录项，写入数据尚未实现）

### Shell

| 命令 | 说明 |
| --- | --- |
| `help` | 列出命令 |
| `cls` | 清屏 |
| `echo <text>` | 回显 |
| `time` | 显示开机以来经过的秒数（PIT 100 Hz 计数） |
| `dir` | 列出根目录文件与大小 |
| `type <file>` | 打印文件内容 |
| `mkfile <file>` | 在根目录新建**空**文件 |
| `del <file>` | 删除文件 |

## 构建与运行

### 依赖

- **MSYS2**：`x86_64-elf` 交叉编译器、NASM、mtools（`mformat`/`mmd`/`mcopy`）、QEMU
- **[Limine](https://github.com/limine-bootloader/limine) 引导器 v12.x**（`limine-binary`）
- **OVMF** UEFI 固件（`OVMF_CODE.fd` / `OVMF_VARS.fd`）

> `Makefile` 顶部的工具路径目前写成了 `D:/msys64/...`、`D:/tools/...`，
> 换机器时需要改这几个变量（`CROSS` / `NASM` / `QEMU` / `MFORMAT` / `OVMF_CODE` / `LIMINE`）。

### 一键构建并运行

    make run

### 只构建

    make            # 生成 dist/kernel（ELF64）与 dist/gqdos.img（64 MB FAT32 镜像）

### 清理

    make clean

`release/` 里带有 `run.bat` / `run.sh`，用于在只有预编译产物的机器上启动 QEMU。

## 目录结构

    GQ_DOS/
    ├── Makefile                 # 构建与启动（含超软盘镜像打包）
    ├── limine.conf              # Limine 引导配置
    ├── linker.ld                # 更高半链接脚本
    ├── include/limine.h         # Limine 协议头
    ├── src/kernel/
    │   ├── main.c               # kmain：初始化各子系统并进入 shell
    │   ├── console.c            # 帧缓冲文字控制台
    │   ├── printf.c             # 极简 printf
    │   ├── serial.c             # COM1 串口
    │   ├── idt.c / isr.c        # 中断描述符表与异常/IRQ 分发
    │   ├── interrupts.asm       # ISR/IRQ 汇编入口桩（NASM）
    │   ├── pic.c / pit.c        # 8259 PIC 与 8254 定时器
    │   ├── apic.c               # 本地 APIC
    │   ├── kbd.c                # PS/2 键盘
    │   ├── ata.c                # ATA PIO 磁盘驱动
    │   ├── shell.c              # DOS 风格 shell
    │   └── io.h                 # 端口 I/O 内联封装
    ├── src/fs/fat32.c           # 自写 FAT32（读路径完整；写路径待实现）
    ├── src/font/font8x8_basic.h # 8x8 位图字体
    ├── src/libc/string.c        # freestanding 的 mem*/str* 子集
    ├── docs/roadmap.md          # 长期计划与阶段划分
    ├── scripts/make-release.sh  # 打包脚本
    ├── tests/host/test_fat32.c  # FAT32 的宿主机侧单元测试
    └── toolchain/build-cross.sh # 交叉工具链构建脚本

## 技术栈

- **C**（`x86_64-elf-gcc`，`-ffreestanding -mcmodel=kernel -mno-sse -mno-red-zone`）
- **NASM**（中断入口桩，`-f elf64`）
- Limine Boot Protocol v12.6.0
- QEMU + OVMF

## 已知限制

这是一个学习性质的业余项目，刻意不追求功能完备：

- **FAT32 只有读取路径完整，还没有写数据的接口**（`mkfile` 创建空文件，无法写入内容）
- 仅支持 FAT32 根目录（未实现子目录遍历与长文件名）
- 单核、单任务；无进程调度、无用户态
- 仅 ATA PIO（无 DMA / AHCI / NVMe）
- 无网络栈
- 工具链路径硬编码在 `Makefile` 中，尚未参数化

## 致谢

- [Limine Bootloader](https://github.com/limine-bootloader/limine)
- [font8x8](https://github.com/dhepper/font8x8)（公有领域 8x8 位图字体，Daniel Hepper）
