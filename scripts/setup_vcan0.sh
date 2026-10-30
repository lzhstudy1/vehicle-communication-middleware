#!/usr/bin/env bash
set -euo pipefail

IFACE="${1:-vcan0}"

if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    SUDO="sudo"
fi

echo "[INFO] 目标接口: $IFACE"

# 1. 加载 vcan 模块
if ! $SUDO modprobe vcan 2>/dev/null; then
    echo "[ERROR] 加载 vcan 模块失败。"
    echo "请尝试安装内核额外模块："
    echo "  sudo apt install linux-modules-extra-$(uname -r)"
    exit 1
fi
echo "[INFO] vcan 模块已加载"

# 2. 接口不存在才创建
if ip link show "$IFACE" >/dev/null 2>&1; then
    echo "[INFO] $IFACE 已存在，跳过创建"
else
    echo "[INFO] 创建 $IFACE ..."
    $SUDO ip link add dev "$IFACE" type vcan
fi

# 3. 确保接口 UP
$SUDO ip link set "$IFACE" up
echo "[INFO] $IFACE 已设置为 UP"

# 4. 打印状态
ip -details link show "$IFACE"

echo "[OK] $IFACE 已就绪。"
echo "测试方法："
echo "  终端1: candump $IFACE"
echo "  终端2: cansend $IFACE 123#1122334455667788"
