# 屏幕与触摸设备支持

本页列出屏幕芯片、接口、默认分辨率及构造参数。固定开发板优先使用 [开发板预设](BoardPresets.md)。分辨率按旋转 0 标注，实际接口、接线、电源与时序以模组为准。

## 屏幕芯片、接口与分辨率

包含 `<NICE_GFX.h>` 后使用下表类型。`cs`、`dc`、`rst`、`sck`、`mosi`、`miso`、`wr`、`rd`、`d0`～`d15` 是应用声明的实际引脚常量；`makeRgbConfig()`、`makeMipiConfig()`、`makeScanConfig()` 的写法见文末。每行示例独立使用，选择对应接口的类型后缀和参数。

<!-- BEGIN GENERATED DEVICE DISPLAYS -->

共 88 个屏幕芯片、137 组尺寸与初始化配置、423 个带分辨率的公开显示类型。相同配置的接口合并为一行；每行给出一个实际声明，其他接口按文末的构造参数表使用。

| 屏幕芯片 | 配置前缀 | 接口 | 默认宽×高 | 实例化示例 | 说明 |
| --- | --- | --- | --- | --- | --- |
| AXS15231B | `AXS15231B` | QSPI | 172×640 | `nice_gfx::AXS15231B_172x640_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| AXS15231B | `AXS15231B` | MIPI | 240×960 | `nice_gfx::AXS15231B_240x960_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| AXS15231B | `AXS15231B` | QSPI | 320×480 | `nice_gfx::AXS15231B_320x480_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| AXS15231B | `AXS15231B` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 360×640 | `AXS15231B_360x640_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 此处为 SPI 类型；不覆盖仅引出 QSPI 的模块；QSPI 仅旋转 0 |
| AXS15260D | `AXS15260D` | MIPI | 452×1280 | `nice_gfx::AXS15260D_452x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| CH13613 | `CH13613` | QSPI | 480×480 | `nice_gfx::CH13613_480x480_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅 ESP32-S3；当前旋转 0 |
| CO5300 | `CO5300` | QSPI | 368×448 | `nice_gfx::CO5300_368x448_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| CO5300 | `CO5300` | QSPI | 410×502 | `nice_gfx::CO5300_410x502_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| CO5300 | `CO5300` | QSPI / MIPI | 466×466 | `nice_gfx::CO5300_466x466_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| CO5300 | `CO5300` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 480×480 | `CO5300_480x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 此处为 SPI 类型；不覆盖仅引出 QSPI 的模块；仅旋转 0 |
| EK79007 | `EK79007` | MIPI | 1024×600 | `nice_gfx::EK79007_1024x600_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| EK9716B | `EK9716B` | RGB | 800×480 | `nice_gfx::EK9716B_800x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ER88577 | `ER88577` | MIPI | 800×1280 | `nice_gfx::ER88577_800x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| GC9106 | `GC9106` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 80×160 | `GC9106_80x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| GC9107 | `GC9107` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | `GC9107_128x128_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 另有 128×160 配置 |
| GC9107 | `GC9107` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | `GC9107_128x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 尺寸变体 |
| GC9503 | `GC9503` | RGB | 480×480 | `nice_gfx::GC9503_480x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| GC9A01 | `GC9A01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×240 | `GC9A01_240x240_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| GC9B71 | `GC9B71` | SPI / SW_SPI / QSPI | 320×386 | `GC9B71_320x386_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| GC9C01 | `GC9C01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 360×360 | `GC9C01_360x360_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 160×160 | `GC9D01_160x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 另有 40/50/60×160 配置 |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 40×160 | `GC9D01_40x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 尺寸变体 |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 50×160 | `GC9D01_50x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 尺寸变体 |
| GC9D01 | `GC9D01` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 60×160 | `GC9D01_60x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 尺寸变体 |
| HI8561 | `HI8561` | MIPI | 540×1168 | `nice_gfx::HI8561_540x1168_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| HX8347C | `HX8347C` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `HX8347C_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8347D | `HX8347D` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `HX8347D_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8352C | `HX8352C` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×400 | `HX8352C_240x400_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8357A | `HX8357A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `HX8357A_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8357B | `HX8357B` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `HX8357B_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8357C | `HX8357C` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `HX8357C_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8357D | `HX8357D` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `HX8357D_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8369A | `HX8369A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×864 | `HX8369A_480x864_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| HX8394 | `HX8394` | MIPI | 720×1280 | `nice_gfx::HX8394_720x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| HX8399 | `HX8399` | MIPI | 1080×1920 | `nice_gfx::HX8399_1080x1920_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| ILI9163 | `ILI9163` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | `ILI9163_128x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| ILI9225 | `ILI9225` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 176×220 | `ILI9225_176x220_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| ILI9331 | `ILI9331` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ILI9331_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| ILI9341 | `ILI9341_2` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ILI9341_2_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9341 第二套初始化方案 |
| ILI9341 | `ILI9341` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ILI9341_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| ILI9341 | `ILI9341_Transposed` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ILI9341_Transposed_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 转置扫描方向预设；引脚由构造参数传入 |
| ILI9342 | `ILI9342` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ILI9342_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 此列为当前配置默认值；按实际模块尺寸调整 |
| ILI9481 | `ILI9481` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | SPI 线像素为 RGB666；并口类型自动选择 RGB565 |
| ILI9481 | `ILI9481_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence2` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence2_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 2 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence2_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence2_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence3` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence3_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 3 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence3_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence3_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence4` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence4_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 4 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence4_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence4_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence5` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence5_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 5 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence5_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence5_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence6` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence6_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 6 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence6_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence6_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence7` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence7_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 7 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence7_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence7_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9481 | `ILI9481_Sequence8` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence8_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ILI9481 第 8 套初始化，SPI RGB666 |
| ILI9481 | `ILI9481_Sequence8_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9481_Sequence8_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9486 | `ILI9486` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9486_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | SPI 线像素为 RGB666；并口类型自动选择 RGB565 |
| ILI9486 | `ILI9486_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9486_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9486 | `ILI9486_PaddedSPI` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9486_PaddedSPI_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 带串并转换的屏板，使用匹配的桥接协议 |
| ILI9488 | `ILI9488` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9488_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | SPI 线像素为 RGB666；并口类型自动选择 RGB565 |
| ILI9488 | `ILI9488_Parallel` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ILI9488_Parallel_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 请按模块实际接口和接线选择 |
| ILI9806 | `ILI9806` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×854 | `ILI9806_480x854_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| ILI9881C | `ILI9881C` | MIPI | 800×1280 | `nice_gfx::ILI9881C_800x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| JBT6K71 | `JBT6K71` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `JBT6K71_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| JD9165 | `JD9165` | MIPI | 1024×600 | `nice_gfx::JD9165_1024x600_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| JD9365 | `JD9365` | MIPI | 800×1280 | `nice_gfx::JD9365_800x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| JD9366 | `JD9366` | MIPI | 800×1280 | `nice_gfx::JD9366_800x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| JD9613 | `JD9613` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 126×294 | `JD9613_126x294_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| JD9853 | `JD9853` | SPI / SW_SPI | 172×320 | `JD9853_172x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 触摸屏尺寸配置，见模组与触摸说明 |
| JD9853 | `JD9853` | SPI / SW_SPI | 240×284 | `JD9853_240x284_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| JD9853A | `JD9853A` | SPI / SW_SPI | 172×320 | `JD9853A_172x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 触摸屏尺寸配置，见模组与触摸说明 |
| NT35310 | `NT35310` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `NT35310_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| NT35510 | `NT35510` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | `NT35510_480x800_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | 16 位寄存器地址，示例选用 8 位并口 |
| NT39125 | `NT39125` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×432 | `NT39125_240x432_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| NV3001B | `NV3001B` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×220 | `NV3001B_128x220_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| NV3007 | `NV3007` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 168×428 | `NV3007_168x428_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| NV3022B | `NV3022B` | SPI / SW_SPI | 240×284 | `NV3022B_240x284_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| NV3023 | `NV3023` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | `NV3023_128x128_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| NV3041A | `NV3041A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 480×272 | `NV3041A_480x272_SPI gfx(cs, dc, rst, sck, mosi, miso);` | QSPI 仅旋转 0 |
| NV3052 | `NV3052` | RGB | 720×1280 | `nice_gfx::NV3052_720x1280_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| OTA7290B | `OTA7290B` | MIPI | 480×1920 | `nice_gfx::OTA7290B_480x1920_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| OTM8009A | `OTM8009A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | `OTM8009A_480x800_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | 16 位寄存器地址，示例选用 8 位并口 |
| PT6891 | `PT6891` | SPI / SW_SPI | 95×28 | `PT6891_95x28_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 仅旋转 0 |
| R61529 | `R61529` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `R61529_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| R61581 | `R61581` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `R61581_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| RM67162 | `RM67162` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 240×536 | `RM67162_240x536_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 此处为 SPI 类型；不覆盖仅引出 QSPI 的模块 |
| RM68120 | `RM68120` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | `RM68120_480x800_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | 16 位寄存器地址，示例选用 8 位并口 |
| RM68140 | `RM68140` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `RM68140_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| RM690B0 | `RM690B0` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 482×600 | `RM690B0_482x600_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 此处为 SPI 类型；不覆盖仅引出 QSPI 的模块；QSPI 仅旋转 0 |
| S6D02A1 | `S6D02A1` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | `S6D02A1_128x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| SD5207 | `SD5207` | MIPI | 568×1210 | `nice_gfx::SD5207_568x1210_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| SEPS525 | `SEPS525` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 160×128 | `SEPS525_160x128_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| SH8601 | `SH8601` | QSPI | 280×456 | `nice_gfx::SH8601_280x456_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| SH8601 | `SH8601` | QSPI | 368×448 | `nice_gfx::SH8601_368x448_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| SH8601 | `SH8601` | QSPI | 466×466 | `nice_gfx::SH8601_466x466_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | 仅旋转 0 |
| SH8601 | `SH8601` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 480×480 | `SH8601_480x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 此处为 SPI 类型；不覆盖仅引出 QSPI 的模块；仅旋转 0 |
| SPD2010 | `SPD2010` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI | 412×412 | `SPD2010_412x412_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 当前仅旋转 0；SPI mode 3，由类型自动选择 |
| SPFD5408 | `SPFD5408` | PARALLEL8 / PARALLEL16 | 240×320 | `SPFD5408_240x320_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | 旧版并口屏板协议；仅旋转 0 |
| SSD1283A | `SSD1283A` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 130×130 | `SSD1283A_130x130_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| SSD1331 | `SSD1331` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 96×64 | `SSD1331_96x64_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| SSD1351 | `SSD1351` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | `SSD1351_128x128_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| SSD1357 | `SSD1357` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×128 | `SSD1357_128x128_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| SSD1963 | `SSD1963` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 272×480 | `SSD1963_272x480_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | SSD1963 480 方案；示例选用并口 |
| SSD1963 | `SSD1963` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | `SSD1963_480x800_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | SSD1963 对应屏幕时序方案，使用并口 |
| SSD1963 | `SSD1963_ALT` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | `SSD1963_ALT_480x800_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | SSD1963 对应屏幕时序方案，使用并口 |
| SSD1963 | `SSD1963_BD` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 480×800 | `SSD1963_BD_480x800_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | SSD1963 对应屏幕时序方案，使用并口 |
| ST7102 | `ST7102` | RGB / MIPI | 480×480 | `nice_gfx::ST7102_480x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST7102 | `ST7102` | SPI / SW_SPI / QSPI / MIPI | 480×800 | `ST7102_480x800_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 仅旋转 0 |
| ST7121 | `ST7121` | MIPI | 720×1280 | `nice_gfx::ST7121_720x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| ST7123 | `ST7123` | MIPI | 720×1560 | `nice_gfx::ST7123_720x1560_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| ST7262 | `ST7262` | RGB | 1024×600 | `nice_gfx::ST7262_1024x600_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST7262 | `ST7262` | RGB | 480×272 | `nice_gfx::ST7262_480x272_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST7262 | `ST7262` | RGB | 800×480 | `nice_gfx::ST7262_800x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST7701 | `ST7701` | MIPI | 480×360 | `nice_gfx::ST7701_480x360_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| ST7701 | `ST7701` | RGB | 480×480 | `nice_gfx::ST7701_480x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST7703 | `ST7703` | MIPI | 720×1280 | `nice_gfx::ST7703_720x1280_MIPI gfx(makeMipiConfig());` | 仅旋转 0 |
| ST7735 | `ST7735` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 128×160 | `ST7735_128x160_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 模块 tab 类型可用 DisplayOptions.tab 调整 |
| ST7781R | `ST7781R` | PARALLEL8 / PARALLEL16 | 240×320 | `ST7781R_240x320_PARALLEL8 gfx(cs, dc, rst, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7);` | 旧版并口屏板协议；仅旋转 0 |
| ST7789 | `ST7789_2` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ST7789_2_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ST7789 第二套初始化方案 |
| ST7789 | `ST7789` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×240 | `ST7789_240x240_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 复用 ST7789 初始化；默认 240×240，保留各旋转的显存偏移 |
| ST7789 | `ST7789` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ST7789_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | — |
| ST7789 | `ST7789_Sequence3` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 240×320 | `ST7789_Sequence3_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | ST7789 第三套初始化方案 |
| ST7789P | `ST7789P` | SPI / SW_SPI | 240×284 | `ST7789P_240x284_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 触摸屏尺寸配置，见模组与触摸说明 |
| ST7789T3 | `ST7789T3` | SPI / SW_SPI | 240×320 | `ST7789T3_240x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 触摸屏尺寸配置，见模组与触摸说明 |
| ST7789V2 | `ST7789V2` | SPI / SW_SPI | 170×320 | `ST7789V2_170x320_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 触摸屏尺寸配置，见模组与触摸说明 |
| ST7789V2 | `ST7789V2` | SPI / SW_SPI | 240×280 | `ST7789V2_240x280_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 触摸屏尺寸配置，见模组与触摸说明 |
| ST77903 | `ST77903` | RGB | 320×480 | `nice_gfx::ST77903_320x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST77903 | `ST77903` | QSPI（持续扫描） | 400×400 | `nice_gfx::ST77903_400x400_QSPI gfx(makeScanConfig());` | 仅旋转 0 |
| ST77916 | `ST77916` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / QSPI / MIPI | 360×360 | `ST77916_360x360_SPI gfx(cs, dc, rst, sck, mosi, miso);` | MIPI 仅旋转 0 |
| ST77916 | `ST77916` | QSPI | 390×390 | `nice_gfx::ST77916_390x390_QSPI gfx(cs, rst, sck, d0, d1, d2, d3);` | — |
| ST77922 | `ST77922` | RGB / MIPI | 480×480 | `nice_gfx::ST77922_480x480_RGB gfx(makeRgbConfig());` | 仅旋转 0 |
| ST77922 | `ST77922` | SPI / SW_SPI / QSPI | 532×300 | `ST77922_532x300_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 仅旋转 0 |
| ST7796 | `ST7796_PaddedSPI` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 320×480 | `ST7796_PaddedSPI_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 带串并转换的屏板，使用匹配的桥接协议 |
| ST7796 | `ST7796` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 / MIPI | 320×480 | `ST7796_320x480_SPI gfx(cs, dc, rst, sck, mosi, miso);` | MIPI 仅旋转 0 |
| WEA2012 | `WEA2012` | SPI / SW_SPI / PARALLEL8 / PARALLEL16 | 356×400 | `WEA2012_356x400_SPI gfx(cs, dc, rst, sck, mosi, miso);` | 当前仅旋转 0；SPI mode 3，由类型自动选择 |

<!-- END GENERATED DEVICE DISPLAYS -->

另有尺寸由 `RgbConfig::rgb.timings` 决定的 `nice_gfx::EK9716B_RGB`、`nice_gfx::ST7262_RGB`。固定接线的快捷对象、支持状态及编辑器复用规则统一见 [开发板预设](BoardPresets.md)。

## 触摸芯片与使用入口

触摸驱动和总线入口见 [模组与触摸](VendorTouchScreens.md)，类型位于 `nice_gfx` 命名空间。完整导出生成 `TouchAdapter.h`，负责硬件初始化、采样与 `pollUi()` 调度。常用预设如下：

| 触摸芯片 | 接口与默认地址 | 实例化或配置方法 | 当前适配范围 |
| --- | --- | --- | --- |
| XPT2046 | 硬件 SPI / 软件 SPI | `nice_gfx::Xpt2046 touch(touchBus);`；设计器触摸类型为 `xpt2046` | 按实际屏幕尺寸校准；SPI CS 与可选 IRQ 由应用填写 |
| CHSC6X | I²C，`0x2E` | 选择 `xiao-round` 硬件预设，触摸类型为 `chsc6x` | XIAO ESP32S3 Round Display，GC9A01 240×240；使用 `Wire`，默认 100 kHz |
| CHSC6417 | I²C，`0x2E` | 选择 `oj-ojoy` 硬件预设，触摸类型为 `chsc6417` | ESP32-S3 + CH13613 480×480；使用 `Wire1`，默认 400 kHz |
| CST9220 | I²C，`0x5A` | 选择 `esp-mosaico` 预设，或配置触摸类型 `cst9220` 与实际 SDA/SCL/IRQ/RESET | ESP32 系列；使用 `Wire`，默认 400 kHz，坐标范围按工程尺寸检查 |
| GT911 | I²C，`0x5D` | 选择 `waveshare-esp32-s3-touch-lcd-4b` 预设 | ST7701 RGB + GT911；通过 `gfx.i2cBus()` 共享板级总线，TCA9554 负责复位 |
| GT1151 | I²C，`0x14` | 选择 `esp32-s31-korvo-1` 预设；库接口 `nice_gfx::Gt1151 touch(touchBus, 800, 480);` | ESP32-S31-Korvo-1 V1.1 / SUB3；SDA=0、SCL=1，编辑器使用 400 kHz；完整报告校验，公开 API 返回首个触点；[AURA 真机项目](../../nice_gfx_editor/projects/27-korvo-aura/README.md) |

### XPT2046 实例化

下面采用 ESP32 经典款示例接线与独立软件 SPI 触摸总线。引脚和校准数组都需要按硬件修改；数组依次为 `xMin, xSpan, yMin, ySpan, flags`。使用 [触摸校准示例](../examples/06_Touch/Touch_calibrate/Touch_calibrate.ino) 取得实际值。

```cpp
#include <NICE_GFX.h>
#include <Platform/Arduino/TouchSpi.h>

