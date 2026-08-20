# GQ_DOS 从零开发长期计划（Roadmap）

> 版本: v1.1  ·  状态: 规划中  ·  作者: GQ
> 本文件是项目的"宪法"：任何技术路线变更必须在此记录变更历史，避免走回头路。

## 0. 项目目标（冻结）

1. **目标一（主线）**：电脑开机 → 屏幕上打印文字。
2. **目标二（进阶）**：拥有自己的文件系统（可读写，且与真实世界互通）。
3. **明确不做**（防止范围蔓延）：自定义分页/内存管理、多任务、用户态、图形界面、网络。
   - 做完目标二即宣告胜利；以上条目列为"远期可能"，届时单独立项。
   - 注：Limine 已替我们开好分页（更高半映射），"分页"在本文中特指"接管并自定义页表"。

## 1. 关键技术决策（定下来就不轻易改）

| 编号 | 决策 | 选择 | 理由 |
|------|------|------|------|
| D1 | CPU 模式 | 64 位长模式 (x86-64) | Limine 在 UEFI 下直接把内核送入 64 位长模式、分页已开、更高半已映射，省掉最难的一截初始化 |
| D2 | 引导方式 | Limine（UEFI） | 引导器是"加载器"而非内核；内核、驱动、文件系统 100% 自己写。用 Limine 换来的：不用写引导器、开箱即得 framebuffer 与内存映射 |
| D3 | 内核语言 | C（x86_64-elf 交叉编译）+ 少量 NASM | 汇编只用在 C 表达不了的边界（入口设栈、装载 GDT/IDT、端口/MSR 访问） |
| D4 | 磁盘与文件系统 | ATA PIO 读盘 + FAT32（ESP 原生） | FAT16 可选，见下方说明 |
| D5 | 内核格式 | ELF64（更高半链接） | Limine 直接加载 ELF，不用 flat binary，也不写 ELF 加载器 |
| D6 | 参考平台 | QEMU（OVMF 固件，UEFI） | qemu-system-x86_64；真机（UEFI U 盘）列阶段 5 |

**D4 说明**：FAT16 与 FAT32 共用约 90% 的代码（BPB、目录项、簇链思路一致），差异在 FAT
表项宽度（16/32 位）和根目录位置（固定区/簇链）。UEFI 的引导分区 ESP 原生就是 FAT32，
直接做 FAT32 最省返工、真机与 U 盘通用；若想要原汁原味 DOS 的 FAT16，只需额外写一段
分区表解析去读第二分区，成本很低。**默认 FAT32，随时可切换 FAT16**。

## 2. "从零"的界定（重要，与最初表述对齐）

- 引导器用 Limine：它是**预编译的引导/加载器，不是内核**，不违反"无预编译内核"的约束。
- 以下全部自己从零写：内核、显示驱动（帧缓冲字体渲染）、异常/中断、键盘/时钟、
  磁盘驱动（ATA PIO）、文件系统（FAT32）、shell。
- 编译器（x86_64-elf-gcc）、汇编器（NASM）、模拟器（QEMU + OVMF）是工具，不属于内核。

## 3. 防返工三原则（本计划的灵魂）

1. **接口契约先行**：每动手写代码前，先把模块间"怎么对话"写进 docs/。
   之后升级内部实现（例如磁盘驱动从 ATA PIO 换 AHCI）只改实现、不改契约，牵一发而不动全身。
2. **能主机测试的代码，先主机测试再进内核**：printf、字符串、FAT32 解析全部写成
   零内核依赖的纯 C，先在 Windows 上用真实磁盘镜像文件当"假硬盘"跑单元测试，
   通过后再链接进内核。文件系统是返工重灾区，提前在主机把坑踩平。
3. **每步可开机、可验收、打 tag**：每个里程碑结束 QEMU 里必须有可见的新东西；
   git commit + tag。任何阶段崩了，回退只损失一个小阶段。

## 4. 冻结的契约（阶段 0 定稿，后续新增而非修改）

### 4.1 Limine Boot Protocol（引导器 ↔ 内核）

- 内核通过"请求/响应结构"向 Limine 索取：framebuffer、memory map、HHDM（更高半直接映射基址）等。
- 入口约定：Limine 以 64 位长模式、分页已开、栈由引导器提供的方式跳入 _start；
  _start 立即设自己的栈（链接脚本在 BSS 预留 64KB）→ 调 kmain。
- 详情沉淀到 docs/limine-protocol.md（我们用到哪些请求、各自响应结构、魔数）。

### 4.2 内存布局

- 内核链接在更高半 0xffffffff80000000；栈由链接脚本放在 BSS 中。
- 其余内存一律走 Limine memory map，不手写硬编码物理地址；HHDM 用于按需直访物理内存。

### 4.3 模块接口清单（后续各阶段逐一实现，接口先行冻结）

