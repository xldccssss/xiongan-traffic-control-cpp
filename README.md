# 雄安区路网 TESSNG 4.1.3 原生 C++ 二次开发版

本目录是独立的原生 C++ 工程。原 `雄安区路网_TESSNG5二次开发` Python＋C++ 桥接工程保持不变，本工程运行时不启动 Python、HTTP 网关或 PyTorch。

## Visual Studio 启动（官方 QuickStart 方式）

本工程保持官方示例的启动链：`QApplication -> tessng() -> MyPlugin::init() -> loadPluginFromMem() -> Qt 事件循环`。现有控制算法没有因工程配置调整而改变。

本项目现在直接按 `CMakeLists.txt` 启动，不依赖生成的 `.sln`：

1. 双击 `open-visual-studio.cmd`。它会让 Visual Studio 直接打开本项目文件夹，并自动识别根目录的 `CMakeLists.txt`。也可以在 Visual Studio 中选择“文件 → 打开 → CMake”，手动选择本目录的 `CMakeLists.txt`。
2. 等待 Visual Studio 完成 CMake 配置；它会优先读取 `CMakePresets.json`，关闭 Preset 支持的 Visual Studio 则读取等价的 `CMakeSettings.json`。两者都把 `x64-Release` 限定为唯一的 Release 子配置，不会再从 Release 目录启动 Debug 程序。
3. 在顶部配置中选择 `TESSNG 4.1.3 - x64 Release`。需要逐行调试时才选择 Debug。
4. 顶部启动项应显示 `CMakeLists.txt`。若尚未自动显示，右键 `CMakeLists.txt` 选择“设置为启动项”。`.vs/launch.vs.json` 已把该 CMake 工程绑定到实际运行目标 `XionganTrafficNativeCpp.exe`，并预设运行目录和环境变量。
5. 按 `F5`。CMake 会编译目标、把匹配的 TESSNG 运行库部署到可执行文件旁，再启动程序。程序加载路网后等待手动点击“开始”，不会自动开始仿真。

请不要再打开 `build\Debug` 或 `build\Release` 中旧的 `.sln`；这些目录只作为 CMake 的编译缓存和产物目录。

命令行方式：

- Release：先运行 `build-release.cmd`，再运行 `run.cmd`。
- Debug：先运行 `build-debug.cmd`，再运行 `run-debug.cmd`。
- 自检：运行 `verify.cmd`。
- 完整运行态自检：先完成 Release 编译，再运行 `verify-runtime.cmd`；程序自动仿真 6 秒并退出，结果写入 `generated\native_runtime_smoke_report.json`。

编译产物分别位于 `bin\Debug` 和 `bin\Release`；`runtime` 保存 Release 所需的 TESSNG/Qt/MSVC 运行库。换电脑时仍需依照 TESSNG 授权要求配置该电脑的 Key，并按新电脑实际位置修改 `CMakePresets.json` 中的 `QT_ROOT` 与 `TESSNG_SDK_ROOT`。

## 已配置环境

- Visual Studio：`F:\DevTools\VisualStudio\2022\Community`
- Qt：`F:\Qt\5.15.2\msvc2019_64`
- TESSNG SDK：`F:\TESSNG_4.1.3`
- C++ SDK 母版：`F:\TESSNG_4.1.3\SecondDev\TESS_CppAPI_BATCH_COMMANDS_EXAMPLE`
- TESSNG 头文件：`include`
- TESSNG 链接库：`lib\debug`、`lib\release`
- 默认路网：`WorkSpace\Data\雄安区路网_C++本地仿真版.tess`
- 启动配置：可执行文件同目录的 `config.json`，其中 `__simuafterload=false`

项目目录与启动流程参照普通 `TESS_CppAPI_EXAMPLE` QuickStart；实际头文件、链接库和运行库统一取自同版本的 `TESS_CppAPI_BATCH_COMMANDS_EXAMPLE`，因为本工程使用了 4.1.3.2 批次 `Step` 指令接口。两套模板文件不可混用。

详细配置与排错说明见 `docs\VisualStudio与TESSNG环境配置.md`。
逐项算法与控制对齐结果见 `docs\算法一致性与迁移映射.md`。

## 原生执行链

- `ProjectModel`：读取源清单后将相距约 26.4 m 的 J253/J254 合并为一个外边界控制交叉口，最终建立 254 个逻辑交叉口、1036 个组合相位、929 个决策点、22 对 OD 和 2000 辆预载计划的内存索引。
- `TrafficAggregator`：原生聚合排队、速度、占有率、到达率和能耗。
- `SignalEngine`：固定配时/改进最大压力两分支，动态主从租约，双向兼容相位优先，黄灯和全红安全状态机。
- `RoutingEngine`：每车固定终点，22 棵反向最短路径树，在上游决策点动态换路，驶入路口后锁定出口。
- `DemandEngine`：随机种子每轮固定，2000 辆全路网预载，18 个外围端点默认 1000 辆/h，4 个居民区端点默认 100 辆/h。
- `MetricsRecorder`：停止时将本轮 JSON/CSV 写入 `result\round_NNN\算法名`。

## 多轮对照仿真

右侧控制栏的“多轮对照仿真”可设置 1–100 个对照轮次。点击“开始自动对照”后无需手动切换算法：

- 1 个对照轮次依次运行“改进最大压力”和“固定配时”两个子实验。
- 同一轮两个算法共用完全相同的车辆产生种子和目的地选择种子，保证需求与 OD 可比。
- 进入下一轮时自动重新随机生成两项种子。
- 每个子实验停止后立即保存，不在高频仿真回调中执行文件写入。
- 数据写入 `result\round_NNN\IMPROVED_MAX_PRESSURE` 和 `result\round_NNN\FIXED_TIMING`；一对实验完成后同时生成 `result\round_NNN\comparison_manifest.json`。
- “停止自动对照”或“完全终止”会先停止当前子实验；已经完成的数据不会丢失。

结果包含：完成车辆数与完成率、单位小时完成量、平均/P95 行程时间、平均/P95 延误、平均/P95 停车次数、平均排队时间、全网平均/P95/最大排队、回溢进口累计时间、车道排队不均衡、组合相位和并行放行、平均速度、里程、能耗与碳排估算、动态路径决策/切换次数，以及每 5 秒的时序数据。

## 安全约束

灯色只有 `SignalEngine` 一个写入者。同一物理车道包含多个转向时按整条车道原子放行，不再以左转/直行分裂出冲突指令。红/黄灯车辆停在停止线前，绿灯使用同一灯色结果释放，右转专用车道按配置允许常通。
