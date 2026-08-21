GQ_DOS v0.4.0-alpha 系统安装包
================================

一个从零编写的 UEFI 操作系统（x86-64），引导使用 Limine。

运行要求：
  - QEMU（qemu-system-x86_64，需在 PATH 中，建议 7.0 以上）
  - 建议 512MB 以上可用内存

运行方法：
  - Windows：双击 run.bat（或在命令行运行 run.bat）
  - Linux/macOS：chmod +x run.sh && ./run.sh

启动后：
  屏幕显示 GQ_DOS 启动画面，进入 shell 提示符 "> "。
  可用命令：help cls echo time dir type mkfile del
  输入 help 查看说明。

镜像说明：
  gqdos.img 是 FAT32 启动盘，含内核与 hello.txt 测试文件。
  ovmf/ 是 UEFI 固件（QEMU 启动用）。

源码：https://github.com/guaiqi1964-dev/GQ_DOS
