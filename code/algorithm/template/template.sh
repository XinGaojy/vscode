#!/bin/bash
set -e
SRC=template.cpp
BASE=$(basename $SRC .cpp)

echo '========== 1. 预处理结果（宏展开后） =========='
g++ -E -std=c++17 $SRC -o ${BASE}.i
# 只看最后 200 行，防止 IO 爆炸
tail -n 200 ${BASE}.i > ${BASE}_tail.i
less ${BASE}_tail.i

echo '========== 2. 模板实例化树（-fdump-tree-all） =========='
g++ -std=c++17 -fdump-tree-all $SRC -c
# 文件名格式：fib_tmp.cpp.006t.class
TU_DUMP=$(ls -v ${BASE}.cpp.*t.* | tail -n 1)
echo ">>> 最完整的实例化 dump 文件：$TU_DUMP"
less $TU_DUMP

echo '========== 3. 中间 RTL（可选，更底层） =========='
g++ -std=c++17 -fdump-rtl-expand $SRC -c
RTL_DUMP=${BASE}.cpp.129r.expand
less $RTL_DUMP

echo '========== 4. 最终汇编（带源码对照） =========='
g++ -std=c++17 -S -fverbose-asm -O2 $SRC -o ${BASE}.s
less ${BASE}.s
