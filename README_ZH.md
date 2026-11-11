<p align="center">
  <img src="logo.png" alt="nice gfx logo" width="480">
</p>

[English](README.md) | [简体中文](README_ZH.md)

# NICE_GFX

NICE_GFX 是支持彩色屏、单色显示和电子纸的 Arduino / ESP-IDF 图形库。显示对象在构造时接收引脚与配置，提供图元、UTF-8 文字、离屏 Sprite、触摸和局部刷新；部分平台使用 DMA 或 PIO 加速传输。

配套 NICE GFX Studio 将 HTML/CSS 界面编译为原生 C++ 资源 `ui.h`。运行库负责组件、输入、动画、路由和分块绘制，应用代码负责数据、回调与硬件操作。

- [快速开始](#快速开始)
- [显示设备与平台](#显示设备与平台)
- [字体与 Sprite](#字体与-sprite)
- [设计器界面](#设计器界面)
- [资源要求](#资源要求)
- [文档与示例](#文档与示例)
- [源码布局与检查](#源码布局与检查)
- [协议与使用声明](#协议与使用声明)

## 快速开始

### Arduino

1. 将 `NICE_GFX` 文件夹放入 Arduino 草图目录的 `libraries/`，安装所用开发板的 Arduino core。
2. 从 IDE 示例菜单打开 [Hello_Display](examples/01_Basics/Hello_Display/Hello_Display.ino)。
3. 修改草图内的 `Hardware.h`，核对显示类型、引脚、旋转和背光，再编译上传。

接着可运行 [Basic_Shapes](examples/02_Graphics/Basic_Shapes/Basic_Shapes.ino)、[Display_Color_Bars](examples/09_Diagnostics/Display_Color_Bars/Display_Color_Bars.ino) 与 [Sprite_Basics](examples/05_Sprites/Sprite_Basics/Sprite_Basics.ino)。接线与额外依赖见 [示例说明](examples/README.md)，全部草图见 [示例索引](examples/CATALOG.md)。

下面的最小示例使用 ESP32 经典款与 SPI 接口 ILI9341，引脚须按实际接线修改：

```cpp
#include <NICE_GFX.h>

// CS、DC、RST、SCK、MOSI、MISO
ILI9341_240x320_SPI gfx(5, 2, 4, 18, 23, 19);

void setup() {
  if (!gfx.begin()) return;
  gfx.fillScreen(GFX_BLACK);
  gfx.setFont(nice_gfx::fonts::Fixed6x13);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.drawString("Hello NICE_GFX", 8, 8);
}

void loop() {}
```

`gfx.displayInfo()` 查询控制器、尺寸、引脚和总线，`gfx.lastError()` 查询初始化或传输错误，`gfx.asynchronous()` 查询当前对象的异步传输能力。在支持的后端上，`initDMA()` 启用异步图片提交，`wait()` 等待已接收的传输完成，`deInitDMA()` 排空并关闭该模式。

### ESP-IDF

将本库放入工程的 `components/nice_gfx/`，或通过 `EXTRA_COMPONENT_DIRS` 指向本库目录。使用 C++ 源文件和 `extern "C" void app_main()` 入口。[组件 CMake 文件](CMakeLists.txt) 选择原生 ESP32 后端与 C++17，直接使用 IDF 驱动、GPIO、计时和内存接口。

默认 `full` 构建配置包含绘图、内置字体和 UI。`NICE_GFX_BUILD_PROFILE` 还可选择 `graphics` 和 `minimal`；字体与 UI 的源文件选择同样在该 CMake 文件中配置。

## 显示设备与平台

根据控制器、分辨率、接口和模组接线选择预设：

| 显示类型 | 配置入口 |
| --- | --- |
| 彩色 SPI、软件 SPI、QSPI 与并口屏 | [显示别名](src/Display/Presets/DisplayAliases.h) 与 [控制器驱动](src/Drivers/Display) |
| RGB、MIPI 与持续扫描 QSPI 屏 | [视频显示别名](src/Display/Presets/VideoAliases.h) |
| 单色屏与 u8g2 配置 | [u8g2 预设](src/Display/Presets/U8g2.h) 与 [设备目录](src/Display/Presets/u8g2-displays.json) |
| 原生电子纸 | [电子纸预设](src/Display/Presets/NativeEpaper.h) 与 [Seeed 产品状态](src/Display/Presets/seeed-epaper-displays.json) |
| 集成开发板 | [板级预设](src/Display/Presets/BoardPresets.h) |

触摸驱动包括 XPT2046、CST816、FT6x36、GT911、GT1151 及四线电阻屏。入口见 [触摸驱动目录](src/Drivers/Touch)、[电容触摸示例](examples/06_Touch/Vendor_Capacitive_Touch/Vendor_Capacitive_Touch.ino) 和 [XPT2046 校准示例](examples/06_Touch/Touch_calibrate/Touch_calibrate.ino)。

| 平台 | 可用后端 |
| --- | --- |
| ESP32 系列 | SPI / QSPI、软件 SPI、GPIO 并口及相应后端的 DMA；RGB、MIPI 与持续扫描能力取决于目标芯片外设 |
| ESP8266 | core SPI、软件 SPI、GPIO 并口，同步传输 |
| STM32 | core SPI、软件 SPI、GPIO 并口，部分系列与 SPI 实例提供 DMA |
| RP2040 / RP2350，Arduino-Pico core | PIO SPI、原生 SPI DMA、PIO/GPIO 并口、软件 SPI |
| RP2040，Arduino mbed core | core SPI、软件 SPI、GPIO 并口，同步传输 |
| AVR、SAMD、Renesas | Arduino SPI / GPIO 路径，受 core、可用引脚和内存限制 |
| CI13XX / CI130X | I2C、core 软件 SPI、GPIO 并口，见 [平台说明](src/Platform/CI13XX/README.md) |
| 其他 Arduino core | 通用 SPI / GPIO 路径，需针对所选 core 编译验证 |

屏幕电源、时序、色序和引脚应与实际模组匹配。电子纸应先查看产品目录中的协议实现与实屏验证状态。[刷新示例](examples/09_Diagnostics/EpaperRefresh/README.md) 展示如何通过 `pollDisplay()` 推进刷新；`wait()` 完成传输不代表面板的光学刷新已经结束。

## 字体与 Sprite

内置字体包含点阵、灰度、中文、拉丁文及数字专用字体。主头文件提供的字体见 [字体声明](src/Fonts/Builtin.h)，使用方法见 [字体说明](src/Fonts/README.md) 和 [文字示例](examples/03_Text/README.md)。

屏幕与各个 Sprite 分别通过 `setFont()` 选择字体。文字采用 UTF-8，`setTextSize(1..7)` 使用整数倍缩放。默认不选择字体；未引用字模可由链接器裁剪，中文子集可减少 Flash 占用。

`GFX_Sprite` 提供 1/4/8/16 位离屏画布。下面片段用于已经初始化的显示对象 `gfx`：

```cpp
GFX_Sprite sprite(&gfx);
sprite.setColorDepth(16);
if (sprite.createSprite(120, 48)) {
  sprite.fillSprite(GFX_BLACK);
  sprite.setFont(nice_gfx::fonts::Fixed6x13);
  sprite.setTextColor(GFX_WHITE, GFX_BLACK);
  sprite.drawString("Ready", 4, 4);
  sprite.pushSprite(8, 8);
  gfx.wait();
}
```

检查内存分配结果，并根据画布尺寸和色深预留空间。接口见 [Graphics/Sprite.h](src/Graphics/Sprite.h)。

## 设计器界面

NICE GFX Studio 可导出页面、组件、主题、路由和硬件配置，生成的 `ui.h` 运行于原生 `nice_gfx::htmlui::View`。

1. 在设计器配置屏幕、分辨率、旋转以及触摸或实体按键。
2. 创建页面与组件，给需要业务访问的元素设置稳定 ID，声明事件动作。
3. 导出完整工程，添加应用状态和事件回调。
4. 持续调用生成的 `pollUi()`，通过 `setText()`、`setValue()`、`setImage()` 等方法更新当前页面。

运行库支持按钮、滑块、输入框、自动键盘、仪表、图表、图片、滚动、焦点、导航和动画。触摸与实体按键使用同一事件系统，业务数据由应用代码保存并在切换页面后继续使用。

生成资源、适配器与运行库须保持兼容。资源版本由 [UI/Compiled.h](src/UI/Compiled.h) 中的 `ResourceVersion` 定义，并由生成头文件检查。完整接入方式见 [GfxTouchDemo](examples/07_UI/GfxTouchDemo/README.md)，渲染、输入、资源所有权与动效 API 见 [UI 运行库说明](src/UI/README.md)。

## 资源要求

| 资源 | 使用要求 |
| --- | --- |
| 原生设计器 UI | 需要完整 C++ 标准库和足够的动态内存；AVR 使用基础绘图接口 |
| RGB565 帧缓冲 | 像素存储为宽 × 高 × 2 字节，双缓冲加倍 |
| Sprite、图片与缓存 | 检查分配结果，并预留更新时旧图片与在途帧同时占用的内存 |
| 异步传输 | 按接口约定保留缓冲，等待传输完成后再复用 |
| 多任务访问 | 同一显示对象与 View 由应用串行访问；`startWrite()/endWrite()` 用于组织绘制批次 |
| UI 调度 | 持续处理输入与 `tick()/pollUi()`，耗时工作交由状态机或后台任务 |
| 外部资源 | 按所选示例安装解码、存储、网络和传感器等依赖 |

UI 采用分块绘制和可配置缓存。可通过 `runtimeStats()`、`memoryStats()` 与实际传输测量评估页面的内存和刷新预算。

## 文档与示例

| 入口 | 内容 |
| --- | --- |
| [示例说明](examples/README.md) 与 [索引](examples/CATALOG.md) | 草图选择、接线与依赖 |
| [字体说明](src/Fonts/README.md) | 字体选择、字符覆盖与子集 |
| [UI 运行库](src/UI/README.md) | 资源、渲染、输入、导航与动画 |
| [GfxTouchDemo](examples/07_UI/GfxTouchDemo/README.md) | 设计器工程、生成资源与事件回调 |
| [DynamicImageDemo](examples/07_UI/DynamicImageDemo/README.md) | 运行时图片更新与所有权 |
| [MonochromeMenu](examples/07_UI/MonochromeMenu/README.md) | 单色绘图与按键菜单 |
| [EpaperRefresh](examples/09_Diagnostics/EpaperRefresh/README.md) | 电子纸初始化与刷新调度 |
| [Waveshare4B](examples/07_UI/Waveshare4B/README.md) | ST7701、GT911 与 TCA9554 板级配置 |
| [WioFieldLab](examples/08_Applications/WioFieldLab/README.md) | Wio Terminal 传感器、SD、红外与 Wi-Fi 应用 |
| [MosaicoCameraDemo](examples/08_Applications/MosaicoCameraDemo/README.md) | ESP-Mosaico 摄像头实时预览与叠加绘制 |
| [Graphics_Benchmark](examples/10_Performance/Graphics_Benchmark/README.md) | 绘图与传输性能测试 |

## 源码布局与检查

| 路径 | 职责 |
| --- | --- |
| `src/NICE_GFX.h` | 图形、Sprite、字体、触摸与显示预设的便捷入口 |
| `src/Core/`、`src/Graphics/` | 公共类型、绘图、文字、Sprite 与分块画布 |
| `src/Display/` | 显示对象、面板、传输与刷新抽象、预设 |
| `src/Drivers/` | 显示与触摸控制器实现 |
| `src/Platform/` | Arduino 及各目标平台的总线实现 |
| `src/Fonts/` | 字体声明、解码、字模与许可 |
| `src/UI/` | 编译资源、View、组件、输入、路由与渲染 |
| `examples/` | Arduino 草图 |

Arduino 草图应先包含 `<NICE_GFX.h>`，再包含生成头文件，以便构建工具发现已安装的库。C++ 与 ESP-IDF 工程可直接使用 `<Graphics/Gfx.h>`、`<Graphics/Sprite.h>`、`<Fonts/Builtin.h>`、`<UI/View.h>` 等模块头。


## 协议与使用声明

NICE_GFX 的自有代码采用 [Apache License 2.0](LICENSE)，可免费用于商业及非商业项目，允许修改、集成到闭源应用并分发，须遵守该协议。使用运行库无需支付库版税，也无需购买 NICE GFX Studio。

分发相应源码、固件或衍生作品时，应按 Apache-2.0 第 4 条随附许可，保留适用的版权及修改声明，并保留 [NOTICE](NOTICE) 中适用的归属信息。可按协议放在随附 NOTICE、源码／文档或适当的法律声明界面中。欢迎在官网或产品介绍中注明 **“本产品使用 nice gfx 图形库”** 并附 [项目链接](https://github.com/ailyProject/nice-gfx)；官网推广、芯片厂第一推荐和逐项商用审批均不是取得许可的条件。

NICE GFX Studio 及 AI 设计服务是另行授权的收费产品。运行库使用权与设计器订阅相互独立；订阅结束后，已导出的应用仍可继续运行和商业分发。其中复制的运行库、字体、图标和其他第三方材料继续遵守各自许可。

顶层 Apache 许可不替换第三方代码和素材的原许可。详见 [第三方声明](THIRD_PARTY_NOTICES.md)、[许可索引](LICENSES/README.md)、[字体许可](src/Fonts/licenses)、[图标许可](src/Graphics/LICENSE-Font-Awesome.txt) 和 [绘图许可](src/Graphics/LICENSE-Bodmer.txt)。部分历史示例和图片另有许可或权利尚待核实，已在第三方声明中列明，不因本次 Apache 授权自动取得额外权利。