- 控制台：console_putc / console_clear / console_scroll / printk(fmt, ...)（底层 = 帧缓冲字体绘制）
- 端口：inb / outb / inw / outw
- 块设备：blk_read(lba, buf, count) / blk_write(lba, buf, count)（文件系统不碰端口）
- 文件（VFS-lite）：fopen / fread / fseek / fclose / fstat / fopendir / freaddir
- 键盘：kbd_getc / kbd_readline

## 5. 里程碑计划

### 阶段 0 — 环境就绪（预计 1~2 天）
- MSYS2：提供 make/mtools，并用于编译 x86_64-elf 交叉工具链（binutils + gcc，只 C 前端 + libgcc）
  - 备选：WSL2 + Ubuntu 按 OSDev 教程构建
- 安装 NASM（Windows 安装包）、QEMU（Windows 安装包）
- OVMF 固件：edk2 官方发布或 MSYS2 包，取 OVMF_CODE.fd 与 OVMF_VARS.fd
- Limine：下载官方 release（或源码构建），放 third_party/limine
- git init；README；docs/ 骨架；Makefile；limine.cfg
- **验收**：make run 启动 OVMF → Limine → 加载一个空内核并在串口打印（阶段 1 的起点）

### 阶段 1 — 开机打印文字 🎯 目标一达成（预计 1~2 周）
- 1.1 交叉编译最小 kernel.elf：入口 _start.asm（设栈 → kmain）、链接脚本（更高半 0xffffffff80000000）
- 1.2 limine.cfg（内核路径、协议版本）+ 用 limine 工具部署进 GPT/FAT32 镜像 → OVMF 启动
- 1.3 发 framebuffer 请求：拿到基址 / 宽 / 高 / pitch / bpp（含颜色 mask 字段）
- 1.4 内嵌 8x16 位图字体 + console_putc / printk：把字形画进帧缓冲；另写 10 行串口驱动（COM1）当调试后门
- 1.5 IDT + 异常总处理器（64 位中断门）：任何异常 → 打印寄存器现场 + 停机（性价比最高的调试投资）
- **验收**：QEMU(UEFI) 图形屏幕显示引导与内核文字，其中内核文字由 C 代码打印

### 阶段 2 — 中断、时钟、键盘（预计 1 周，可选但强烈建议）
- 长模式下重编程 8259 PIC（IRQ 重映射到 0x20-0x2F）、PIT 100Hz（端口 0x40/0x43）、
  PS/2 键盘（0x60/0x64，IRQ1）扫描码 → ASCII、按键回显
- 注：这些传统设备在 QEMU 长模式下仍可访问，先按此做（成本低）；真机 UEFI 建议换 APIC（列阶段 5）
- **验收**：按键字符上屏，屏幕角落有时钟计数

### 阶段 3 — 文件系统（只读）🎯 目标二上半场（预计 2~3 周）
- 3.1 ATA PIO 驱动：28 位 LBA 读扇区（轮询 + 超时，QEMU 固定 -drive if=ide）
- 3.2 块设备抽象：blk_read / blk_write（文件系统不直接碰端口）
- 3.3 FAT32 解析（纯 C、零内核依赖）：BPB → FSInfo → 根目录簇链 →
      文件 API：open / read / seek / close / stat + 目录列举
- 3.4 主机先验证：mtools 造一个真实 FAT32 镜像放入 hello.txt，主机单元测试读出；通过后再链接进内核
- 3.5 引导侧无需改动：Limine 负责加载 kernel.elf，我们的内核只读数据文件
- **验收**：hello.txt 放进 ESP，GQ_DOS 开机后自己读出并打印；同一镜像 Windows 也能正常识别

### 阶段 4 — 文件系统写入 + DOS 风格 shell 🎯 目标二达成（预计 2~4 周）
- 4.1 FAT32 写入：分配簇、扩展簇链、更新 FAT 与目录项；创建 / 删除 / 重命名文件
- 4.2 shell：提示符 C:\>，命令 help / cls / echo / dir / type / mkfile / del / time
- **验收**：在 GQ_DOS 里创建文件、写入内容、重启后仍在；镜像挂到 Windows 能看到该文件；
      反过来 Windows 放进去的文件 GQ_DOS 也能读
- 🎉 至此两个目标全部达成

### 阶段 5 — 可选延伸（看心情，不承诺）
- 真机 UEFI：把镜像写进 U 盘直接启动（FAT32 方案天然兼容）
- 中断升级：8259 PIC → APIC（真机更"正统"）
- 磁盘驱动升级：ATA PIO → AHCI / NVMe（换实现，blk_read/blk_write 契约不变）
- 自定义分页 / 内存管理 / 多任务雏形

