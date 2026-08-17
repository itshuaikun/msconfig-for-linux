#!/usr/bin/env bash
# 一键构建：任意发行版只需安装 cmake + Qt(5/6) Widgets 开发包
set -e
cd "$(dirname "$0")"

cmake -S . -B build
cmake --build build -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo
echo "构建完成: $(pwd)/build/msconfig"
echo "直接运行:      ./build/msconfig"
echo "后端自检:      ./build/msconfig --check"
