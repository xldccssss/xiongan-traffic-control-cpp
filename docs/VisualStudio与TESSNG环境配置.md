# Visual Studio 与 TESSNG 4.1.3 C++ 环境配置

## 1. 工程入口

- CMake 入口：`CMakeLists.txt`
- 程序入口：`src/main.cpp`
- 插件入口：`src/MyPlugin.cpp`
- 路网回调：`src/MyNet.cpp`
- 仿真回调：`src/MySimulator.cpp`

程序启动流程与官方 `TESS_CppAPI_EXAMPLE` 保持一致：创建 `QApplication`，调用 `tessng(...)` 创建 TESSNG 主窗口，创建并初始化 `MyPlugin`，再通过 `gpTessInterface->loadPluginFromMem(plugin)` 注册插件。

## 2. 本机路径

| 项目 | 路径 |
| --- | --- |
| Visual Studio 2022 Community | `F:\DevTools\VisualStudio\2022\Community` |
| Qt 5.15.2 MSVC x64 | `F:\Qt\5.15.2\msvc2019_64` |
| TESSNG 4.1.3 | `F:\TESSNG_4.1.3` |
| 官方 QuickStart 结构参考 | `F:\TESSNG_4.1.3\SecondDev\TESS_CppAPI_EXAMPLE` |
| 本项目实际 C++ SDK 母版 | `F:\TESSNG_4.1.3\SecondDev\TESS_CppAPI_BATCH_COMMANDS_EXAMPLE` |

项目的 `include`、`lib\debug`、`lib\release` 来自同一套 TESSNG 4.1.3 批次指令 SDK，不允许混入普通示例的旧头文件或库。普通示例用于参照目录和启动方式；批次示例提供本项目需要的 `Step`、`SimulatorBatchCommands` 与多线程接口。

## 3. Visual Studio 操作

1. 打开 Visual Studio，选择“打开本地文件夹”，打开项目根目录；也可双击 `open-visual-studio.cmd`。
2. 在配置预设中选择 `TESSNG 4.1.3 - x64 Release`。
3. 等待 CMake 配置结束。
4. 在启动项下拉框选择 `雄安路网原生C++仿真`。
5. 按 `F5` 启动调试，或按 `Ctrl+F5` 直接运行。

`CMakeLists.txt` 通过 `VS_STARTUP_PROJECT` 把 `XionganTrafficNativeCpp` 设为生成解决方案的默认启动项目；`.vs/launch.vs.json` 为“打开文件夹/CMake”模式提供同名启动项。

## 4. 环境变量与运行库

项目预设和启动脚本会设置：

- `QT_ROOT` / `QTDIR`：Qt 根目录。
- `TESSNG_ROOT`：TESSNG SDK 根目录。
- `TESSNG_RUNTIME_DIR`：当前配置使用的 TESSNG DLL 目录。
- `QT_PLUGIN_PATH`：Qt 插件目录。
- `PATH`：优先加入 TESSNG 运行库和 Qt `bin`。

Release 默认使用项目内、由批次示例复制的 `runtime`；Debug 默认使用批次示例的 `x64\Debug`，避免普通示例/批次示例或 Debug/Release 文件混用。

## 5. 路网与启动配置

`config.json` 会在构建后复制到 `bin\Debug` 或 `bin\Release`，包含：

- `__netfilepath`：默认雄安路网文件。
- `__simuafterload=false`：加载完成后等待人工点击开始。
- `__enableBatchCommandsMode=true`：保留批次计算能力。

`src/main.cpp` 也会校验路网路径，因此路径失效时会明确提示，不会进入空路网。

## 6. 验证

依次执行：

1. `build-release.cmd`
2. `verify.cmd`
3. `run.cmd`

验证标准：CMake 配置成功、Release 编译成功、`NativeCoreSelfTest` 通过、TESSNG 主窗口能够加载指定路网。软件激活仍由用户在 TESSNG 激活窗口中完成。

## 7. 更换电脑

更换电脑时只修改 `CMakePresets.json` 中以下两项：

- `QT_ROOT`
- `TESSNG_SDK_ROOT`

同时确认 MSVC、Qt 和 TESSNG 均为 64 位，Debug/Release 库没有混用。不要把 Python SDK、旧版 TESSNG 头文件或其他 Qt 编译套件加入本工程。
