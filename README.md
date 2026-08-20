# GQ_DOS

一个从零开始编写的 UEFI 操作系统内核（x86-64 长模式）。

> 当前版本：**v0.1.1-alpha**（遵循 SemVer）

## 这是什么

GQ_DOS 是一个"从零编写"的个人操作系统项目，核心目标只有两个：

1. **让电脑开机，在屏幕上打印文字** ✅（已达成）
2. **写出自己的文件系统**（规划中）

关键原则：

- **无任何预编译内核**：引导、显示驱动、文件系统等全部自己编写。
- 引导使用 [Limine](https://github.com/limine-bootloader/limine)（它只是加载器，不是内核）。
- 目标平台：QEMU + OVMF（UEFI，x86-64）。

## 当前进度

- [x] 阶段 0：交叉编译工具链（x86_64-elf-gcc）
- [x] 阶段 1：开机打印文字（帧缓冲 + 8x8 字体控制台 + 极简 printf）
- [ ] 阶段 2：中断 / 时钟 / 键盘
- [ ] 阶段 3：文件系统（只读，FAT32）
- [ ] 阶段 4：文件系统写入 + DOS 风格 shell

## 特性

- 更高半内核（链接于 0xffffffff80000000）
- Limine Boot Protocol 请求：base revision 6、framebuffer、HHDM
- 帧缓冲文字控制台：32bpp、自动换行、滚动、光标
- 极简 printf：%s %c %d %u %x %X %p %lu %lx
- 纯 C 内核，零汇编（Limine 以 kmain 为 ELF 入口并预置 64KiB 栈）

## 构建与运行

### 依赖

- MSYS2：x86_64-elf 交叉编译器、NASM、mtools、sgdisk、QEMU
- Limine 引导器 v12.x
- OVMF UEFI 固件

（环境搭建详见 docs/roadmap.md 与 toolchain/ 目录）

### 一键运行

    make run

### 手动构建

    make        # 生成 dist/kernel 与 dist/gqdos.img 磁盘镜像

## 目录结构

    GQ_DOS/
    ├── Makefile              # 构建与启动
    ├── limine.conf           # Limine 引导配置
    ├── linker.ld             # 更高半链接脚本
    ├── include/limine.h      # Limine 协议头
    ├── src/kernel/           # 内核（main/console/printf）
    ├── src/font/             # 8x8 位图字体
    ├── src/fs/               # 文件系统（规划中）
    ├── src/libc/             # 用户态库（规划中）
    ├── docs/                 # 文档与路线图
    ├── scripts/              # 脚本
    └── third_party/          # 第三方参考

## 技术栈

- C（x86_64-elf-gcc，-ffreestanding）
- NASM（后续阶段引入）
- Limine Boot Protocol v12.6.0
- QEMU + OVMF

## 致谢

- [Limine Bootloader](https://github.com/limine-bootloader/limine)
- [font8x8](https://github.com/dhepper/font8x8)（公有领域 8x8 位图字体，Daniel Hepper）
