# 架构与维护

## 源码职责

| 目录 | 职责 |
| --- | --- |
| `src/Core/` | 状态、像素、几何与版本 |
| `src/Graphics/` | 绘图、文字与 Sprite |
| `src/Display/` | 显示对象、传输契约与刷新调度 |
| `src/Display/Presets/` | 尺寸、接口变体与开发板快捷对象 |
| `src/Drivers/Display/` | 显示芯片初始化、旋转、窗口与协议 |
| `src/Drivers/Touch/` | 触摸契约、校准与芯片驱动 |
| `src/Drivers/IO/` | TCA9554 等通用外设协议 |
| `src/Platform/` | 平台总线、内存、DMA 与加速 |
| `src/UI/` | 编译资源、View、Image、输入与导航 |
| `src/UI/detail/` | 绘制、缓存和软键盘等内部实现 |
| `src/Fonts/` | 字模、解码与许可；预览图位于 `docs/assets/fonts/` |
| `examples/` | 接线示例、应用逻辑与设备演示 |
| `tests/`、`tools/` | 主机回归与维护检查；产物位于 `tests/.build/` |

`NICE_GFX.h` 是便利入口，也可直接包含所需模块头。旧触摸、模组及部分 UI 头保留转发入口；`UI/detail/` 不属于稳定公共接口。

## 驱动与开发板预设

底层文件和类型按芯片、接口、尺寸或扫描方向命名。芯片程序放在 `Drivers/Display/`，扫描与桥接差异放在 [PanelVariants.h](../src/Display/Presets/PanelVariants.h)，例如 `ILI9341_Transposed`、`ILI9486_PaddedSPI`。

品牌名称可用于开发板快捷类、别名和宏。接线与启动策略集中在 [BoardPresets.h](../src/Display/Presets/BoardPresets.h) 和 [BoardRuntime.h](../src/Display/Presets/BoardRuntime.h)，复用通用驱动；SDK 宏及第三方署名保持原文。完整目录和平台限制见 [开发板预设](BoardPresets.md)。

编辑器配置生成库内目录及预设。导出时，显示接线、频率、控制器和 RGB 时序匹配库预设便直接复用；修改这些参数后使用显式自定义配置。旋转、触摸、按键和 UI 设置仍由应用选择。共享外设使用同一实例：例如板卡预设与应用共用 [TCA9554](../src/Drivers/IO/TCA9554.h)，触摸通过公共 `I2cBus` 接入。

## 编译 UI 与业务代码

编辑器位于同级 `nice_gfx_editor/`，主库 `src/` 是固件运行库和字体的源头。HTML/CSS 在导出时编译为 `ui.h`，设备由 `View` 加载资源。资源 ABI 以 [Compiled.h](../src/UI/Compiled.h) 的 `ResourceVersion` 为准，与库语义版本分别维护；变更资源布局时同步编译器、生成资源和运行库。

业务数据、回调和外设操作放在应用代码中，通过稳定 ID 更新当前页面。重新导出时保留业务源码；只改界面且硬件、接口与 ABI 兼容时，可替换 `ui.h`。地图等扩展另有生成文件，应按扩展说明同步。接入方法见 [组件交互](ArduinoComponentInteraction.md) 和 [动态图片](DynamicImages.md)。

图片所有权由 [Image.cpp](../src/UI/Image.cpp) 管理，在途帧保留旧资源引用。布局和组件索引按帧复用，可通过 `runtimeStats()` 查看实际工作量。平台渲染能力由 [RenderBackend.h](../src/Platform/Common/RenderBackend.h) 选择，硬件能力不满足时保留软件路径。

## ESP-IDF 构建

库的 `CMakeLists.txt` 默认使用原生 ESP-IDF。Arduino 作为 IDF 组件时，在注册本库前设置 `NICE_GFX_ARDUINO_COMPONENT` 为实际组件名，例如 `arduino`。

