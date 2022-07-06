# dragonfly
C++ 20(modules) 期货/期指 图表分析库
### 系统要求:
- Visual Studio 2022 Community
- Windows 10/11.

### 安装
- [安装 `vcpkg`](https://github.com/microsoft/vcpkg/releases/tag/2022.06.15)
- vcpkg install boost-filesystem boost-algorithm flatbuffers magic-enum spdlog SQLiteCpp protobuf cpp-httplib --triplet=x64-windows

### 使用
```c++
import dragonfly;
namespace df = dragonfly;

// 直接使用 dragonfly会自动下载数据, 自动存储到本地
df::Chart chart("AU", df::Period::D1, system_clock::now() - std::chrono::days(30), system_clock::now());

// 指标
chart.macd(12, 26, 9);
chart.kdj(9, 3, 3);
chart.rsi(6);
```
