# 电子纸兼容与选型

优先按精确面板料号与修订选择预设，同一控制器可能使用不同初始化和波形。

| 场景 | 入口 |
| --- | --- |
| SSD1680 / SSD1681 指定 Waveshare V2 模组，需要局刷、BUSY 或休眠 | [专用电子纸路径](MonochromeAndEpaper.md) |
| GxEPD2 系列及补充面板 | 本页；[完整面板目录](../src/Display/Presets/gxepd2-displays.json) |
| Seeed 产品及板级供电 | [Seeed 墨水屏](SeeedEpaper.md) |
| u8g2 中的电子纸配置 | [U8g2 兼容](U8g2Compatibility.md) |

## 支持范围

GxEPD2 兼容路径提供 **112 个面板预设**：固定版 GxEPD2 的 103 个低层面板，以及 GxEPD、bb_epaper、Adafruit_EPD 来源的 9 个补充配置。黑白、黑白红、四色、六色、七色与 IT8951 的 16 级灰阶使用各自色板。

[控制器覆盖清单](../src/Display/Presets/epaper-coverage.json) 包含 47 个标签，含别名与修订；以下另列单色路径的 SSD1606、SSD1607。

| 控制器系列 | 已有实现入口的标签 |
| --- | --- |
| SSD | SSD1606、SSD1607、SSD1608、SSD1619、SSD1675、SSD1675A、SSD1675B、SSD1677、SSD1680、SSD1681、SSD1682、SSD1683、SSD1685、SSD2677、SSD2683 |
| UC | UC8151、UC8151D、UC8159c、UC8171、UC8175、UC8176、UC8179、UC8179C、UC8253、UC8276、UC8276C |
| IL | IL0324、IL0326、IL0371、IL0373、IL0376F、IL0398、IL3820、IL3829、IL3895、IL3897、IL91874 |
| JD | JD79653A、JD79660、JD79661、JD79665AA、JD79667、JD79686AB |
| 其他 | EK79652、EK79655、EK79686、GD7965、HX8717、IT8951 |

SSD1675、SSD1675A、SSD1675B 使用独立配置；ACeP / Spectra6 按实际面板名选择。控制器标签覆盖不代表所有面板版本均兼容。

## 基础用法

```cpp
#include <NICE_GFX.h>
#include <Display/Presets/GxEpd2.h>

nice_gfx::GxEpd2Config makeConfig() {
  nice_gfx::GxEpd2Config c;
  c.pins.clock = 12; c.pins.mosi = 11;
  c.pins.cs = 10; c.pins.dc = 9; c.pins.reset = 8; c.pins.busy = 7;
  return c;
}
NGFX_GXEPD2_154_GDEY0154D67 gfx(makeConfig());

void setup() {
  if (!gfx.begin()) return;
  gfx.startWrite();
  gfx.fillScreen(GFX_WHITE);
  gfx.drawRect(0, 0, gfx.width(), gfx.height(), GFX_BLACK);
  gfx.endWrite();
}
void loop() {
  // 应用应检查返回值，并继续处理输入。
  gfx.pollDisplay(millis());
}
```

示例引脚须按实际接线修改。`begin()` 成功表示初始化已被接受，后续持续调用 `pollDisplay()`；`View::tick()` 或生成的 `pollUi()` 会自动推进。`displayRefreshing()` 查询待完成工作，`asynchronous()==false` 只表示输入像素已复制。

轮询错误检查返回值或 `gfx.gxEpd2Panel().status()`；恢复调用 `gfx.gxEpd2Panel().recover()`。本路径开放全刷；指定模组局刷与休眠使用专用路径。

## 总线与平台

- ESP32 默认申请独立 SPI host，使用 DMA 暂存；其他 Arduino 默认使用 core 的标准 SPI 引脚。自定义总线通过 `config.io` 注入，适配器须存活整个使用期。
- 105 个面板使用协议录制与预算回放。ESP32 在工作任务中录制；其他 MCU 的录制阶段同步执行，轮询预算为软限制。
- 四个 IT8951 配置、两个 12.48 英寸配置与 GDEH0576T81 需要真实读回，目前限 ESP32。IT8951 必须接 MISO，`vcomMillivolts=-1` 保留设备校准值。
- 多控制器面板须填写额外片选等必需信号。模组电源、BUSY、电平与波形按具体硬件验证。

## 图片与内存

目标图与发送快照各占一份，刷新期间只保留后续最新目标。事务备份、三色上传平面、协议缓存与 DMA 暂存另计，目录中的 `framebufferBytes` 仅是单帧打包数据。

协议缓存默认上限 1 MiB，按需分配；扩容峰值可达新容量的约 1.5 倍。ESP32 工作任务另需 4 KiB 栈及 RTOS 开销，SPI 暂存另占 512 B。大屏优先使用 PSRAM，并检查分配结果。

IT8951 将打包灰阶按 16 行展开上传，避免整屏 8-bit 副本。编辑器按实际色板预览，慢屏采用动画终态与最新帧合并；输入及业务回调继续处理。

## 来源与许可

固定来源见 [GxEPD2 清单](../src/Drivers/Display/GxEpd2/upstream-manifest.json) 与 [补充协议来源](../src/Drivers/Display/EpaperExtra/upstream-sources.json)。GxEPD2 / GxEPD 衍生驱动保留 GPLv3，BitBank 部分保留 GPL-3.0-or-later，Adafruit 部分保留 MIT；分发时保留相应版权与许可。

本清单为软件协议支持范围，刷新耗时、残影、温度适应性与稳定频率需实屏测量。裸并口体系 EPDiy、Inkplate 不属于上述 SPI 兼容范围。