# Arduino 示例程序

示例按“要学习的功能”组织，编号表示建议学习顺序。开发板、屏幕控制器、分辨率和引脚属于硬件配置，不作为示例分类。所有草图的主文件与所在文件夹同名，可从 Arduino IDE 的 NICE_GFX 示例菜单打开。

| 目录 | 内容 | 建议从这里开始 |
| --- | --- | --- |
| `01_Basics` | 初始化、字体选择、第一行文字 | [Hello_Display](01_Basics/Hello_Display/Hello_Display.ino) |
| `02_Graphics` | 图元、渐变、抗锯齿、裁剪与视口 | [Basic_Shapes](02_Graphics/Basic_Shapes/Basic_Shapes.ino) |
| `03_Text` | 字体、UTF-8、对齐、数值与文本 Sprite | [Monospace_Fonts](03_Text/Monospace_Fonts/Monospace_Fonts.ino) |
| `04_Images` | 位图、JPEG、PNG、Flash 与文件系统 | [GFX_Flash_Bitmap](04_Images/GFX_Flash_Bitmap/GFX_Flash_Bitmap.ino) |
| `05_Sprites` | 离屏绘制、色深、滚动、旋转与透明合成 | [Sprite_Basics](05_Sprites/Sprite_Basics/Sprite_Basics.ino) |
| `06_Touch` | 触摸接线、校准、采样和按钮交互 | [Touch_calibrate](06_Touch/Touch_calibrate/Touch_calibrate.ino) |
| `07_UI` | 编译型界面、组件事件、动画与动态图像 | [GfxTouchDemo](07_UI/GfxTouchDemo/README.md) |
| `08_Applications` | 时钟、仪表、图表、游戏、传感器与相机应用 | [WioFieldLab](08_Applications/WioFieldLab/README.md) |
| `09_Diagnostics` | 显示信息、色序、读写与触摸诊断 | [Display_Color_Bars](09_Diagnostics/Display_Color_Bars/Display_Color_Bars.ino) |
| `10_Performance` | 绘图、DMA、传输与 UI 性能测试 | [Graphics_Benchmark](10_Performance/Graphics_Benchmark/README.md) |

[查看全部示例](CATALOG.md)。

## 运行通用示例

1. 安装 NICE_GFX 和所用开发板的 Arduino core。
2. 打开 Hello_Display，修改草图内的 `Hardware.h`，核对显示类型、构造函数引脚、默认旋转与背光。
3. 在 IDE 中选择实际开发板，编译并上传；串口使用 115200 baud。
4. 依次运行 Basic_Shapes、Display_Color_Bars 和 Sprite_Basics，检查图形、色序和离屏绘制。