## 6. 目录结构（阶段 0 建好）

    GQ_DOS/
    ├── Makefile
    ├── limine.cfg
    ├── README.md
    ├── docs/               # roadmap / limine-protocol / memory-map / design / build
    ├── toolchain/          # x86_64-elf 交叉编译器构建脚本
    ├── third_party/limine/ # limine 二进制（gitignore 或 submodule）
    ├── src/kernel/         # entry.asm kmain.c console.c gdt.c idt.c pic.c pit.c kbd.c ata.c serial.c
    ├── src/fs/             # fat32.c fs.h        （零内核依赖）
    ├── src/libc/           # string.c printf.c   （零内核依赖）
    ├── src/font/           # font8x16.h（位图字体）
    ├── tests/              # 主机端单元测试
    ├── scripts/            # run.sh mkdisk.sh deploy.sh
    └── dist/               # 镜像等产物（gitignore）

## 7. 构建与测试策略

- **交叉编译**：x86_64-elf-gcc，参数固定
  -ffreestanding -fno-pie -fno-stack-protector -mno-red-zone -mcmodel=kernel -nostdlib
  → ld（OUTPUT_FORMAT elf64-x86-64）→ kernel.elf
- **磁盘镜像**：先建 GPT 磁盘 → FAT32 ESP → 拷入 EFI/BOOT/BOOTX64.EFI + limine.cfg + kernel.elf
  （mtools / guestfish 操作镜像文件，避免管理员权限挂载）
- **运行**：qemu-system-x86_64
  -drive if=pflash,format=raw,file=OVMF_CODE.fd
  -drive if=pflash,format=raw,file=OVMF_VARS.fd
  -drive file=dist/gqdos.img,format=raw,if=ide -serial stdio -no-reboot
- **调试**：QEMU -d int -D qemu.log 日志；串口日志；阶段 1.5 的异常总处理器
- **主机单测**：fs / libc 用普通 gcc 编译，块设备 = fopen(disk.img) 假硬盘；
  参考镜像用 mtools 制作，对照 OSDev FAT 页面与微软 FAT 规格
- **协作验证**：AI 助手后台跑 QEMU，用 monitor 的 screendump 截图回读，确认文字真的显示在屏幕上

## 8. 风险与对策

| 风险 | 对策 |
|------|------|
| 交叉编译器在 Windows 上编译失败 | 备选 WSL2 + Ubuntu（更顺）；再不行用现成 x86_64-elf 工具链（工具不算内核） |
| OVMF / Limine 部署流程不熟 | 官方 limine-c-template 与"Booting with Limine"教程可对照；阶段 0 把一条 make run 跑通 |
| framebuffer 像素格式差异（RGB/BGR 等） | 只支持 32bpp RGB，读 Limine framebuffer 的颜色 mask 字段自适应 |
| 长模式 IDT 配置错误导致三重故障 | 1.5 异常总处理器 + QEMU -d int 日志，逐寄存器排查 |
| ATA 读盘时序问题 | 只针对 QEMU 一种硬件，if=ide 写进脚本，轮询带超时 |
| FAT32 细节坑（簇链、FSInfo、边界） | 全部先在主机单测中用真实镜像验证 |
| 中途失去动力 | 每阶段都有看得见的开机成果；git tag 每个里程碑 |

## 9. 时间估算（业余节奏）

| 阶段 | 内容 | 耗时 |
|------|------|------|
| 0 | 环境就绪 | 1~2 天 |
| 1 | 开机打印文字 | 1~2 周（比 BIOS 路线更快，省了写引导器） |
| 2 | 中断/时钟/键盘 | 1 周 |
| 3 | 文件系统只读 | 2~3 周 |
| 4 | 文件系统写入 + shell | 2~4 周 |
| 5 | 可选延伸 | 不限 |
| **合计** | **两个目标达成** | **约 2~3 个月** |

## 10. 参考资源

- Limine 官方：GitHub Limine-Bootloader/limine + limine-protocol/PROTOCOL.md（协议权威）
- limine-c-template：官方最小 C 内核模板，阶段 0 直接对照（GNUmakefile、limine.cfg、部署脚本）
- 教程：gysddn.github.io "Booting with Limine"
- OSDev Wiki：Limine / Interrupts / IDT / ATA PIO Mode / FAT
- 李忠《x86 汇编语言：从实模式到保护模式》（保护模式/段/页基础仍有用）
- 郑钢《操作系统真象还原》（中文从零写内核，思路可对照）
- Intel 软件开发手册第 3 卷（后期查寄存器/MSR 细节）
- edk2 OVMF 发布页（拿 OVMF_CODE.fd / OVMF_VARS.fd）

## 11. 变更记录

| 日期 | 版本 | 变更内容 |
|------|------|----------|
| 2026-08-20 | v1.1 | 重大调整：引导改为 Limine(UEFI)；32 位保护模式→64 位长模式；FAT16→FAT32(ESP)；flat binary→ELF64。删除自写引导器里程碑，新增 Limine 部署与 framebuffer 字体渲染 |
| 2025-xx-xx | v1.0 | 初始计划（BIOS + 自写引导器 + 32 位 + FAT16） |
