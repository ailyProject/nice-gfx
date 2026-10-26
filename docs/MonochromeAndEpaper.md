# 单色屏与电子纸

单色 OLED、电子纸和彩色屏共用 nice_gfx 的绘图、Sprite、字体及原生 UI。颜色与物理刷新是独立能力，UI 不依赖具体控制器名称；菜单、路由、实体按键及业务回调继续使用原接口。

## 支持范围

| 显示控制器 | 配置 | 公共显示类型 |
| --- | --- | --- |
| SSD1306 | 128×32、128×64，I²C / SPI | `SSD1306_128x32_I2C`、`SSD1306_128x64_I2C` 及对应 `_SPI` |
| SH1106 | 128×64，I²C / SPI | `SH1106_128x64_I2C`、`SH1106_128x64_SPI` |
| SSD1681 | Waveshare 1.54 V2 黑白模组，200×200 | `SSD1681_200x200_EPD_SPI` |
| SSD1680 | Waveshare 2.9 V2 黑白模组，128×296 | `SSD1680_128x296_EPD_SPI` |

电子纸类型对应上述具体模组配置；相同控制器、尺寸的其他屏幕不能仅凭型号视为兼容。波形、初始化及忙电平必须匹配模组。其他型号见 [电子纸兼容](EpaperCompatibility.md)、[Seeed 墨水屏](SeeedEpaper.md) 和 [U8g2 兼容](U8g2Compatibility.md)。

## 基础绘图

Arduino I²C 示例，接线与总线初始化按实际开发板设置：

```cpp
#include <NICE_GFX.h>
#include <Wire.h>
#include <Platform/Arduino/I2cWire.h>

nice_gfx::I2cWire bus(Wire);
SSD1306_128x64_I2C screen(bus, 0x3C);

void setup() {
  Wire.begin();
  if (!screen.begin()) return;
  screen.setFont(nice_gfx::fonts::Fixed6x13);
  screen.startWrite();
  screen.fillScreen(GFX_BLACK);
  screen.setTextColor(GFX_WHITE);
  screen.drawString("Menu", 2, 2);
  screen.endWrite();
}
void loop() {}
```

多项绘图用 `startWrite()` / `endWrite()` 合并成一次逻辑更新；独立绘图后可显式调用 `present()`。尤其不要在逐像素循环中调用 `present()`。单色后端只发送实际变化的字节，SSD1306 与 SH1106 使用各自的寻址方式。

电子纸的 SPI 构造参数为 `CS, DC, RESET, BUSY, CLOCK, MOSI, frequency`。`begin()` 启动非阻塞初始化；`present()` 接受更新，持续调用 `pollDisplay(millis())` 推进物理刷新。`displayRefreshing()` 查询物理忙状态，`epaperPanel()` 提供模组刷新配置及恢复入口。BUSY 引脚必须实际连接。

## 原生 UI 与菜单

- 持续调用 `ui.tick(millis())`，它会轮询显示硬件；不要等待电子纸刷新结束才读取按键或更新业务状态。
- `View::busy()` 保留逻辑绘制与动画的含义；`View::refreshing()` 还包含物理显示刷新。`finish()` 完成 UI 合成及源缓冲读取，不阻塞等待电子纸光学刷新。
- OLED 保留位移、滚动等动画，并按能力限频。单色不支持的透明渐变效果直接采样到终态，保留完成事件及显隐动作。
- 电子纸采用终态切页、开关与仪表更新，停止自动媒体播放和惯性滚动；物理刷新时继续处理输入，并合并后续界面变化。
- 单色焦点使用与实际底图相反的像素；禁用态增加虚线标记，分页的空心/实心圆点保持可辨。彩色渲染路径保留原行为。

小屏按目标分辨率排版，不整体缩小彩色页面。优先短文本、列表与分页，避免依赖颜色、持续动画和复杂软键盘完成关键操作。

新设计可优先采用 OLED 黑底白字、电子纸白底黑字。电子纸数值设置优先使用加减、选项与确认操作；连续输入先更新业务状态，画面按实际刷新能力合并呈现。编辑器保留原设计，切换目标不会永久改写作者的颜色及动画。

## 图片与内存

`MediaMono1` 使用每行补齐到字节的 MSB-first 位图，1 为白、0 为黑；`MediaMono1Masked` 每帧在颜色平面之后附加同尺寸遮罩，1 为不透明。因此黑色、白色均可独立于透明度使用。`ImageFormat::Mono1 / Mono1Masked` 同样支持复制、所有权交接及明确的字节步长。

导出资源通过 `NICE_GFX_MEDIA_MONO1_VERSION` 检查单色解码能力；RGB565 / RGB332 资源同样可用。

128×64 的一个单色像素平面为 1,024 B，但后端还会保留已发送图像、事务备份，UI 状态和临时块也占内存。不能把一个像素平面的大小当作整个 UI 的 RAM 占用。默认单色 UI 使用 4 行 RGB565 小块，128 像素宽时每块为 1,024 B；同步传输使用一块，异步传输使用两块。完整 UI 仍要求带完整 C++ 标准库的平台。AVR 仅适合按容量使用基础绘图，完整 UI 不适用；Uno 的 32 KiB Flash 可能不足。

ESP32 的单色/电子纸 SPI 选择字节传输模式，使用一个最多 1 KiB 的 DMA 可用暂存区，不创建彩色 RGB DMA 工作任务；原彩色 SPI 配置保持原行为。其他平台复用已有 SPI / I²C 实现。

## 使用建议

持续处理按键与业务状态，把屏幕刷新交给轮询推进。先用 [MonochromeMenu](../examples/07_UI/MonochromeMenu/README.md) 或 [EpaperRefresh](../examples/09_Diagnostics/EpaperRefresh/README.md) 核对接线，再在实屏测量刷新耗时、残影与稳定频率。