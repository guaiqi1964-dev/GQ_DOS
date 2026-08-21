#!/bin/bash
# 生成 GQ_DOS 系统安装包（zip）
# 用法（MSYS2/Linux）: bash scripts/make-release.sh
set -e
cd "$(dirname "$0")/.."

VER=v0.4.0-alpha
PKG="GQ_DOS-$VER"
REL="dist/release/$PKG"
# OVMF 固件路径（按本机实际位置修改）
OVMF_CODE="D:/tools/ovmf/OVMF_CODE.fd"
OVMF_VARS="D:/tools/ovmf/OVMF_VARS.fd"

echo "== 构建 =="
make

echo "== 组装安装包 =="
rm -rf "$REL"
mkdir -p "$REL/ovmf"
cp dist/gqdos.img "$REL/"
cp "$OVMF_CODE" "$REL/ovmf/"
cp "$OVMF_VARS" "$REL/ovmf/"
cp release/run.bat release/run.sh release/README.txt "$REL/"

echo "== 压缩 =="
rm -f "dist/$PKG.zip"
tar -a -cf "dist/$PKG.zip" -C dist/release "$PKG"

echo "完成: dist/$PKG.zip"
echo "上传: 在 GitHub 上创建 Release（tag $VER）并上传该 zip 即可。"
