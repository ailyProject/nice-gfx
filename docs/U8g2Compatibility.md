# U8g2 兼容

通过随库 u8x8 底层协议使用原 u8g2 屏幕配置，绘图、字体和菜单仍使用 NICE_GFX，无需另外安装 u8g2。完整型号见 [屏幕目录](../src/Display/Presets/u8g2-displays.json)。

## 覆盖范围

| 项目 | 当前覆盖 |
| --- | --- |
| 上游实体屏公开构造类 | 5,007 个，全部保留来源对应关系 |
| NICE_GFX 类型别名 | 1,669 个；合并同一模块的 `_1`、`_2`、`_F` 缓冲模式 |
| 实际显示回调 | 234 个；模块、偏移、反色及初始化变体分别保留 |
| 接口协议 | 四线 SPI、三线/9-bit SPI、I²C、6800、8080、KS0108、SED1520 |
| 屏幕类型 | OLED、单色 LCD、LED 矩阵、VFD，以及上游的电子纸配置 |

`NULL`、`BITMAP` 是虚拟输出，A2 热敏打印机不是屏幕，未列入屏幕目录。灰度控制器按上游 u8g2 的方式输出 **1-bit 黑白图像**，不能因此推断具备多级灰度。字体使用 [NICE_GFX 字体](font.md)。

编辑器可搜索型号与模块变体；SSD1306、SH1106 及指定电子纸模组优先使用 [专用路径](MonochromeAndEpaper.md)。

## 使用

在原 u8g2 构造类名中去掉缓冲模式 `_1` / `_2` / `_F`，把前缀 `U8G2_` 改为 `NGFX_U8G2_`。例如 `U8G2_PCD8544_84X48_F_3W_SW_SPI` 对应以下类型；引脚只是示例，必须按开发板和模块修改。

```cpp
#include <NICE_GFX.h>

nice_gfx::U8g2Config makeConfig() {
  nice_gfx::U8g2Config c;
  c.pins.clock = 12;
  c.pins.mosi = 11;
  c.pins.cs = 10;
  c.pins.reset = 8;
  return c;
}
NGFX_U8G2_PCD8544_84X48_3W_SW_SPI gfx(makeConfig());

void setup() {
  if (!gfx.begin()) return;
  gfx.startWrite();
  gfx.fillScreen(GFX_WHITE);
  gfx.drawRect(0, 0, gfx.width(), gfx.height(), GFX_BLACK);
  gfx.endWrite();
}
void loop() {
  gfx.pollDisplay(millis());
}
```

`begin()` 同步完成配置校验、内存/总线准备并接受初始化指令，持续的 `pollDisplay(now)` 推进控制器初始化与物理更新；使用原生 `View::tick()` 或编辑器生成的 `pollUi()` 会自动推进。`gfx.displayRefreshing()` 查询待完成工作；`asynchronous()==false` 表示输入像素在调用返回前已复制，不表示屏幕已经完成刷新。直接检查 `pollDisplay()` 或 `View::tick()` 返回值，后端状态可查 `gfx.u8g2Panel().status()`；`lastError()` 不自动记录独立轮询错误。故障恢复调用 `gfx.u8g2Panel().recover()` 并继续轮询。

所有别名接收同一种 `U8g2Config`，不兼容原 u8g2 的位置参数构造函数。`i2cAddress` 是 **7 位地址**，0 使用模块默认地址；`busClockHz=0` 使用上游默认频率。可选 CS/RESET 可以为 -1，但 MAX7219 的 LOAD/CS、SED1520 的 E1/E2 等必需信号不能省略。编辑器按具体协议生成接线项。

## 总线与性能

别名中的 HW/SW/2ND 保留上游来源。实际总线由配置决定，默认提供可移植的软件协议；不会凭名字猜测第二个外设或共享总线。

| 场景 | 配置 |
| --- | --- |
| ESP32 四线 SPI | `U8g2PlatformIo` + `U8g2Esp32SpiPort` + `U8g2SpiIo`，把后者地址赋给 `config.io`；编辑器自动生成 |
| 其他 Arduino 四线 SPI | `U8g2ArduinoSpiPort(SPIClass&)` + `U8g2SpiIo`；先按该 core 配置硬件 SPI 引脚 |
| 硬件 I²C | `U8g2I2cIo(gpio, I2cBus&)`；应用先配置 I²C 总线，ESP32 编辑器使用 `BoardI2c` |
| 三线 SPI 和特殊并口 | 保留上游 BYTE 协议，软件 GPIO 路径 |
| 已有高性能专用驱动 | 优先选择原 SSD1306 / SH1106 / 电子纸具体模组预设 |

`config.io` 是借用指针，GPIO、port 和 adapter 必须先于显示对象构造，并存活整个使用期。

ESP32 适配申请独立空闲 SPI host，使用 256 B DMA 暂存。通用 `SPIClass` 在跨 poll 事务期间可能保持选择，必须专用或由应用协作管理；此时不能直接调用同一总线上的阻塞触摸驱动。硬件 I²C 保留上游事务并透传总线错误，现有 `BoardI2c` / `I2cWire` 将 NACK 报告为 `Status::IoError`。默认软件 I²C 不检查 ACK，也不支持时钟延展。

固件按所选标签链接驱动，没有引用全部驱动的全局注册表。图像使用私有 1-bit 目标缓冲与发送快照；UI 事务需要时另分配回滚备份。128×64 的前两份像素缓冲共 2,048 B，另有 `tile_width×8 B` 转换暂存；其他状态、tape、I/O、UI 与分配器开销另计。它不提供 u8g2 的 `_1` 小页内存模式，低 RAM 平台需要核算完整峰值。

传输逐图块行录制，合并相邻命令，跳过未变化页；进行中的帧保持不可变快照，连续更新只保留最新目标。`pollBudgetMicros` 和 `maxDataBytesPerPoll` 限制单次工作，预算在回调之间检查，单个上游回调或 I²C 事务不能中途抢占。`maxTapeBytes` 是按需增长的上限，默认 32 KiB，并非预分配，初始化后释放大缓存。

电子纸沿用原初始化与刷新序列，相邻更新起点至少间隔 2 秒，等待期间合并最新目标；完成状态依据上游固定延时，不读取 BUSY。UI 采用终态动画及非阻塞输入。通用兼容路径不承诺局刷；需要指定模组局刷、BUSY 与休眠控制时，使用 [电子纸专用路径](MonochromeAndEpaper.md)。菜单系统继续依赖通用显示能力和输入接口，不依赖 u8g2 类型名。

## 来源与限制

随库协议固定于 u8g2 提交 `d6c8499c5f2707cac8eccd09fd8f677d12b17977`，使用私有符号，避免与应用另装的 u8g2 冲突。原始版权、BSD-2-Clause 许可与哈希见 [驱动目录](../src/Drivers/Display/U8g2/)。

目录覆盖表示软件配置与协议入口齐全。具体 MCU 的编译、实际接线、最高总线频率和光学刷新效果需按目标验证。