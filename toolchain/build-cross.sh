#!/bin/bash
# 构建 x86_64-elf 交叉编译器（binutils + gcc）
# 用法（在 MSYS2 中）: bash toolchain/build-cross.sh
set -e

PREFIX=/opt/cross
TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

BINUTILS=binutils-2.42
GCC=gcc-13.2.0
WORK=/tmp/cross-build
mkdir -p "$WORK"
cd "$WORK"

echo "== 下载源码（需网络）=="
[ -f $BINUTILS.tar.xz ] || curl -fL -o $BINUTILS.tar.xz https://ftp.gnu.org/gnu/binutils/$BINUTILS.tar.xz
[ -f $GCC.tar.xz ] || curl -fL -o $GCC.tar.xz https://ftp.gnu.org/gnu/gcc/$GCC/$GCC.tar.xz

echo "== 解压 =="
tar xf $BINUTILS.tar.xz
tar xf $GCC.tar.xz

echo "== 构建 binutils =="
mkdir -p build-binutils && cd build-binutils
../$BINUTILS/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make -j$(nproc) && make install

echo "== 构建 gcc =="
cd "$WORK" && mkdir -p build-gcc && cd build-gcc
../$GCC/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-multilib
make -j$(nproc) all-gcc all-target-libgcc
make install-gcc install-target-libgcc

echo "== 完成 =="
"$PREFIX/bin/$TARGET-gcc" --version
