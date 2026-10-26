# 架构与构建

## 源码职责

| 目录 | 职责 |
| --- | --- |
| `src/Core/` | 状态、像素、几何与版本 |
| `src/Graphics/` | 绘图、文字、Sprite 与 TileCanvas |
| `src/Display/` | 显示对象、后端、传输契约与刷新调度 |
| `src/Display/Presets/` | 屏幕型号、接口变体与开发板预设 |
| `src/Drivers/` | 显示、触摸及外设协议；随附驱动许可 |
| `src/Platform/` | 平台总线、内存、DMA 与加速 |
| `src/UI/` | 编译资源、组件、输入、导航与渲染 |
| `src/Fonts/` | 字模、解码与许可 |
| `examples/` | Arduino 接线与应用示例 |
| `tests/hardware/` | 硬件编译夹具 |

Arduino 使用 `<NICE_GFX.h>` 统一入口；也可按需包含 `<Graphics/Gfx.h>`、`<Graphics/Sprite.h>`、`<Fonts/Builtin.h>` 和显示预设头。`src/UI/detail/` 属于内部实现。

## 驱动与开发板

芯片协议位于 `Drivers/Display/`，接口组合与尺寸位于 `Display/`。扫描及桥接变体见 [PanelVariants.h](../src/Display/Presets/PanelVariants.h)，固定接线与启动顺序见 [BoardPresets.h](../src/Display/Presets/BoardPresets.h) 和 [BoardRuntime.h](../src/Display/Presets/BoardRuntime.h)。

U8g2、GxEPD2 和 Seeed 兼容路径使用随库固定的底层协议，应用仍使用 NICE_GFX 绘图与 UI。构建应编译 `src/` 下的 `.c` 和 `.cpp`，保留各驱动目录中的许可与来源清单。

## UI 与业务代码

配套 `nice_gfx_editor/` 将 HTML/CSS 编译为 `ui.h`，设备通过 `View` 加载资源。应用保存业务数据、绑定回调和操作外设，通过稳定 ID 更新页面。接入见 [组件交互](ArduinoComponentInteraction.md)。

资源 ABI 以 [Compiled.h](../src/UI/Compiled.h) 的 `ResourceVersion` 为准。升级后重新导出完整工程，同步运行库与适配器；硬件和 ABI 均兼容时可只替换 `ui.h`。

## ESP-IDF 构建

根目录 [CMakeLists.txt](../CMakeLists.txt) 默认使用原生 ESP-IDF。Arduino 作为 IDF 组件时，在注册本库前将 `NICE_GFX_ARDUINO_COMPONENT` 设为实际组件名，例如 `arduino`。

| `NICE_GFX_BUILD_PROFILE` | UI 实现 | 内置字模 |
| --- | --- | --- |
| `full`（默认） | 包含 | 全部 |
| `graphics` | 排除 | 全部 |
| `minimal` | 排除 | 排除 |

`NICE_GFX_ENABLE_UI`、`NICE_GFX_ENABLE_FONTS` 可覆盖默认值。启用字体时，`NICE_GFX_FONT_SOURCES` 留空表示全部，`NONE` 排除内置字模，也可填写 `Fixed6x13;GfxSansCN16` 等名称列表。

这些选项只选择源文件；排除的实现不能调用。Arduino 库构建器与编辑器导出的 IDF 组件脚本有各自的源文件配置。完整工程见 [原生 IDF 示例](../../nice_gfx_idf_examples/README.md)。

## 刷新与内存

所有 View API 从同一 UI 任务调用，持续运行 `pollUi()` 或 `tick()`。默认分块绘制；缓存、合成和资源切换另占内存，预算与统计见 [渲染性能](RenderingPerformance.md)。

检查构造、挂载和分配结果。`View::create()` 与 `mount()` 的失败约定见 [View.h](../src/UI/View.h)；关闭异常时，部分 STL 分配失败仍可能终止程序。刷新和资源生命周期见 [UI 运行库](../src/UI/README.md)。

## 配置维护

开发板目录由同级编辑器的 `configs/boards/*.json` 生成，[BoardPresets.md](BoardPresets.md) 和 [BoardPresets.json](BoardPresets.json) 应与库预设头保持一致。修改源配置后，在编辑器运行 `boards:library:generate` 和 `boards:library:check`；运行库更新后运行 `runtime:rebuild` 和 `runtime:check`，具体命令以编辑器 `package.json` 为准。

本库当前未随附 `tools/` 和主机测试运行脚本，根目录 `package.json` 中对应的 npm 入口不能直接运行。编译验证请使用 [Arduino 示例](../examples/README.md)、硬件夹具或原生 IDF 工程，并在目标屏幕检查颜色、坐标、输入与刷新。