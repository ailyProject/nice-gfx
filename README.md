# NICE_GFX 1.0.0

NICE_GFX 是面向彩色屏幕的 Arduino / ESP-IDF 图形库。显示对象在构造时接收引脚与配置，提供图元、UTF-8 文字、离屏 Sprite、触摸和局部刷新；部分平台使用 DMA 或 PIO 加速传输。原生 ESP-IDF 接入直接使用 IDF 驱动、GPIO、计时和内存接口，无需 Arduino 组件。

应用界面通过配套设计器编译为原生 C++ 资源 `ui.h`。运行库负责组件状态、输入、动画、路由和分块绘制，应用代码负责业务数据、事件回调与硬件操作。

- [快速开始](#快速开始)
- [屏幕与触摸设备支持](docs/DeviceSupport.md)
- [Waveshare ESP32-S3-Touch-LCD-4B：ST7701、GT911与TCA9554板级配置](examples/07_UI/Waveshare4B/README.md)
- [开发平台支持表](#开发平台支持表)
- [字体与 Sprite](#字体与-sprite)
- [设计器界面与设备交互](#设计器界面与设备交互)
- [功能与资源限制](#功能与资源限制)
- [文档与示例](#文档与示例)
- [源码布局与检查](#源码布局与检查)

## 快速开始

示例按功能与学习顺序组织，完整分类见 [示例目录](examples/README.md)。先运行 [Hello_Display](examples/01_Basics/Hello_Display/Hello_Display.ino)，再试 [Basic_Shapes](examples/02_Graphics/Basic_Shapes/Basic_Shapes.ino)、[Sprite_Basics](examples/05_Sprites/Sprite_Basics/Sprite_Basics.ino) 与 [Display_Color_Bars](examples/09_Diagnostics/Display_Color_Bars/Display_Color_Bars.ino)。这些示例从屏幕当前宽高计算布局，接线、旋转和背光在草图内的 Hardware.h 配置；Wio Terminal 也使用同一套入口，模板选择板载 SPI3、旋转 3 与 LCD_BACKLIGHT。完整的 [Wio Field Lab](examples/08_Applications/WioFieldLab/README.md) 提供传感器、SD、红外与异步 Wi-Fi 应用及可导入编辑器的项目。

将 `NICE_GFX` 文件夹放入 Arduino 草图目录的 `libraries/`，安装所用开发板的 Arduino core，再从 IDE 示例菜单打开本库示例。示例接线与额外依赖见 [examples/README.md](examples/README.md)。

下面使用 ESP32 经典款与 SPI 接口 ILI9341。引脚必须与实际接线一致，其他屏幕及触摸类型见 [设备支持](docs/DeviceSupport.md)。

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

`gfx.displayInfo()` 查询控制器、尺寸、引脚和总线信息，`gfx.lastError()` 查询初始化或传输错误，`gfx.asynchronous()` 查询当前显示对象的异步传输能力。

显示初始化使用 `begin()` 或 `beginChecked()`；重复颜色使用 `pushBlock(color, pixelCount)`，像素数组使用 `pushPixels(data, pixelCount)`，批量写入通过 `startWrite()/endWrite()` 管理。`pushPixels` 的长度统一为像素数，主机 RGB565 数组使用 `setSwapBytes(true)`，MSB 在前的字节数组使用 `setSwapBytes(false)`。显示对象的 `init()`、两参数 `pushColor()`、`pushColors()` 和两参数 `writeColor()` 转发入口已移除。Sprite 的 `pushColor(color, pixelCount)` 是独立的画布实现，继续可用。

`initDMA()` 不接收片选参数；片选与传输配置由构造函数选择。`asynchronous()` 查询能力，`initDMA()` 启用支持的异步图片提交，`deInitDMA()` 排空并关闭该模式，`wait()` 等待已接收的传输完成。

ESP-IDF 工程通过原生组件接入本库，使用 C++ `app_main()`；同级独立的 [nice_gfx_idf_examples 1.0.0 示例项目包](../nice_gfx_idf_examples/README.md) 提供 34 个工程，覆盖绘图、字体、Sprite/DMA、UI 及 IDF 任务、定时器、GPIO、NVS 和事件循环。

## 显示芯片与实例化代码

[屏幕与触摸设备支持](docs/DeviceSupport.md) 集中维护全部屏幕芯片、接口、默认分辨率和实例化方法，涵盖 SPI、软件 SPI、8/16 位并口、普通 QSPI、RGB、MIPI 与 ST77903 持续扫描 QSPI。

触摸实现包括 XPT2046、CST816 / FT6x36 / GT911 / GT1151 等 I²C 控制器及四线电阻屏。驱动入口、板卡组合和校准见 [触摸接入](docs/DeviceSupport.md#触摸芯片与使用入口)，接线示例见 [Vendor_Capacitive_Touch](examples/06_Touch/Vendor_Capacitive_Touch/Vendor_Capacitive_Touch.ino)。屏幕分辨率、接口、电源和时序应与实际模组匹配。

## 开发平台支持表

下表说明当前源码提供的后端。具体显示接口还取决于目标芯片外设、Arduino core、可用引脚和内存；选择目标后应编译自己的工程并验证实际屏幕。

| 平台 | 显示总线与加速 | 主要边界 |
| --- | --- | --- |
| ESP32 系列 | SPI / QSPI、软件 SPI、GPIO 8/16 位并口；支持相应后端时使用 DMA | RGB 需要 LCD RGB 外设；MIPI 使用 ESP32-P4；ST77903 扫描需要 SPI SCT 外设 |
| ESP8266 | core SPI、软件 SPI、GPIO 并口 | 同步传输，硬件 SPI 引脚受 core 限制 |
| STM32 | core SPI、软件 SPI、GPIO 并口；部分系列与 SPI 实例提供 DMA | 当前 DMA 映射覆盖 F0/F1/F2/F3/F4/F7/G0/G4/L0/L1/L4；H7/U5 使用同步路径 |
| RP2040 / RP2350，Arduino-Pico core | PIO SPI、原生 SPI DMA、PIO/GPIO 并口、软件 SPI | PIO 与 DMA 资源有限；并口的数据引脚连续性和 GPIO 窗口不满足时回退 GPIO |
| RP2040，Arduino mbed core | core SPI、软件 SPI、GPIO 并口 | 使用 mbed 的 SPI 映射与同步路径 |
| AVR ATmega328P / ATmega2560 | core SPI、软件 SPI、GPIO 并口，直接 PORT 写入优化 | RAM、Flash 与引脚数较少；基础绘图、短文本和小 Sprite 按容量使用 |
| SAMD / Renesas | core SPI、软件 SPI、GPIO 并口 | 通用同步路径；能力按具体 core 和板卡确认 |
| CI13XX | core SPI、软件 SPI、GPIO 并口 | CI1302 core 的 SPI 接口使用软件实现 |
| 其他 Arduino core | 通用 Arduino SPI / GPIO 路径 | 需要兼容的 GPIO、SPI、Print 与计时接口，并单独编译验证 |

DMA 能力与资源分配由实际对象决定，使用 `gfx.asynchronous()` 查询。STM32 的映射与回退逻辑见 [Stm32Spi.h](src/Platform/STM32/Stm32Spi.h)，RP2040/RP2350 后端见 [Rp2040Spi.h](src/Platform/RP2040/Rp2040Spi.h) 和 [Rp2040Pio.h](src/Platform/RP2040/Rp2040Pio.h)。

## 字体与 Sprite

内置 22 套等宽点阵字体，包括 13 套英文和 9 套中文。屏幕与 Sprite 分别通过 `setFont()` 选择字体，`setTextSize(1..7)` 使用整数倍像素缩放。默认不选择字体；未引用的字库可由链接器裁剪。

`drawString()`、`drawNumber()`、`drawFloat()`、`print()/println()` 使用当前字体，字符串采用 UTF-8。完整中文字库需要较多 Flash，AVR 使用短文本子集并检查 PROGMEM 地址范围。字形图样、字符覆盖、后备字体及子集生成见 [字体指南](docs/font.md) 和 [字体维护说明](src/Fonts/README.md)。

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

创建画布后检查返回值；缓冲占用随尺寸和色深增长。接口见 [Graphics/Sprite.h](src/Graphics/Sprite.h)。

## 设计器界面与设备交互

配套 [NICE GFX Studio](../nice_gfx_editor/README.md) 提供页面、组件、主题、路由和硬件配置。HTML/CSS 在导出时编译为 `ui.h`，设备运行原生 `nice_gfx::htmlui::View`。

1. 在设计器选择屏幕、分辨率、旋转、触摸或实体按键，并核对实际引脚。
2. 创建页面与组件，给需要业务访问的元素设置稳定 ID，声明事件动作。
3. 首次导出完整 Arduino 工程，将业务状态与回调写入主程序。
4. 持续调用生成的 `pollUi()`，通过 `setText/setValue/setImage` 等接口更新当前页面。

```cpp
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"

nice_gfx::htmlui::View ui(displayInstance(), kTilePixels);
bool uiReady = false;

void setup() {
  if (!beginDisplayHardware()) return;
  uiReady = ui_config::load(ui) == nice_gfx::Status::Ok;
  // 按 ui.h 中生成的 bind_* 接口绑定业务回调。
}
void loop() {
  if (uiReady) pollUi(ui, millis());
}
```

运行库提供按钮、开关、滑块、输入框、仪表、图表、图片、滚动、分页、焦点与路由。触摸和实体按键进入同一事件系统；应用数据由业务代码保存，再按页面和变化项更新界面。动态图片的复制、所有权交接及预置资源切换见 [动态图片指南](docs/DynamicImages.md)。

输入框和数值步进框使用共用自动键盘：默认全屏，可配置高度，支持浅色 / 深色风格、点击或左右滑动内容移动光标及退格删除。设计器通过 `data-keyboard-theme="light|dark"` 保存字段风格，固件使用 `ui.setKeyboardTheme(nice_gfx::htmlui::KeyboardTheme::Dark)` 切换深色，传入 `KeyboardTheme::Light` 切换浅色；`ui.resetKeyboardTheme()` 恢复字段配置，`ui.keyboardTheme()` 查询有效风格。键盘顶部仅保留收起和密码显隐，输入与提交 API 见 [编译器说明](../nice_gfx_editor/packages/html-ui/README.md#原生输入-api)。


当前资源 ABI 为 17，兼容性以 [UI/Compiled.h](src/UI/Compiled.h) 和生成 `ui.h` 的断言为准。升级时重新导出完整工程，同步运行库及生成适配器，保留已有业务主程序。动效支持有限多属性关键帧、错峰、往返、显隐与完成事件，以及图片和图标的局部缩放/旋转、子树揭示；详见 [原生动效接口](src/UI/README.md#compiled-keyframes-and-local-transforms-abi-17-motion-api-1)。组件 API、回调语义与导航规则见 [Arduino 组件交互](docs/ArduinoComponentInteraction.md)，HTML/CSS 语法见 [编译器支持范围](../nice_gfx_editor/packages/html-ui/HTML_CSS_SUPPORT.md)。

## 功能与资源限制

| 项目 | 使用要求 |
| --- | --- |
| 原生设计器 UI | 需要完整 C++ 标准库和足够的动态内存；AVR 当前仅使用基础显示绘图接口 |
| RGB565 帧缓冲 | 像素内存为宽×高×2 字节，双缓冲加倍；按内部 RAM / PSRAM 预算配置 |
| Sprite、图片与缓存 | 检查分配结果；更新中的旧图片和在途帧可能短暂同时占用内存 |
| 异步传输 | 按接口要求保留输入缓冲，等待 `wait()` 完成后再复用；完成拷贝不等于物理 VSYNC |
| 多任务访问 | 同一显示对象与 View 由应用串行访问；`startWrite()/endWrite()` 是绘制批次 |
| UI 调度 | 持续处理输入与 `tick()/pollUi()`；事件回调记录工作请求，耗时操作由状态机或后台任务执行 |
| 图片与外部资源 | PNG/JPEG 解码、存储、网络和传感器由应用或相应依赖处理 |
| 模组与 SDK | 电源、GPIO、视频时序、色序和读回接线按实际硬件配置；升级 SDK 后重新编译与验收 |

原生 UI 采用分块绘制，缓存按配置使用；刷新吞吐取决于区域大小、总线、内存与页面复杂度。可通过 `runtimeStats()`、`memoryStats()` 和实际显示传输测量评估预算。

## 文档与示例

| 文档或示例 | 内容 |
| --- | --- |
| [设备支持](docs/DeviceSupport.md) | 屏幕与触摸芯片、接口、分辨率、实例化和初始化限制 |
| [字体指南](docs/font.md) | 全部字体图样、覆盖范围、调用方法与中文子集 |
| [Arduino 组件交互](docs/ArduinoComponentInteraction.md) | 稳定 ID、事件、数值与输入、导航、业务联动和调度 |
| [动态图片](docs/DynamicImages.md) | 运行时像素更新、所有权、预置资源和内存契约 |
| [UI 运行库](src/UI/README.md) | 资源生命周期、渲染、输入、路由与动画 |
| [ESP-IDF 示例项目包](../nice_gfx_idf_examples/README.md) | 34 个原生 IDF 工程，含与 Arduino 共用场景的性能基准 |
| [示例目录](examples/README.md) | 接线、字体、触摸和额外依赖 |
| [GfxTouchDemo](examples/07_UI/GfxTouchDemo/README.md) | 可编辑设计器工程、生成资源、应用状态和事件回调 |
| [DynamicImageDemo](examples/07_UI/DynamicImageDemo/README.md) | 动态图片完整 Arduino 示例 |
| [MosaicoCameraDemo](examples/08_Applications/MosaicoCameraDemo/README.md) | ESP-Mosaico V1.2 摄像头实时预览、颜色识别与检测框叠加 |
| [Display_Info](examples/09_Diagnostics/Display_Info) | 查询实际显示对象与总线能力 |
| [Touch_calibrate](examples/06_Touch/Touch_calibrate/Touch_calibrate.ino) | XPT2046 接线与校准 |

## 源码布局与检查

| 路径 | 职责 |
| --- | --- |
| `src/NICE_GFX.h` | 绘图、Sprite、内置字体和显示预设的便捷入口 |
| `src/Core/` | 状态、像素、几何和公共算法工具 |
| `src/Graphics/` | 绘图、文字、Sprite、TileCanvas；`Gfx.h` 为较轻的绘图声明入口 |
| `src/Display/` | 显示对象、公共 Panel、传输抽象、刷新；`Presets/` 保存尺寸、接口及模组预设 |
| `src/Drivers/Display/`、`src/Drivers/Touch/` | 芯片程序与具体驱动；显示芯片的初始化、旋转和类定义在同一文件中维护 |
| `src/Platform/` | Arduino 通用总线，以及 ESP32、STM32、RP2040 的独立平台实现 |
| `src/Fonts/` | 字体声明、解码与 26 个独立字模编译单元 |
| `src/UI/` | 资源结构、View、组件、输入、导航和渲染支持 |
| `docs/`、`examples/` | 使用文档与 Arduino 示例；IDF 示例位于同级 `../nice_gfx_idf_examples/` |

设计器应用与 HTML/CSS 编译器保存在独立的 `nice_gfx_editor/` 仓库。主库的维护脚本位于 `tools/`，正式主机测试位于 `tests/`；设备固件使用主库源码和生成资源。目录职责、兼容入口、板级配置和构建方式见 [架构说明](docs/Architecture.md)。

修改芯片程序与窗口对齐时编辑 `src/Drivers/Display/` 中对应的文件；修改显示组合和类型别名时编辑 `src/Display/`。Arduino 与 ESP-IDF 共用图形、字体和 UI 源码；原生 IDF 选择 ESP32 平台源文件，跳过 Arduino、RP2040 和 STM32 平台实现。显示目录与能力表由维护工具更新。

Arduino 草图先包含 `<NICE_GFX.h>`，使构建工具发现已安装的库；设计器资源包含 `<UI/View.h>` 和所用字体的声明。模块头是正式接口：`<Graphics/Gfx.h>` 声明绘图对象，`<Graphics/Sprite.h>` 声明 Sprite，`<Fonts/Builtin.h>` 声明内置字体，`<Display/Presets/All.h>` 提供显示预设。普通 C++ 和 ESP-IDF 可以直接按需要组合模块头，绘图与 UI 声明不引入具体显示芯片或 MCU 后端。

在本库根目录检查发布信息、文档链接、示例配置与模块依赖：

```powershell
npm run check
# 需要 PATH 中的 C++17 编译器和 CMake，或设置 CXX / CMAKE：
npm test
# 更新版本号、资源 ABI 说明和动效链接后再检查：
npm run metadata:sync
```

主机回归直接使用库内固定资源，不依赖设计器仓库；帧生命周期、原生/分块渲染对比与内存分配失败也纳入同一入口。配置说明见 [主机测试](tests/README.md)。

实际工程需验证颜色、坐标、输入、首帧与持续运行。

字体、图标和随库代码的许可与声明见 [字体许可目录](src/Fonts/licenses)、[图标许可](src/Graphics/LICENSE-Font-Awesome.txt)、[绘图许可](src/Graphics/LICENSE-Bodmer.txt) 和 [其他许可文件](LICENSES)。