ILI9341_240x320_SPI gfx(5, 2, 4, 18, 23, 19);
nice_gfx::TouchSoftwareSpi touchBus(nice_gfx::TouchPins(26, 27, 32, 25, -1));
nice_gfx::Xpt2046 touch(touchBus);
const uint16_t calibration[5] = {300, 3600, 300, 3600, 3};
bool ready = false;

void setup() {
  if (!gfx.begin() || touch.begin() != nice_gfx::Status::Ok) return;
  if (touch.setCalibration(calibration, 0) != nice_gfx::Status::Ok) return;
  gfx.attachTouch(touch);
  ready = true;
}
void loop() {
  if (!ready) return;
  uint16_t x, y;
  if (gfx.getTouch(&x, &y)) {
    // 使用已经校准到屏幕坐标的 x、y。
  }
}
```

使用硬件 SPI 时，先初始化对应 `SPIClass`，再构造 `TouchSpi`。共享显示总线时，通过回调等待已提交的显示传输完成，再改变 SPI 参数；所有访问由同一任务串行执行：

```cpp
nice_gfx::Status beforeTouch(void *context) {
  return static_cast<NICE_GFX *>(context)->wait();
}
// SPI 由应用按实际引脚调用 begin()；此处使用已经初始化的实例。
nice_gfx::TouchSpi touchBus(SPI, touchCs, touchIrq, 2500000, beforeTouch, &gfx);
nice_gfx::Xpt2046 touch(touchBus);
```

`TouchSoftwareSpi` 的参数为 `TouchPins(clock, mosi, miso, cs, irq)`；`TouchSpi` 的参数为 `SPIClass&, cs, irq, frequency, beforeSelect, context`。IRQ 可为 -1，硬件 SPI 频率范围为大于 0 且不超过 2.5 MHz。总线和触摸对象需在屏幕使用期间保持有效。

### 电容触摸接入设计器界面

在设计器选择对应预设或触摸类型，填写实际引脚，导出完整 Arduino 工程。应用使用生成文件接入：

```cpp
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"

