# 源码职责与兼容入口

| 目录 | 内容 |
| --- | --- |
| `src/Core/` | 状态、像素、几何、公共算法与版本声明 |
| `src/Display/` | Panel、传输契约、刷新、显示对象组合 |
| `src/Display/Presets/` | 显示尺寸、接口和明确板型的便利预设 |
| `src/Drivers/Display/` | 芯片初始化、旋转、窗口与协议；一个芯片或协议族一个头文件 |
| `src/Drivers/Touch/` | 公共触摸契约、校准，以及独立触摸芯片驱动 |
| `src/Platform/` | Arduino、ESP32、STM32、RP2040 的总线、内存和加速实现 |
| `src/UI/` | 资源 ABI、View、Image、输入、导航与其他公开接口 |
| `src/UI/detail/` | 缓存、绘制、软键盘和其他实现专用文件 |
| `src/Fonts/` | 字体描述、字模、解码与随字模发布的许可 |
| `docs/assets/fonts/` | 字体预览图和预览目录数据，不参与固件编译 |
| `examples/` | 应用接线、复位顺序、板级辅助芯片、业务代码 |
| `tests/`、`tools/` | 可复现主机回归和库维护检查；生成产物位于 `tests/.build/` |

`NICE_GFX.h` 保留便利入口。应用也可以直接包含模块头，以避免引入无关预设。`UI/Compiled.h` 是生成资源与运行库共享的 ABI；改变资源布局时，编辑器编译器和运行库必须一起更新，库语义版本与资源 ABI 分开维护。

触摸公共接口位于 [TouchTypes.h](../src/Drivers/Touch/TouchTypes.h)、[TouchController.h](../src/Drivers/Touch/TouchController.h) 和 [TouchBus.h](../src/Drivers/Touch/TouchBus.h)，不依赖显示传输。SPI 触摸芯片位于 [XPT2046.h](../src/Drivers/Touch/XPT2046.h)。旧 `Touch.h`、`TouchI2c.h`、`ModuleDrivers.h` 和 UI 实现头保留转发入口。

示例自带 `Hardware.h`，用户修改草图内的接线。138 个通用示例的起步配置由 [共享模板](../examples/_shared/Hardware.h) 生成，维护者修改模板后运行 `npm run examples:sync`。旧 `Display/Presets/ExampleBoard.h` 仅用于兼容已安装库的旧草图；编辑器固件运行库包不包含这个示例兼容入口。

板级 IO 扩展器和外围设备由应用管理。例如 [Waveshare4B](../examples/HTML_UI/Waveshare4B/README.md) 在示例内保存 TCA9554 实现，通过 `RgbConfig::commandWrite` / `commandContext` 向显示驱动提供命令传输。PPA、DMA 和 PSRAM 策略属于平台实现，UI 调用平台接口。

图片的复制、收养和引用计数在 [Image.cpp](../src/UI/Image.cpp) 独立编译，资源生命周期不依赖 View 的渲染实现。缓存、动画和输入的内部片段位于 `UI/detail/`，不会成为稳定公共接口。

## 构建与维护

原生 ESP-IDF 默认使用 `CMakeLists.txt`。Arduino 作为 IDF 组件时，在包含本库前显式设置 `NICE_GFX_ARDUINO_COMPONENT` 为实际组件名，例如 `arduino` 或 `espressif__arduino-esp32`。

默认编译所有内置字体。IDF 项目可以在包含本库前设置 `NICE_GFX_FONT_SOURCES` 为字体名列表，例如 `Fixed6x13;GfxSansCN16`，减少参与编译的源文件；列表必须覆盖应用实际引用的字体。`setFont()` API 不变。Arduino 和编辑器完整导出默认继续保留全部内置字库及许可。

维护检查只需 Node.js 20 或更新版本，不需要安装 npm 依赖：

```powershell
npm run check
npm run metadata:sync
npm run examples:check
```

主机测试另外需要 C++17 编译器和 CMake。把它们加入 PATH，或设置 `CXX`、`CMAKE` 为可执行文件路径，然后运行 `npm test`。支持 GCC、Clang 和 Zig；资源描述符还会进行 C++11 初始化兼容检查。测试覆盖组件构建模式、头文件组合、图片所有权、板级命令协议，以及真实 View/TileCanvas 渲染的动画回归。真机接线、时序和 DMA 行为需在对应硬件上继续验证。

编辑器位于独立仓库。主库目录调整后，在编辑器运行 `node scripts/rebuild-runtime.mjs`，重新生成运行库、完整性清单和示例本地 IO 模块；该包只收集固件源码及必要许可，不包含文档预览资源。