| `NICE_GFX_BUILD_PROFILE` | UI 实现 | 内置字模 |
| --- | --- | --- |
| `full`（默认） | 包含 | 全部 |
| `graphics` | 排除 | 全部 |
| `minimal` | 排除 | 排除 |

`NICE_GFX_ENABLE_UI`、`NICE_GFX_ENABLE_FONTS` 可覆盖对应默认值。`NICE_GFX_FONT_SOURCES` 为空表示全部，为 `NONE` 表示不编译内置字模，也可填写 `Fixed6x13;GfxSansCN16` 等名称列表。字体列表要求启用字体模块，并覆盖应用实际引用的字体；自定义字体仍通过 `setFont()` 使用。

这些选项只选择源文件，不改变公共头和 ABI。排除 UI 后不能调用其运行库实现。Arduino 库构建器不读取这些选项；编辑器另行生成的 IDF 组件脚本目前也不接收这些裁剪开关，默认包含完整 UI 和字体。

## 刷新与内存

所有 View API 从同一个 UI 任务调用；持续运行 `pollUi()` 或 `tick()`。默认分块绘制无需整屏帧缓冲，缓存和原生合成路径会额外占用内存。

| 接口 | 用途与边界 |
| --- | --- |
| `configureMergeSlack(pixels)` | 默认 32，表示为减少一次窗口提交允许额外重绘的像素；刷新中不能修改，减小后不能拆回已合并区域 |
| `configureRenderBudget(budget)` | 无进行中的刷新时设置预算；默认两个字段均为 0 |
| `budget.maxPixelsPerTick` | 正数限制可分块的光栅工作，必须满足屏幕对齐；改走有界 tile 路径，减少部分整帧加速 |
| `budget.maxMicrosPerTick` | 合作式软截止时间；首次布局、不可分步操作和传输仍可能超时 |
| `memoryStats()` | 按容量估算 View 保留存储，包含外借 tile；不等于设备峰值内存 |
| `View::create()` | 检查初始化结果；失败保留输出指针，启用异常时可把构造的 `bad_alloc` 转为 `NoMemory` |
| `mount()` | 新初始页面准备成功后才替换旧状态；准备失败保留旧项目，但准备期会增加临时内存 |

直接使用 `RefreshScheduler` 时，对应接口为 `configureMergeSlack()` 和 `configurePixelBudget()`。预算不改变 DMA 排空或缓冲区借用契约，也不提供硬实时保证。调整时比较提交次数、帧耗时和输入延迟。

驱动扫描缓冲、分配器开销、临时共存的新旧资源需另外计算。例如 800×480 RGB565 单张为 750 KiB。关闭异常时，STL 分配失败仍可能终止程序，任意 setter 尚无统一的内存失败返回契约；应在目标设备验证峰值。

## 维护命令

在主库目录运行；检查只需 Node.js 20.11 或更新版本，无需安装 npm 依赖：

```powershell
npm.cmd run check
npm.cmd run examples:check
npm.cmd test
```

主机测试另需 C++17 编译器和 CMake，可通过 PATH 或 `CXX`、`CMAKE` 指定。支持 GCC、Clang、Zig，并检查资源描述符的 C++11 初始化兼容性。文档更新运行 `check` 即可；源码改动按影响运行回归，硬件接线与时序另做实板验证。

修改版本元数据后运行 `npm.cmd run metadata:sync`。修改 [示例共享模板](../examples/_shared/Hardware.h) 后运行 `npm.cmd run examples:sync`；分类见 [示例目录](../examples/README.md)。旧 `ExampleBoard.h` 仅作兼容，不进入编辑器固件包。

修改板卡配置后，在编辑器目录运行 `npm.cmd run boards:library:generate`，再用 `npm.cmd run boards:library:check` 核对。修改主库固件源码后，在编辑器运行 `npm.cmd run runtime:rebuild` 与 `npm.cmd run runtime:check`，同步运行库包、清单及导出代码；`node scripts/rebuild-runtime.mjs --check` 仅检查。文档与预览图不进入固件包。