nice_gfx::htmlui::View ui(displayInstance(), kTilePixels);
bool ready = false;

void setup() {
  if (!beginDisplayHardware()) return;
  ready = ui_config::load(ui) == nice_gfx::Status::Ok;
}
void loop() {
  if (ready) pollUi(ui, millis());
}
```

适配器将触摸坐标转换到显示旋转后的 UI 坐标，约每 5 ms 采样，持续松手 30 ms 后报告释放；通信错误取消接触。自定义触摸可实现 `sampleTouch(int32_t &x, int32_t &y)`，返回 `TouchSample::Pressed / Released / Error`，再通过 [UI/Input.h](../src/UI/Input.h) 的 `TouchInput` 输入 View。业务事件处理见 [Arduino 组件交互](ArduinoComponentInteraction.md)。

## 接口构造参数与平台要求

表中的配置前缀、宽高和接口组成类型名，例如 `ILI9341_240x320_SPI`。改用同一行列出的其他接口时，更换类型后缀并使用对应参数；SPI / 软件 SPI / 并口类型在全局命名空间，QSPI / RGB / MIPI 类型使用 `nice_gfx::`。

| 接口 | 构造参数 | 平台与默认值 |
| --- | --- | --- |
| SPI | `cs, dc, rst, sck, mosi, miso, hz, options` | 已实现对应 SPI 后端的 Arduino 平台；`miso=-1`、`hz=40000000` |
| SW_SPI | 同 SPI | GPIO 软件 SPI；默认 `hz=1000000` |
| PARALLEL8 | `cs, dc, rst, wr, rd, d0`～`d7`，最后可选 `options` | GPIO 8 位并口；不读屏时 `rd=-1` |
| PARALLEL16 | `cs, dc, rst, wr, rd, d0`～`d15`，最后可选 `options` | GPIO 16 位并口；需要足够的数据引脚 |
| 普通 QSPI | `cs, rst, sck, d0, d1, d2, d3, hz`；或再传入 `options` | ESP32 quad SPI 后端；`hz=40000000`，没有 DC 参数；CH13613 当前用于 ESP32-S3 |
| RGB | `nice_gfx::RgbConfig` | 具有 LCD RGB 外设的 ESP32-S3 / ESP32-S31 / ESP32-P4；配置引脚和视频时序 |
| MIPI | `nice_gfx::MipiConfig` | ESP32-P4 DSI / DPI；配置通道、视频时序和 PHY 电源 |
| ST77903 扫描 QSPI | `nice_gfx::ScanQspiConfig` | 具有 `SOC_SPI_SCT_SUPPORTED` 外设的 ESP32-S3；持续扫描、自有帧缓冲 |

`options` 使用 `nice_gfx::PanelOptions`，默认构造即可。`rst=-1` 表示使用无复位 GPIO 的路径，仍需满足模组复位要求。AVR、SAMD、Renesas 等通用硬件 SPI 使用开发板实际的 SCK/MOSI/MISO 引脚。平台能力与资源边界见 [主库说明](../README.md#开发平台支持表)。

显示类型使用 RGB565 颜色值，像素缓冲格式依接口而定。表中接口表示库提供对应类型，仍需核对模组是否引出该接口。电源、gamma、偏移与初始化必须匹配实物。`_Transposed` 表示扫描方向变体，`_PaddedSPI` 表示串并转换协议，`_SequenceN`、`_2`、`_ALT`、`_BD` 表示其他初始化变体；它们复用芯片驱动。

## RGB 与 MIPI 配置

带芯片名的显示类型会选择正确的 `VideoController`，实际尺寸来自配置中的 SDK 时序。纯 RGB 控制器 EK9716B / ST7262 的命令初始化为空，仍需配置数据线、尺寸、时序、供电和背光。

```cpp
#include <NICE_GFX.h>

