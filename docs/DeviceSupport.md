# 屏幕与触摸设备支持

本页列出屏幕芯片、接口、默认分辨率及构造参数。固定开发板优先使用 [开发板预设](BoardPresets.md)。分辨率按旋转 0 标注，实际接口、接线、电源与时序以模组为准。

| 屏幕类别 | 选型与用法 |
| --- | --- |
| 彩色屏与触摸 | 本页 |
| SSD1306 / SH1106、指定模组的电子纸局刷 | [单色屏与电子纸](MonochromeAndEpaper.md) |
| 原 u8g2 屏幕配置 | [U8g2 兼容](U8g2Compatibility.md) |
| GxEPD2 系列、多色及灰阶电子纸 | [电子纸兼容](EpaperCompatibility.md) |
| Seeed 墨水屏与整机 | [Seeed 墨水屏](SeeedEpaper.md) |

## 屏幕芯片、接口与分辨率

包含 `<NICE_GFX.h>` 后，按下表的配置前缀、宽高和接口组成类型名。引脚常量由应用按实际接线声明，构造参数见后文。

```cpp
ILI9341_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);
// QSPI 示例（独立使用）：
// nice_gfx::CH13613_480x480_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);
```

<!-- BEGIN GENERATED DEVICE DISPLAYS -->

彩色显示目录共 88 个屏幕芯片、137 组尺寸与初始化配置、423 个带分辨率的公开显示类型。相同配置的接口合并为一行；类型名与构造方式见下方说明。

