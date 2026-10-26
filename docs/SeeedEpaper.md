# Seeed 墨水屏支持

提供 **23 个产品入口**：21 个使用 Seeed 官方 8-bit SPI 协议，两个 IT8951 产品使用 16 灰阶流式驱动。按面板型号或整机名称选型，完整配置见 [产品目录](../src/Display/Presets/seeed-epaper-displays.json)。

## 型号与入口

下列类型均以 `NGFX_SEEED_` 为前缀，包含头文件 `Display/Presets/SeeedEpaper.h`，构造参数为 `nice_gfx::GxEpd2Config`。

| 类型后缀 | Seeed 产品 | 控制器 | 可见分辨率 | 支持能力 |
| --- | --- | --- | --- | --- |
| `154_BW` | 1.54 英寸黑白 | SSD1681 | 200×200 | 黑白全刷 |
| `213_BW` | 2.13 英寸黑白 | SSD1680 | 122×250 | 黑白全刷 |
| `290_BW` | 2.9 英寸黑白 | SSD1680 | 128×296 | 黑白全刷 |
| `290_FLEX_BW` | 2.9 英寸柔性黑白 | UC8151D | 128×296 | OTP 波形、黑白全刷 |
| `213_FLEX_BW` | 2.13 英寸柔性黑白 GDEW0213I5FD | UC8151D | 104×212 | 官方独立 OTP 波形、黑白全刷 |
| `420_BW` | 4.2 英寸黑白 | SSD1683 | 400×300 | 黑白全刷 |
| `426_BW` | 4.26 英寸黑白 | SSD1677 | 800×480 | 官方反相传图、黑白全刷 |
| `583_BW` | 5.83 英寸黑白 | UC8179 | 648×480 | 黑白全刷 |
| `750_BW` | 7.5 英寸黑白 | UC8179 | 800×480 | 黑白全刷 |
| `750_BW_JD79686B` | 7.5 英寸 JD79686B 修订 | JD79686B | 800×480 | 独立初始化、黑白全刷 |
| `1030_GRAY16` | 10.3 英寸 | IT8951 / ED103TC2 | 1872×1404 | 16 灰阶全刷、真实读回 |
| `400_SPECTRA6` | 4.0 英寸六色 | ED2208 | 400×600 | 六色全刷 |
| `730_SPECTRA6` | 7.3 英寸六色 | ED2208 | 800×480 | 六色全刷 |
| `1330_SPECTRA6` | 13.3 英寸六色 | T133A01 | 1200×1600 | 双片选、六色全刷 |
| `709_SPECTRA6` | 7.09 英寸六色 | GDEB0709E01 / NT61522+EK73601 | 1200×1600 | 官方目录尺寸、双片选、六色全刷 |
| `154_BWRY` | 1.54 英寸四色 | JD79660 | 200×200 | 黑白黄红全刷 |
| `213_BWRY` | 2.13 英寸四色 | JD79676 | 122×250 | 黑白黄红全刷 |
| `290_BWRY` | 2.9 英寸四色 | JD79667 | 128×296 | 黑白黄红全刷 |
| `RETERMINAL_E1001` | reTerminal E1001 | UC8179 | 800×480 | 黑白全刷 |
| `RETERMINAL_E1002` | reTerminal E1002 | ED2208 | 800×480 | 六色全刷 |
| `RETERMINAL_E1003` | reTerminal E1003 | IT8951 / ED103TC2 | 1872×1404 | 16 灰阶全刷、真实读回 |
| `RETERMINAL_E1004` | reTerminal E1004 | T133A01 | 1200×1600 | 双片选、六色全刷 |
| `RETERMINAL_STICKY` | reTerminal Sticky | 自动检测 SSD1677 / SSD2677 | 800×480 | 芯片识别、温度读回、黑白全刷 |

2.13 英寸两款面板的控制器传输宽度为 **128**，可见宽度为 **122**，右侧补白由后端处理。不同控制器的 7.5 英寸屏保留不同预设，不能只凭尺寸互换。

目录包含固定来源中的 22 个产品及 2.13 英寸柔性屏 GDEW0213I5FD。其他 Seeed 产品可按精确面板料号查询 [电子纸目录](EpaperCompatibility.md)。

## 接线、板级供电与示例

完整可编译示例见 [SeeedEpaper.ino](../examples/09_Diagnostics/SeeedEpaper/SeeedEpaper.ino)。它使用 EE04 和 UC8179 7.5 英寸屏：