nice_gfx::RgbConfig makeRgbConfig() {
  nice_gfx::RgbConfig c;
  // 填写 c.reset、c.rgb 的数据/同步引脚、timings、像素时钟与帧缓冲。
  // ST7701 等需要命令初始化的控制器还要填写 c.io3wire.line_config。
  return c;
}
nice_gfx::ST7701_480x480_RGB gfx(makeRgbConfig());
```

RGB 的 `rgb` 保存 SDK 的数据线、时序与帧缓冲配置，当前输入和帧缓冲均为 RGB565，物理 RGB 总线支持 8 / 16 位。`io3wire` 使用三线 GPIO 与第九位命令/数据标记；`expect_clk_speed` 默认 1 MHz，`createIo` 和 `displayOnInit` 默认 true。

ESP32-P4 使用 MIPI 类型：

```cpp
nice_gfx::MipiConfig makeMipiConfig() {
  nice_gfx::MipiConfig c;
  // 填写 c.dsi 的 num_data_lanes / lane_bit_rate_mbps。
  // 填写 c.dpi 的 video_timing、像素时钟，以及板级 c.ldo。
  return c;
}
nice_gfx::ILI9881C_800x1280_MIPI gfx(makeMipiConfig());
```

MIPI 后端最多使用 2 条数据通道。ST77922 必须显式设置 `dsi.num_data_lanes=1`；AXS15231B、CO5300、ST77916、ST7796 默认 1 条，其余默认 2 条。JD9365、HX8399、HX8394 也支持 1 条。`panel.bits_per_pixel` 的 16 / 18 / 24 必须与 `dpi.out_color_format` 的 RGB565 / RGB666 / RGB888 一致，输入保持 RGB565；输出能力由各控制器检查。

RGB666 / RGB888 输出还要求 ESP32-P4 v3.00 或更新芯片，以及对应 `ChipVariant=postv3` 构建。EK79007、ST7123、ST7121、JD9165、OTA7290B、JD9366、HI8561、SD5207 的 MIPI 配置仅开放 16 位；ST77916 开放 16 / 24 位。`MipiConfig::acquireLdo` 默认 true，使用外部 PHY 电源时按板级方案关闭。HX8394 / OTA7290B 的部分板卡还需由应用执行 I²C 电源操作。

## ST77903 持续扫描 QSPI

ST77903 没有显示 RAM。该后端将局部绘制合成为自有 RGB565 帧缓冲，持续发送 VSYNC、消隐和逐行像素，使用内部 SRAM DMA 行缓冲。

```cpp
nice_gfx::ScanQspiConfig makeScanConfig() {
  nice_gfx::ScanQspiConfig c;
  // 填写真实 GPIO：c.clock、c.data[0..3]、c.cs、c.reset。
  c.width = 400;
  c.height = 400;
  c.frequency = 40000000;
  return c;
}
nice_gfx::ST77903_400x400_QSPI gfx(makeScanConfig());
```

| 配置字段 | 默认值与范围 |
| --- | --- |
| `host` | `SPI2_HOST`；也可用 `SPI3_HOST`，扫描器独占该 host |
| `clock`、`data[4]`、`cs` | -1，必须填写互不冲突的实际输出 GPIO |
| `reset`、`resetActiveHigh` | -1 / false；-1 使用软件复位 |
| `width`、`height` | 400 / 400；宽为 8 的倍数且不超过 400，高为 2 的倍数且不超过 480 |
| `frequency` | 40 MHz；范围 1～80 MHz，并须符合模组规格 |
| `frontPorch`、`backPorch` | 8 / 8 |
| `minimumLineUs` | 42；范围 41～100 微秒，实际行周期还包含传输和 SDK 开销 |
| `batchLines`、`batchCount` | 20 / 3；范围 1～64 行、2～8 组 |
| `framebufferInPsram` | true，分配失败后尝试内部 RAM |
| `taskStackBytes`、`taskPriority` | 4096 / 23；栈为 2048～65536，优先级大于 0 且低于 FreeRTOS 上限 |
| `taskCore` | -1 选择初始化核；显式设置时也必须为初始化核 |
| `displayOnInit`、`bgr` | true / false |
| `init` | 可选自定义模组命令序列 |

400×400 RGB565 帧缓冲占 320000 字节，默认三组 20 行 DMA 像素缓冲合计 48000 字节，另外还有 staging、描述符、任务栈和同步对象。关闭显示或 `shutdown()` 停止新增帧并回收已提交 DMA 后再释放资源。

## 自定义初始化、旋转与读回限制

`RgbConfig::init`、`MipiConfig::init`、`ScanQspiConfig::init` 接受 `VideoInitOverride`。命令和参数内存保持有效至 `begin()` 返回；自定义序列替换模组寄存器程序，驱动仍执行所需的协议前置步骤。

```cpp
static const nice_gfx::VideoInitCommand moduleInit[] = {
  // 此处应填写完整的实际模组序列；以下仅说明记录格式。
  {0x11, nullptr, 0, 120},
  {0x29, nullptr, 0, 0},
};
nice_gfx::VideoInitOverride init(moduleInit,
    sizeof(moduleInit) / sizeof(moduleInit[0]));