| 屏幕芯片 | 配置前缀 | 接口 | 默认宽×高 | 说明 |
| --- | --- | --- | --- | --- |
| AXS15231B | `AXS15231B` | QSPI | 172×640 | 仅旋转 0 |
| AXS15231B | `AXS15231B` | MIPI | 240×960 | 仅旋转 0 |
| AXS15231B | `AXS15231B` | QSPI | 320×480 | 仅旋转 0 |
| AXS15231B | `AXS15231B` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 360×640 | QSPI 仅旋转 0 |
| AXS15260D | `AXS15260D` | MIPI | 452×1280 | 仅旋转 0 |
| CH13613 | `CH13613` | QSPI | 480×480 | 仅 ESP32-S3；当前旋转 0 |
| CO5300 | `CO5300` | QSPI | 368×448 | 仅旋转 0 |
| CO5300 | `CO5300` | QSPI | 410×502 | 仅旋转 0 |
| CO5300 | `CO5300` | QSPI / MIPI | 466×466 | 仅旋转 0 |
| CO5300 | `CO5300` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 480×480 | 仅旋转 0 |
| EK79007 | `EK79007` | MIPI | 1024×600 | 仅旋转 0 |
| EK9716B | `EK9716B` | RGB | 800×480 | 仅旋转 0 |
| ER88577 | `ER88577` | MIPI | 800×1280 | 仅旋转 0 |
| GC9106 | `GC9106` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 80×160 | — |
| GC9107 | `GC9107` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | 另有 128×160 配置 |
| GC9107 | `GC9107` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | 尺寸变体 |
| GC9503 | `GC9503` | RGB | 480×480 | 仅旋转 0 |
| GC9A01 | `GC9A01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×240 | — |
| GC9B71 | `GC9B71` | SPI / SW_SPI / QSPI | 320×386 | — |
| GC9C01 | `GC9C01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 360×360 | — |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 160×160 | 另有 40/50/60×160 配置 |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 40×160 | 尺寸变体 |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 50×160 | 尺寸变体 |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 60×160 | 尺寸变体 |
| HI8561 | `HI8561` | MIPI | 540×1168 | 仅旋转 0 |
| HX8347C | `HX8347C` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | — |
| HX8347D | `HX8347D` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | — |
| HX8352C | `HX8352C` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×400 | — |
| HX8357A | `HX8357A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| HX8357B | `HX8357B` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| HX8357C | `HX8357C` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| HX8357D | `HX8357D` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| HX8369A | `HX8369A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×864 | — |
| HX8394 | `HX8394` | MIPI | 720×1280 | 仅旋转 0 |
| HX8399 | `HX8399` | MIPI | 1080×1920 | 仅旋转 0 |
| ILI9163 | `ILI9163` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | — |
| ILI9225 | `ILI9225` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 176×220 | — |
| ILI9331 | `ILI9331` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | — |
| ILI9341 | `ILI9341_2` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | ILI9341 第二套初始化方案 |
| ILI9341 | `ILI9341` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | — |
| ILI9341 | `ILI9341_Transposed` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | 转置扫描方向预设；引脚由构造参数传入 |
| ILI9342 | `ILI9342` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | 此列为当前配置默认值；按实际模块尺寸调整 |
| ILI9481 | `ILI9481` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | SPI 线像素为 RGB666；并口类型自动选择 RGB565 |
| ILI9481 | `ILI9481_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence2` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 2 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence2_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence3` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 3 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence3_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence4` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 4 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence4_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence5` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 5 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence5_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence6` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 6 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence6_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence7` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 7 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence7_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence8` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | ILI9481 第 8 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence8_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9486 | `ILI9486` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | SPI 线像素为 RGB666；并口类型自动选择 RGB565 |
| ILI9486 | `ILI9486_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9486 | `ILI9486_PaddedSPI` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 带串并转换的屏板，使用匹配的桥接协议 |
| ILI9488 | `ILI9488` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | SPI 线像素为 RGB666；并口类型自动选择 RGB565 |
| ILI9488 | `ILI9488_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 请按模块实际接口和接线选择 |
| ILI9806 | `ILI9806` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×854 | — |
| ILI9881C | `ILI9881C` | MIPI | 800×1280 | 仅旋转 0 |
| JBT6K71 | `JBT6K71` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | — |
| JD9165 | `JD9165` | MIPI | 1024×600 | 仅旋转 0 |
| JD9365 | `JD9365` | MIPI | 800×1280 | 仅旋转 0 |
| JD9366 | `JD9366` | MIPI | 800×1280 | 仅旋转 0 |
| JD9613 | `JD9613` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 126×294 | — |
| JD9853 | `JD9853` | SPI / SW_SPI | 172×320 | 触摸屏尺寸配置 |
| JD9853 | `JD9853` | SPI / SW_SPI | 240×284 | — |
| JD9853A | `JD9853A` | SPI / SW_SPI | 172×320 | 触摸屏尺寸配置 |
| NT35310 | `NT35310` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| NT35510 | `NT35510` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | 16 位寄存器地址 |
| NT39125 | `NT39125` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×432 | — |
| NV3001B | `NV3001B` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×220 | — |
| NV3007 | `NV3007` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 168×428 | — |
| NV3022B | `NV3022B` | SPI / SW_SPI | 240×284 | — |
| NV3023 | `NV3023` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | — |
| NV3041A | `NV3041A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 480×272 | QSPI 仅旋转 0 |
| NV3052 | `NV3052` | RGB | 720×1280 | 仅旋转 0 |
| OTA7290B | `OTA7290B` | MIPI | 480×1920 | 仅旋转 0 |
| OTM8009A | `OTM8009A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | 16 位寄存器地址 |
| PT6891 | `PT6891` | SPI / SW_SPI | 95×28 | 仅旋转 0 |
| R61529 | `R61529` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| R61581 | `R61581` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| RM67162 | `RM67162` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 240×536 | 接口以模组实际引脚为准 |
| RM68120 | `RM68120` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | 16 位寄存器地址 |
| RM68140 | `RM68140` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | — |
| RM690B0 | `RM690B0` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 482×600 | QSPI 仅旋转 0 |
| S6D02A1 | `S6D02A1` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | — |
| SD5207 | `SD5207` | MIPI | 568×1210 | 仅旋转 0 |
| SEPS525 | `SEPS525` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 160×128 | — |
| SH8601 | `SH8601` | QSPI | 280×456 | 仅旋转 0 |
| SH8601 | `SH8601` | QSPI | 368×448 | 仅旋转 0 |
| SH8601 | `SH8601` | QSPI | 466×466 | 仅旋转 0 |
| SH8601 | `SH8601` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 480×480 | 仅旋转 0 |
| SPD2010 | `SPD2010` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 412×412 | 当前仅旋转 0；SPI mode 3，由类型自动选择 |
| SPFD5408 | `SPFD5408` | PARALLEL8 / PARALLEL16 | 240×320 | 旧版并口屏板协议；仅旋转 0 |
| SSD1283A | `SSD1283A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 130×130 | — |
| SSD1331 | `SSD1331` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 96×64 | — |
| SSD1351 | `SSD1351` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | — |
| SSD1357 | `SSD1357` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | — |
| SSD1963 | `SSD1963` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 272×480 | SSD1963 480 时序方案 |
| SSD1963 | `SSD1963` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | SSD1963 对应屏幕时序方案，使用并口 |
| SSD1963 | `SSD1963_ALT` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | SSD1963 对应屏幕时序方案，使用并口 |
| SSD1963 | `SSD1963_BD` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | SSD1963 对应屏幕时序方案，使用并口 |
| ST7102 | `ST7102` | RGB / MIPI | 480×480 | 仅旋转 0 |
| ST7102 | `ST7102` | SPI / SW_SPI / QSPI / MIPI | 480×800 | 仅旋转 0 |
| ST7121 | `ST7121` | MIPI | 720×1280 | 仅旋转 0 |
| ST7123 | `ST7123` | MIPI | 720×1560 | 仅旋转 0 |
| ST7262 | `ST7262` | RGB | 1024×600 | 仅旋转 0 |
| ST7262 | `ST7262` | RGB | 480×272 | 仅旋转 0 |
| ST7262 | `ST7262` | RGB | 800×480 | 仅旋转 0 |
| ST7701 | `ST7701` | MIPI | 480×360 | 仅旋转 0 |
| ST7701 | `ST7701` | RGB | 480×480 | 仅旋转 0 |
| ST7703 | `ST7703` | MIPI | 720×1280 | 仅旋转 0 |
| ST7735 | `ST7735` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | 模块 tab 类型可用 PanelOptions.tab 调整 |
| ST7781R | `ST7781R` | PARALLEL8 / PARALLEL16 | 240×320 | 旧版并口屏板协议；仅旋转 0 |
| ST7789 | `ST7789_2` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | ST7789 第二套初始化方案 |
| ST7789 | `ST7789` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×240 | 复用 ST7789 初始化；默认 240×240，保留各旋转的显存偏移 |
| ST7789 | `ST7789` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | — |
| ST7789 | `ST7789_Sequence3` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | ST7789 第三套初始化方案 |
| ST7789P | `ST7789P` | SPI / SW_SPI | 240×284 | 触摸屏尺寸配置 |
| ST7789T3 | `ST7789T3` | SPI / SW_SPI | 240×320 | 触摸屏尺寸配置 |
| ST7789V2 | `ST7789V2` | SPI / SW_SPI | 170×320 | 触摸屏尺寸配置 |
| ST7789V2 | `ST7789V2` | SPI / SW_SPI | 240×280 | 触摸屏尺寸配置 |
| ST77903 | `ST77903` | RGB | 320×480 | 仅旋转 0 |
| ST77903 | `ST77903` | QSPI（持续扫描） | 400×400 | 仅旋转 0 |
| ST77916 | `ST77916` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI / MIPI | 360×360 | MIPI 仅旋转 0 |
| ST77916 | `ST77916` | QSPI | 390×390 | — |
| ST77922 | `ST77922` | RGB / MIPI | 480×480 | 仅旋转 0 |
| ST77922 | `ST77922` | SPI / SW_SPI / QSPI | 532×300 | 仅旋转 0 |
| ST7796 | `ST7796_PaddedSPI` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | 带串并转换的屏板，使用匹配的桥接协议 |
| ST7796 | `ST7796` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / MIPI | 320×480 | MIPI 仅旋转 0 |
| WEA2012 | `WEA2012` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 356×400 | 当前仅旋转 0；SPI mode 3，由类型自动选择 |

<!-- END GENERATED DEVICE DISPLAYS -->

另有尺寸由 `RgbConfig::rgb.timings` 决定的 `nice_gfx::EK9716B_RGB`、`nice_gfx::ST7262_RGB`。固定接线的快捷对象、支持状态及编辑器复用规则统一见 [开发板预设](BoardPresets.md)。

## 触摸芯片与使用入口

显示与触摸分别配置，类型位于 `nice_gfx` 命名空间。按芯片包含下表头文件；Arduino 的 I²C 驱动通过 [TouchWire](../src/Platform/Arduino/TouchWire.h) 适配已初始化的 `Wire`，已有总线应共享同一实例。

| 触摸芯片或协议 | 公开类型与入口 |
| --- | --- |
| XPT2046 | [Xpt2046](../src/Drivers/Touch/XPT2046.h)，使用 `TouchSpi` / `TouchSoftwareSpi` |
| CST816 / CST820 | [Cst816](../src/Drivers/Touch/CST816.h)、S/T/D 别名、`Cst820` |
| FT 系列 | [Ft6x36](../src/Drivers/Touch/FT6x36.h)、`Ft6236`、`Ft6336`、`Ft6336U`、`Ft5x06`、`Ft3168`、`Ft6146` |
| CHSC6X | [Chsc6x](../src/Drivers/Touch/CHSC6X.h) |
| AXS5106L / AXS15231B | [Axs5106L](../src/Drivers/Touch/AXS5106L.h)、[Axs15231bTouch](../src/Drivers/Touch/AXS15231B.h) |
| Goodix | [Gt911](../src/Drivers/Touch/GT911.h)、[Gt1151](../src/Drivers/Touch/GT1151.h)；后者另提供多点 Frame API |
| CST328 / CST3530 | [Cst328](../src/Drivers/Touch/CST328.h)、[Cst3530](../src/Drivers/Touch/CST3530.h) |
| CST92xx | [Cst9217 / Cst9220](../src/Drivers/Touch/CST92xx.h) |
| SPD2010 | [Spd2010Touch](../src/Drivers/Touch/SPD2010.h) |
| 四线电阻屏 | [FourWireTouch](../src/Drivers/Touch/TouchResistive.h)，使用 [TouchAnalog](../src/Platform/Arduino/TouchAnalog.h) |

设计器完整导出生成 `TouchAdapter.h`，负责初始化、坐标转换、采样和 `pollUi()` 调度。常用板卡组合如下，接线与地址以项目配置为准：

| 编辑器预设 | 显示 / 触摸 | 接入要点 |
| --- | --- | --- |
| `xiao-round` | GC9A01 / CHSC6X | `0x2E`，`Wire` |
| `oj-ojoy` | CH13613 / CHSC6417 | `0x2E`，`Wire1` |
| `esp-mosaico` | CO5300 / CST9220 | `0x5A`，`Wire` |
| `waveshare-esp32-s3-touch-lcd-4b` | ST7701 / GT911 | `0x5D`，通过 `gfx.i2cBus()` 共享总线，TCA9554 负责复位 |
| `esp32-s31-korvo-1` | ST7262 / GT1151 | `0x14`，SDA=0、SCL=1，800×480 |

CHSC6417 由编辑器生成专用 `TouchAdapter.h`，库内 `Chsc6x` 用于 CHSC6X。

接入示例见 [CST816](../examples/06_Touch/Vendor_Capacitive_Touch/Vendor_Capacitive_Touch.ino)、[GT1151](../examples/06_Touch/ESP32S31_GT1151/ESP32S31_GT1151.ino) 和 [Waveshare 4B](../examples/07_UI/Waveshare4B/README.md)。UI 初始化与业务回调见 [组件交互](ArduinoComponentInteraction.md)。自定义触摸通过 [TouchInput](../src/UI/Input.h) 向 View 提交按下、释放或错误采样；通信错误应取消接触。

### XPT2046 接线与校准

以下使用独立软件 SPI 总线，配合已初始化的显示对象 `gfx`。引脚与校准值须按实物修改；校准数组依次为 `xMin, xSpan, yMin, ySpan, flags`，通过 [触摸校准示例](../examples/06_Touch/Touch_calibrate/Touch_calibrate.ino) 获取。

```cpp
#include <Platform/Arduino/TouchSpi.h>

