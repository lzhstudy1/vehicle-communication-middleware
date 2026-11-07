出现这个情况，是因为你在复制 Markdown 文本时，**没有把“代码块”的语法（三个反引号 \`\`\`）一起带进去**，导致 GitHub 把所有内容都当成了普通的段落文字挤在了一起。

另外，系统架构图和目录结构必须放在**代码块**里，才能保持对齐和等宽字体。

我帮你重新优化了一版，带上了更多现代化的 Emoji 图标，并且**把所有需要的格式符号都补齐了**。

请打开你的 `README.md`，把现在的内容**全部删掉**，将下面这段原封不动地复制进去（你可以直接点编辑器右上角的“复制代码”按钮）：

```markdown
# 🚗 车载通信中间件 VCM (Vehicle Communication Middleware)

本项目是一个基于 Linux SocketCAN 实现的**轻量级车载通信中间件**。旨在模拟真实车载 ECU 中的底层通信与诊断协议栈，覆盖了从原始 CAN 报文收发、信号解析，到 ISO-TP 多帧传输的完整数据链路。

## ✨ 核心特性

- 📡 **底层通信 (SocketCAN)**：基于 Linux 原生 SocketCAN API 封装，支持 CAN 帧收发、多级硬件过滤掩码、超时重传保护。
- 🚀 **无锁并发 (Lock-Free)**：手写基于 C++11 内存序（Memory Order）的单生产者-单消费者（SPSC）无锁环形队列，实现高频数据零锁开销吞吐。
- 📊 **信号解析 (DBC Parser)**：支持解析 DBC 文件，精准处理 Intel（小端）/ Motorola（大端）字节序、跨字节提取与补码符号扩展。
- 📦 **传输层协议 (ISO-TP)**：完整实现 ISO 15765-2 协议，支持单帧/多帧拆包与拼装，含流控帧（FC）、块大小（BS）、最小间隔（STmin）及 SN 序号校验。
- 🛠️ **工程化支持**：使用 CMake 进行多目标构建，集成 Google Test 单元测试，支持 ThreadSanitizer (TSan) 检测数据竞争，代码行覆盖率达 77%+。

## 🏗️ 系统架构

```text
+---------------------------------------------------+
|                Application (main.cpp)             |
+---------------------------------------------------+
|            ISO-TP (Sender / Receiver)             |
+---------------------------------------------------+
|         DBC Parser (Signal Database)              |
+---------------------------------------------------+
|           SocketCAN (CanSocket / CanFrame)        |
+---------------------------------------------------+
|  Foundation (Logger / LockFree RingBuffer / CMake)|
+---------------------------------------------------+
```

## 📂 目录结构

```text
vcm/
├── app/          # 主程序（端到端集成演示）
├── include/vcm/  # 核心头文件
├── src/          # 核心源文件 (SocketCAN, ISO-TP, DBC, Logger, RingBuffer)
├── tests/        # Google Test 单元测试代码
├── dbc/          # DBC 数据库及自动生成的 JSON 文件
├── scripts/      # Python 辅助脚本 (DBC转JSON)
└── CMakeLists.txt
```

## 🛠️ 快速开始

### 1. 环境依赖
- Ubuntu 22.04 (或更高版本)
- CMake >= 3.16, GCC >= 11 (支持 C++17)
- `can-utils` (用于测试虚拟 CAN 总线)

```bash
sudo apt update
sudo apt install build-essential cmake can-utils
```

### 2. 启用虚拟 CAN 总线 (vcan0)
```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

### 3. 编译与测试
```bash
git clone https://github.com/你的用户名/vehicle-communication-middleware.git
cd vehicle-communication-middleware
mkdir build && cd build

# 构建项目 (包含测试)
cmake ..
make -j$(nproc)

# 运行所有单元测试
ctest --output-on-failure
```

### 4. 运行全链路演示 (端到端联调)
```bash
# 在 build 目录下返回根目录执行，确保找到 DBC 文件
cd ..
./build/app/vcm_app
```
*(终端将打印模拟诊断仪发送诊断请求，与 ECU 接收拼装解析的完整日志)*

## 🧪 进阶测试与质量保证

**开启 ThreadSanitizer (TSan) 验证多线程安全**：
```bash
cd build
cmake -DENABLE_TSAN=ON ..
make -j
# 使用 setarch 绕过内核地址随机化，防止 TSan 报错
setarch $(uname -m) -R ./tests/vcm_tests
```

## 📝 协议实现说明
- **CAN 过滤**：支持按 ID 范围或掩码过滤（如 `0x100 / 0x700` 匹配 `0x100~0x1FF`）。
- **ISO-TP 流控**：接收方处理长数据时，若缓存溢出（`>4096` 字节），自动回复 `FS=2` (Overflow) 终止传输。
- **字节序处理**：在 `SignalDatabase` 中针对 Motorola 跨字节信号，采用按位游标跳跃算法实现无损提取。

## 📄 License
This project is licensed under the MIT License.
```

### 💡 复制后检查两点：
1. 确保包含 `text` 字样的地方，外面都包裹了三个反引号 ` ``` `。
2. 保存后，点击 GitHub 的 `Preview`（预览）标签页，你就能看到排版完美的标题、列表、架构图和代码高亮了！

改完之后 `git add . && git commit -m "docs: beautify README" && git push`，再去刷新你的 GitHub 主页，绝对是顶级开源项目的视觉效果！