// rgbConfig.init = init;
// mipiConfig.init = init;
// scanConfig.init = init;
```

`VideoInitCommand` 的字段依次为命令、参数指针、参数字节数和命令后的延时毫秒数。`displayOnInit` 只控制后端额外的显示使能操作；初始化序列本身也可能包含显示使能命令。ST77903 自定义 B5/B6/3A/36 必须与尺寸、消隐、RGB565 和色序配置一致。

- RGB、MIPI、ST77903 扫描 QSPI 当前仅开放旋转 0。CO5300、SH8601、RM690B0、SPD2010 的对应 QSPI 配置及 CH13613、WEA2012 也有固定旋转限制，按具体类型使用。
- AXS15231B、NV3041A、ST7102 的普通 QSPI 局部绘制先合成整帧，在窗口结束或 `finish()` 时发送；多个窗口可能多次传输整帧。RGB565 帧缓冲占宽×高×2 字节。
- ST77922 窗口横向按 4 像素对齐。CO5300、SH8601、GC9B71、RM67162、RM690B0 的 QSPI 窗口按 2×2 对齐；SPD2010 的 QSPI 窗口横向及像素事务按 4 像素对齐。
- PT6891 使用专用指令、95×28 窗口和显存行映射。ST7735 的 tab、各控制器的色序与偏移通过实例选项配置。
- CH13613 的 QSPI 读取 opcode 为 `0x03`，当前未开放高级 `readPixel()` / `readRect()`；频率须符合所用模组规格。
- RGB / MIPI / 扫描 QSPI 的输入拷贝完成后可以复用调用方缓冲；`wait()` 不表示物理 VSYNC 或无撕裂。扫描后端的 `framebuffer()` 是只读本地内存视图。
- 屏幕读回还要求控制器协议和实际 MISO / RD 接线支持；触摸需要独立器件、接线与坐标校准。

## 实现与检查入口

屏幕类型与构造函数位于 [Display.h](../src/Display/Display.h)、[DisplayAliases.h](../src/Display/Presets/DisplayAliases.h)、[ColorAliases.h](../src/Display/Presets/ColorAliases.h)、[VideoAliases.h](../src/Display/Presets/VideoAliases.h)。RGB / MIPI 与扫描后端分别位于 [Esp32Video.h](../src/Platform/ESP32/Esp32Video.h) 和 [Esp32ScanQspi.h](../src/Platform/ESP32/Esp32ScanQspi.h)，寄存器程序在 [Controllers](../src/Drivers/Display) 中直接维护，RGB / MIPI 与扫描屏程序位于其 [Video](../src/Drivers/Display/Video) 子目录。

XPT2046 接口见 [Touch.h](../src/Drivers/Touch/Touch.h) 与 [TouchSpi.h](../src/Platform/Arduino/TouchSpi.h)。设计器触摸配置与适配器生成见同级维护目录中的 [hardware.mjs](../../nice_gfx_editor/packages/html-ui/hardware.mjs) 与 [arduino.mjs](../../nice_gfx_editor/packages/html-ui/arduino.mjs)。

在主库目录运行 `npm.cmd run check`，检查文档链接、元数据与源码边界。屏幕表按公开显示别名维护，触摸与板级说明需同步实现。硬件验证应覆盖首帧、色序、旋转、边角坐标、触摸释放与持续刷新。