```cpp
#include <NICE_GFX.h>
#include <Display/Presets/SeeedEpaperBoards.h>

constexpr auto board = nice_gfx::SeeedEpaperBoard::EE04;
nice_gfx::SeeedEpaperBoardIo io(board);
nice_gfx::GxEpd2Config makeConfig() {
  auto c = nice_gfx::seeedEpaperConfig(board);
  c.io = &io;
  return c;
}
NGFX_SEEED_750_BW screen(makeConfig());
```

`io` 必须早于显示对象构造，并在整个使用期存活。板级适配器在 `begin()` 时启用官方配置的电源网，处理 reTerminal 共享总线上的 SD 片选；构造时不操作硬件。它不会挂载 SD 卡或接管应用的按键、传感器和电池管理。

| ESP32-S3 板卡 | SCK / MOSI / MISO | CS / DC / RST / BUSY | 第二片选 | 电源输出 HIGH |
| --- | --- | --- | --- | --- |
| EE02 | 7 / 9 / 无 | 44 / 10 / 38 / 4 | 41 | 43 |
| EE03 | 7 / 9 / 8 | 44 / 无 / 38 / 4 | 无 | 43 |
| EE04、EE05 | 7 / 9 / 无 | 44 / 10 / 38 / 4 | 无 | 43 |
| E1001、E1002 | 7 / 9 / 8 | 10 / 11 / 12 / 13 | 无 | 无额外屏幕电源脚 |
| E1003 | 7 / 9 / 8 | 10 / 无 / 12 / 13 | 无 | 21、11 |
| E1004 | 7 / 9 / 8 | 10 / 11 / 38 / 13 | 2 | 12 |
| Sticky | 13 / 14 / 12 | 15 / 16 / 17 / 18 | 无 | 47 |

以上为 ESP32-S3 的 GPIO 编号，按当前官方板卡修订配置。其他 MCU 的 XIAO Breakout / Driver / EN 板应显式填写当前 core 的 D 引脚映射，不套用这张表。官方 Breakout 使用 RST=D0、CS=D1、DC=D3、BUSY=D5、SCK=D8、MOSI=D10；Driver Board 的 BUSY 为 D2。

持续调用 `pollDisplay(millis())`，或使用自动推进显示轮询的 `View::tick()` / `pollUi()`。检查轮询返回值；`begin()` 成功表示初始化已被接受，屏幕光学刷新尚未完成。

## 能力边界与资源

- 产品入口统一开放全刷。上游包含的局刷、快刷、部分黑白屏的 4 灰阶暂未接入公共能力，不能通过底层私有类绕过后端快照管理。
- 10.3 英寸和 E1003 采用现有 IT8951 16 灰阶路径，既可显示黑白，也可显示灰阶；需要 ESP32 工作任务和 MISO，默认保留设备 VCOM 校准值。Sticky 同样需要 ESP32 工作任务和真实 MISO，用于探测、温度及波形选择。
- 图像按本库调色板索引存储，四色在传输前转为官方 RGB332 派生的 4-bit 色码，六色排除保留色码。双控制器屏按左右半幅分别发送到 CS 和 CS2。
- `fullRefreshMs` 是保守的界面提交间隔，不是实测耗时。实际完成由各官方协议的 BUSY 等待决定；没有 BUSY 不能据此宣称已刷新。
- 单帧内存以目录的 `framebufferBytes` 为准，目标和发送快照各占一份；六色及四色另有整帧 4-bit 转换缓冲，黑白另有旧帧。上游驱动还可能分配差分缓存，协议录制、事务备份和 DMA 暂存另计。13.3 / 7.09 英寸六色单个 4-bit 平面为 960,000 字节，应启用 PSRAM。
- Sticky 的官方整机还有水平镜像和默认 180° 方向约定；本库保留控制器原生方向，应用应根据实际装配设置旋转和画面方向。

## 来源与许可

Seeed 协议固定于提交 `a736f1726cedb89692ca9ebcedcc3d2468ef92f5`，保留 MIT 许可。原始 Git blob 与导入后 SHA256 见 [来源清单](../src/Drivers/Display/Seeed/upstream-manifest.json)。IT8951 使用的 GxEPD2 路径保留 GPLv3 许可。

本清单表示软件支持范围；板卡修订、电气连接、光学方向和刷新效果需在实际设备验证。