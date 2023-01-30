# dragonfly
C++ 20(modules) 期货/期指 图表分析库
### 系统要求:
- Visual Studio 2022 Community
- Windows 10/11.

### 安装
- [安装 `vcpkg`](https://github.com/microsoft/vcpkg/releases/tag/2023.01.09)
- vcpkg install boost-filesystem boost-algorithm flatbuffers magic-enum spdlog SQLiteCpp protobuf cpp-httplib --triplet=x64-windows

### 使用
```c++
import dragonfly;
namespace df = dragonfly;

// 直接使用 dragonfly会自动下载数据, 自动存储到本地,自动更新
df::Chart chart("AU", df::Period::D1, system_clock::now() - std::chrono::days(30), system_clock::now());

// 指标
chart.macd(12, 26, 9);
chart.kdj(9, 3, 3);
chart.rsi(6);
// 更多指标请参考ind目录
```
合约信息
```c++
for (auto& spec : ContractSpec::instance().specs) {
   spec.id; spec.contract_size;
}
```
回测（experimental）
```c++
using namespace experimental;
Account account;
Position pos;
pos.Open(Position::Long,chart,i,3232);
pos.Close(i+12,3300);
account.history.push_back(pos);
std::cout << account.win_rate();
std::cout << account.profit();
std::cout << account.max_drawdown();
```