从工作区根目录编译，例如：

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32s3 --library nice_gfx nice_gfx/examples/01_Basics/Hello_Display
```

这些入门示例使用 `ExampleDisplay gfx;`，布局从旋转后的 `gfx.width()`、`gfx.height()` 计算。Hello_Display 建议至少 64×64；Sprite_Basics 需要至少 64×32，使用不超过 96×32 的 RGB565 Sprite，像素缓冲最多约 6 KiB，另有对象与分配开销。

Hello_Display、Basic_Shapes 和 Display_Color_Bars 的 `ExampleDrawing.h` 将图形画入两行 RGB565 Sprite 后提交，像素缓冲约为 `屏幕宽度 × 4` 字节；无需全屏缓冲，也能输出到要求 2×2 窗口对齐的 CH13613。绘图回调会按条带重复执行，状态更新应放在回调外。

## 硬件配置

每个通用草图自带可编辑的 `Hardware.h`。硬件预设的类型名可以包含控制器、接口与尺寸，例如 `ILI9341_240x320_SPI`；它们描述设备能力。新增示例应放入对应功能目录，硬件差异在草图配置中处理。

模板提供以下**示例接线**，须按实际硬件修改：

| 目标 | 显示器 | 示例连接 |
| --- | --- | --- |
| Wio Terminal | 板载 ILI9341 / SPI3 | 无需外接；通用入口默认旋转 3，打开 LCD_BACKLIGHT |
| ESP32-S3 | CH13613 QSPI | CS 45、reset 7、clock 17、D0…D3 为 8/16/15/18 |
| 经典 ESP32 | ILI9341 SPI | CS 5、DC 2、reset 4、clock 18、MOSI 23、MISO 19 |
| 其他 ESP32 目标 | ILI9341 SPI | CS 7、DC 2、reset 3、clock 4、MOSI 6、MISO 5 |
| ESP8266 | ILI9341 SPI | CS 15、DC 0、reset 2、clock 14、MOSI 13、MISO 12 |
| RP2040 | ILI9341 SPI | CS 17、DC 20、reset 21、clock 18、MOSI 19、MISO 16 |
| 其他 Arduino core | ILI9341 SPI | CS 10、DC 9、reset 8，使用 variant SPI 引脚 |

通用入口的旋转由 `exampleRotation()` 设置，背光引脚由 `exampleBacklightPin()` 设置，均在本地 Hardware.h 内。其他已有示例可能在 setup 中自行设置旋转，修改时以该草图为准。

触摸示例的 `exampleAttachTouch(gfx)` 构造独立的 XPT2046 控制器。ESP32-S3 示例连接为 clock 10、MOSI 11、MISO 12、CS 13，表示外接 XPT2046；CH13613 模块不因此具备触摸。Wio Terminal 没有板载触摸，模板返回 Unsupported。其他触摸芯片参见 [电容触摸示例](06_Touch/Vendor_Capacitive_Touch/Vendor_Capacitive_Touch.ino) 和 [设备支持说明](../docs/DeviceSupport.md)。

## 既有示例与依赖

已保留原有草图及其图片、字体资源和辅助源码。重复功能的历史布局使用 `Compact`、`Wide` 等后缀区分，并与同功能示例放在一起。这些旧草图仍可能使用固定坐标，迁移目录不代表已改成自适应布局；运行前检查草图注释、旋转、坐标范围和缓冲内存。图库中的复杂眼睛动画也有专用接线。

字体通过 `gfx.setFont(nice_gfx::fonts::Fixed6x13)` 显式选择，Sprite 也须单独设置字体。声明已由 `NICE_GFX.h` 提供；未引用字模在链接时移除。字体示例中的 FLASH_Array、LittleFS、SPIFFS 是历史变体名称，现在均使用 Flash 中的 MonoFont；SD_Card 示例只从卡中读取文本。详见 [字体示例说明](03_Text/README.md) 与 [字体指南](../src/Fonts/README.md)。

JPEG/PNG 解码、SD/文件系统、Wi-Fi 和传感器等依赖按各草图安装。面板或外设专用示例保留明确名称与硬件要求，例如 [CH13613 QSPI](02_Graphics/CH13613_QSPI_Demo/CH13613_QSPI_Demo.ino)、[Waveshare4B](07_UI/Waveshare4B/README.md)、[MosaicoCameraDemo](08_Applications/MosaicoCameraDemo/README.md)；功能分类不会消除其硬件依赖。

应用界面从编辑器工程生成。GfxTouchDemo 附可导入项目、ui.h 与业务回调；[WioFieldLab](08_Applications/WioFieldLab/README.md) 是可单独复制、编译的 Wio Terminal 完整应用，包含传感器、CSV、IR、Wi-Fi、已编译界面与可选编辑的项目文件，运行时无需编辑器。生成资源的 ABI 应与所用运行库匹配。

## 维护

`_shared` 只保存维护模板；可运行的草图均自带所需头文件，复制单个草图文件夹即可保留配置。修改模板后运行 `npm run examples:sync`。新增或迁移草图后运行 `node tools/check-examples.mjs --write` 更新索引，再运行 `npm run check`，验证功能分类、主文件同名、本地包含、模板和文档链接。
