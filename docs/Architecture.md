# 架构与维护

## 源码职责

| 目录 | 职责 |
| --- | --- |
| `src/Core/` | 状态、像素、几何与版本 |
| `src/Graphics/` | 绘图、文字与 Sprite |
| `src/Display/` | 显示对象、传输契约与刷新调度 |
| `src/Display/Presets/` | 尺寸、接口变体与开发板预设 |
| `src/Drivers/Display/` | 显示芯片初始化、旋转与协议 |
| `src/Drivers/Touch/`、`src/Drivers/IO/` | 触摸和通用外设驱动 |
| `src/Platform/` | 平台总线、内存、DMA 与加速 |
| `src/UI/` | 编译资源、View、图片、输入与导航 |
| `src/Fonts/` | 字模、解码与许可；预览位于 `docs/assets/fonts/` |
| `examples/` | 接线与应用示例 |
| `tests/`、`tools/` | 主机回归与维护检查；测试产物在 `tests/.build/` |

`NICE_GFX.h` 是统一入口，也可直接包含模块头。`src/UI/detail/` 是内部实现，不属于稳定公共接口。

## 驱动与开发板预设

通用驱动按芯片和接口命名，扫描及桥接差异放在 [PanelVariants.h](../src/Display/Presets/PanelVariants.h)。开发板接线、启动策略集中在 [BoardPresets.h](../src/Display/Presets/BoardPresets.h) 和 [BoardRuntime.h](../src/Display/Presets/BoardRuntime.h)，复用通用驱动。

开发板目录由同级 `nice_gfx_editor/` 配置生成。导出时，接线、频率、控制器和 RGB 时序匹配则复用预设，否则生成自定义配置；旋转、触摸、按键和 UI 设置由应用选择。型号与平台限制见 [开发板预设](BoardPresets.md)。

## 编译 UI 与业务代码

主库 `src/` 是固件运行库和字体的源头；编辑器位于同级 `nice_gfx_editor/`。HTML/CSS 导出为 `ui.h`，由 `View` 加载。资源 ABI 以 [Compiled.h](../src/UI/Compiled.h) 的 `ResourceVersion` 为准；变更资源布局时同步编译器、生成资源和运行库。

业务数据、回调与外设操作放在应用代码，通过稳定 ID 更新当前页面。硬件、接口与 ABI 兼容时可只替换 `ui.h`；地图等扩展文件按扩展说明同步。接入见 [组件交互](ArduinoComponentInteraction.md) 和 [动态图片](DynamicImages.md)，完整运行约定见 [UI README](../src/UI/README.md)。

## ESP-IDF 构建

根目录 [CMakeLists.txt](../CMakeLists.txt) 默认使用原生 ESP-IDF。Arduino 作为 IDF 组件时，在注册本库前将 `NICE_GFX_ARDUINO_COMPONENT` 设为实际组件名，例如 `arduino`。

| `NICE_GFX_BUILD_PROFILE` | UI 实现 | 内置字模 |
| --- | --- | --- |
| `full`（默认） | 包含 | 全部 |
| `graphics` | 排除 | 全部 |
| `minimal` | 排除 | 排除 |

`NICE_GFX_ENABLE_UI`、`NICE_GFX_ENABLE_FONTS` 可覆盖默认值。启用字体时，`NICE_GFX_FONT_SOURCES` 留空表示全部，`NONE` 表示排除内置字模，也可填写 `Fixed6x13;GfxSansCN16` 等名称列表；应包含应用实际引用的字体。

这些选项只选择源文件，不改变公共头与 ABI；排除 UI 后不能调用其实现。Arduino 库构建器与编辑器导出的 IDF 组件脚本不读取这些裁剪选项，默认包含完整 UI 和字体。

## 刷新与内存

所有 View API 从同一 UI 任务调用，并持续运行 `pollUi()` 或 `tick()`。默认分块绘制无需整屏帧缓冲；缓存、合成和资源切换会额外占用内存，峰值在目标设备验证。

`configureRenderBudget()` 在无进行中的光栅帧时设置。像素预算须满足屏幕对齐，会限制部分整帧加速；时间预算为软截止，不保证硬实时。`runtimeStats()` 统计工作量，`memoryStats()` 估算保留容量，不含分配器和驱动开销。

构造与挂载须检查返回结果；`View::create()` 和 `mount()` 的内存失败边界见 [View.h](../src/UI/View.h)。关闭异常时，STL 分配失败仍可能终止程序。刷新和资源生命周期见 [UI README](../src/UI/README.md)。

## 维护命令

在主库目录运行，需 Node.js 20.11 或更新版本，无需安装 npm 依赖：

```powershell
npm.cmd run check
npm.cmd run examples:check
npm.cmd test
```

主机测试另需 C++17 编译器和 CMake，可通过 PATH 或 `CXX`、`CMAKE` 指定。文档更新运行 `check`；源码改动按影响运行回归，接线与时序另做实板验证。

| 修改内容 | 同步与检查 |
| --- | --- |
| 版本元数据 | 主库运行 `npm.cmd run metadata:sync` |
| [示例共享模板](../examples/_shared/Hardware.h) | 主库运行 `npm.cmd run examples:sync`，再运行 `npm.cmd run examples:check` |
| 开发板配置 | 编辑器运行 `npm.cmd run boards:library:generate`，再运行 `npm.cmd run boards:library:check` |
| 主库固件源码 | 编辑器运行 `npm.cmd run runtime:rebuild`，再运行 `npm.cmd run runtime:check` |

编辑器运行库同步会更新固件包、清单与导出代码；文档及预览图不进入固件包。