nice_gfx::TouchSoftwareSpi touchBus(nice_gfx::TouchPins(26, 27, 32, 25, -1));
nice_gfx::Xpt2046 touch(touchBus);
const uint16_t calibration[5] = {300, 3600, 300, 3600, 3};

// setup() 中，gfx.begin() 成功后：
// if (touch.begin() != nice_gfx::Status::Ok) return;
// if (touch.setCalibration(calibration, 0) != nice_gfx::Status::Ok) return;
// gfx.attachTouch(touch);
// loop() 中通过 gfx.getTouch(&x, &y) 读取屏幕坐标。
```

`TouchPins` 参数为 `clock, mosi, miso, cs, irq`。硬件 SPI 使用 `TouchSpi(SPIClass&, cs, irq, frequency, beforeSelect, context)`，先由应用初始化 SPI，频率不超过 2.5 MHz，IRQ 可为 -1。共享显示 SPI 时，`beforeSelect` 应调用显示对象的 `wait()` 并返回结果；同一任务串行访问，总线和触摸对象保持有效。

## 接口构造参数与平台要求

类型名由配置前缀、宽高和接口组成，例如 `ILI9341_240x320_SPI`。同一行的其他接口使用对应后缀及参数。SPI / 软件 SPI / 并口类型在全局命名空间，QSPI / RGB / MIPI 类型使用 `nice_gfx::`。

| 接口 | 构造参数 | 平台与默认值 |
| --- | --- | --- |
| SPI | `cs, dc, rst, sck, mosi, miso, hz, options` | 对应 Arduino SPI 后端；`miso=-1`、`hz=40000000` |
| SW_SPI | 同 SPI | GPIO 软件 SPI；默认 `hz=1000000` |
| PARALLEL8 / PARALLEL16 | `cs, dc, rst, wr, rd, d0`～`d7` / `d15`，最后可选 `options` | GPIO 并口；不读屏时 `rd=-1` |
| 普通 QSPI | `cs, rst, sck, d0, d1, d2, d3, hz`，最后可选 `options` | ESP32 quad SPI；`hz=40000000`，无 DC 参数；CH13613 当前用于 ESP32-S3 |
| RGB | `nice_gfx::RgbConfig` | 具有 LCD RGB 外设的 ESP32-S3 / ESP32-S31 / ESP32-P4 |
| MIPI | `nice_gfx::MipiConfig` | ESP32-P4 DSI / DPI |
| ST77903 扫描 QSPI | `nice_gfx::ScanQspiConfig` | 具有 `SOC_SPI_SCT_SUPPORTED` 外设的 ESP32-S3；独占 SPI host，自有帧缓冲 |

`options` 为 `nice_gfx::PanelOptions`；`rst=-1` 表示不使用复位 GPIO。AVR、SAMD、Renesas 等通用硬件 SPI 使用板卡实际 SCK/MOSI/MISO 引脚。类型存在仍需模组引出对应接口，电源、色序、偏移与初始化须匹配实物。平台能力见 [中文说明](../README_ZH.md#开发平台支持表)。

`_Transposed` 表示扫描方向变体，`_PaddedSPI` 表示串并转换协议，`_SequenceN`、`_2`、`_ALT`、`_BD` 表示初始化变体。

## RGB 与 MIPI 配置

以下为配置骨架，须补齐实际接线和时序。RGB / MIPI 的实际尺寸以配置时序为准，须与模组匹配。

```cpp
nice_gfx::RgbConfig makeRgbConfig() {
  nice_gfx::RgbConfig c;
  // 填写 c.reset、c.rgb 的数据/同步引脚、时序、像素时钟与帧缓冲。
  // ST7701 等还需填写 c.io3wire.line_config 命令引脚。
  return c;
}
nice_gfx::ST7701_480x480_RGB gfx(makeRgbConfig());
```

RGB 输入和帧缓冲均为 RGB565，物理总线支持 8 / 16 位。EK9716B / ST7262 无需命令初始化，供电和背光仍由板级配置负责。

```cpp
nice_gfx::MipiConfig makeMipiConfig() {
  nice_gfx::MipiConfig c;
  // 填写 c.dsi 的通道数/速率、c.dpi 的视频时序/像素时钟和 c.ldo。
  return c;
}
nice_gfx::ILI9881C_800x1280_MIPI gfx(makeMipiConfig());
```

MIPI 最多 2 条数据通道；ST77922 须显式设置 `dsi.num_data_lanes=1`。`panel.bits_per_pixel` 的 16 / 18 / 24 须与 `dpi.out_color_format` 的 RGB565 / RGB666 / RGB888 一致，输入保持 RGB565，具体输出位深与通道数由控制器校验。RGB666 / RGB888 还要求 ESP32-P4 v3.00 或更新芯片及 `ChipVariant=postv3` 构建。外部 PHY 供电时按板级方案关闭 `acquireLdo`；部分模组另需应用执行 I²C 电源操作。完整字段见 [Esp32Video.h](../src/Platform/ESP32/Esp32Video.h)。

## ST77903 持续扫描 QSPI

ST77903 没有显示 RAM，后端持续扫描自有 RGB565 帧缓冲，并使用内部 SRAM DMA 行缓冲。

```cpp
nice_gfx::ScanQspiConfig makeScanConfig() {
  nice_gfx::ScanQspiConfig c;
  // 填写 c.clock、c.data[0..3]、c.cs、c.reset 的实际 GPIO。
  c.width = 400;
  c.height = 400;
  c.frequency = 40000000;
  return c;
}
nice_gfx::ST77903_400x400_QSPI gfx(makeScanConfig());
```

宽为 8 的倍数且不超过 400，高为 2 的倍数且不超过 480；SPI 频率为 1～80 MHz，并须符合模组规格。扫描器独占 `SPI2_HOST` 或 `SPI3_HOST`，工作任务与初始化使用同一核。消隐、批次和任务配置见 [ScanQspiConfig](../src/Platform/ESP32/Esp32ScanQspi.h)。

默认 400×400 帧缓冲占 320000 字节，三组 20 行 DMA 像素缓冲另占 48000 字节，尚需任务栈、描述符等开销。`shutdown()` 排空 DMA 后释放资源。

## 初始化与传输限制

`RgbConfig::init`、`MipiConfig::init`、`ScanQspiConfig::init` 可用 [VideoInitOverride](../src/Display/VideoControllerTypes.h) 替换模组命令序列；命令及参数内存须保持有效至 `begin()` 返回。ST77903 的几何、消隐、像素格式与色序命令须和扫描配置一致。

- RGB、MIPI、ST77903 扫描 QSPI 仅旋转 0；其他固定旋转限制见屏幕表。
- AXS15231B、NV3041A、ST7102 的普通 QSPI 先合成整帧，在窗口结束或 `finish()` 时发送；多个窗口可能多次传输整帧。RGB565 帧缓冲占宽×高×2 字节。
- ST77922 窗口横向按 4 像素对齐；CO5300、SH8601、GC9B71、RM67162、RM690B0 的 QSPI 按 2×2 对齐；SPD2010 的 QSPI 横向及像素事务按 4 像素对齐。
- RGB / MIPI / 扫描 QSPI 完成输入拷贝后可复用调用方缓冲；`wait()` 不代表物理 VSYNC 或无撕裂。
- 读屏须同时具备控制器支持及 MISO / RD 接线；CH13613 当前未开放高级 `readPixel()` / `readRect()`。触摸需独立器件、接线与校准。

类型与参数见 [显示预设](../src/Display/Presets)，芯片初始化见 [显示驱动](../src/Drivers/Display)。文档与目录维护见 [架构说明](Architecture.md#配置维护)。