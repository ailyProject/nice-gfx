# 开发板快捷预设

目录包含编辑器的 52 项配置：24 项 ready（含通用接线）、4 项 pending、24 项 unsupported。库可独立安装，无需编辑器目录。

## 使用方法

在匹配的目标板上选择快捷类型，无需填写显示引脚。以下为 Wio Terminal 的完整 Arduino 示例：

```cpp
#include <NICE_GFX.h>

nice_gfx::WioTerminalDisplay gfx;
void setup() {
  if (gfx.beginChecked() != nice_gfx::Status::Ok) return;
  gfx.fillScreen(GFX_BLACK);
}
void loop() {}
```

首次初始化应用固定接线、默认旋转、必要的复位与背光顺序。`begin()` 返回 bool，`beginChecked()` 返回具体错误。同一组显示引脚只保留一个显示对象；自定义接线请使用 [通用显示类型](DeviceSupport.md)。

- `gfx.preset()`：当前配置。
- `nice_gfx::findBoardPreset(id)`：按 ID 查询；未知 ID 返回 nullptr。
- `nice_gfx::boardPresetAt(index)`、`BoardPresetCount`：遍历完整目录。

触摸采样、按键与业务外设由应用接入。完整接线、来源和全部限制见 [BoardPresets.json](BoardPresets.json)，未知尺寸保留为 0。

## 编辑器如何复用

显示接线、频率、控制器与 RGB 时序匹配库预设时，Arduino / ESP-IDF 导出直接实例化对应快捷类型。修改这些参数后改用显式自定义配置。旋转、UI 缓冲和输入配置仍由项目决定；可用平台以实际类型和导出检查为准。

## 平台与共享外设

| 预设 | 限制与启动行为 |
| --- | --- |
| Wio Terminal / RP2040 | Arduino；前者使用专用 LCD_SPI |
| Mosaico V1.2 | 仅 `esp_mosaico` Arduino variant，由核心完成电源初始化；不支持原生 IDF |
| XIAO Round | 自动禁用共享 SPI 的 SD CS，应用 BGR 与反色 |
| 学而思 ESP32 | 使用 SPI2_HOST，MISO19 保持输入 |
| Waveshare 4B | TCA9554 完成 LCD/触摸复位，背光低有效；GT911 通过 `gfx.i2cBus()` 共享总线 |
| Korvo-1 | `gfx.copyStats()` / `gfx.scanStats()` 返回同一显示对象的统计 |

## 完整目录

ready 表示在匹配目标上可构造显示对象；pending 尚需板级适配；unsupported 缺少显示适配或关键信息。后两类及错误编译目标会给出构造诊断。表中只列主要限制，ready 不代表所有外设或真机验收已完成。

| 编辑器 ID | 快捷类型 | 状态 | 主要限制 |
| --- | --- | --- | --- |
| `ai-vox` | `AiVoxDisplay` | unsupported | 分辨率待确认 |
| `cubic-core-car` | `CubicCoreCarDisplay` | unsupported | 缺少显示配置：SSD1306 128×64 I2C |
| `custom-spi` | `CustomSpiDisplay` | ready | 通用参考接线 |
| `df-unihiker-k10` | `UnihikerK10Display` | pending | 需接入 TCA9555 背光与板级初始化 |
| `esp-mosaico` | `EspMosaicoDisplay` | ready | 见平台说明 |
| `esp32-s31-korvo-1` | `Esp32S31Korvo1Display` | ready | 见平台说明 |
| `esp32` | `Esp32Display` | ready | 通用参考接线 |
| `esp32s3-ili9341` | `Esp32S3Ili9341Display` | ready | 通用参考接线 |
| `heltec-wifi-lora-32-v3` | `HeltecWiFiLoRa32V3Display` | unsupported | 缺少显示配置：SSD1306 128×64 I2C |
| `lilygo-t-display-s3` | `LilygoTDisplayS3Display` | unsupported | 缺少显示配置：ST7789 170×320 PARALLEL8 |
| `linkbit` | `LinkbitDisplay` | unsupported | 缺少显示配置：EPD-CZ04 104×212 SPI |
| `m5stack-atoms3` | `M5AtomS3Display` | pending | 存在 GC9107 / ST7735S 屏幕版本，需确认实物版本 |
| `m5stack-cardputer` | `M5CardputerDisplay` | unsupported | 缺少显示配置：ST7789 240×135 SPI |
| `m5stack-core` | `M5CoreDisplay` | unsupported | 缺少显示配置：ILI9342C 320×240 SPI |
| `m5stack-core2` | `M5Core2Display` | unsupported | 缺少显示配置：ILI9342C 320×240 SPI |
| `m5stack-coreink` | `M5CoreInkDisplay` | unsupported | 缺少显示配置：GDEW0154M09 200×200 SPI |
| `m5stack-cores3` | `M5CoreS3Display` | unsupported | 缺少显示配置：ILI9342C 320×240 SPI |
| `m5stack-dial` | `M5DialDisplay` | pending | 需接入板载电源、背光或复位 |
| `m5stack-dinmeter` | `M5DinMeterDisplay` | unsupported | 缺少显示配置：ST7789V2 135×240 SPI |
| `m5stack-fire` | `M5FireDisplay` | unsupported | 缺少显示配置：ILI9342C 320×240 SPI |
| `m5stack-paper` | `M5PaperDisplay` | unsupported | 缺少显示配置：IT8951 960×540 SPI |
| `m5stack-station` | `M5StationDisplay` | unsupported | 缺少显示配置：ST7789 240×135 SPI |
| `m5stack-stickc-plus` | `M5StickCPlusDisplay` | unsupported | 缺少显示配置：ST7789 135×240 SPI |
| `m5stack-stickc-plus2` | `M5StickCPlus2Display` | unsupported | 缺少显示配置：ST7789 135×240 SPI |
| `m5stack-stickc` | `M5StickCDisplay` | unsupported | 缺少显示配置：ST7735S 80×160 SPI |
| `m5stack-tab5` | `M5Tab5Display` | unsupported | 缺少显示配置：ILI9881C 720×1280 MIPI |
| `m5stack-tough` | `M5ToughDisplay` | unsupported | 缺少显示配置：ILI9342C 320×240 SPI |
| `mpython-2` | `MPython2Display` | unsupported | 控制器待确认 |
| `mpython-3` | `MPython3Display` | unsupported | 当前显示目录尚无 ST7789 172×320 配置；需适配显存偏移与复位初始化 |
| `oj-badge` | `OjBadgeDisplay` | pending | CST816S 触摸与 LCD_CTRL17 板级初始化尚未接入 |
| `oj-easyvoicelab-1306` | `OjEasyVoiceLabDisplay` | unsupported | 分辨率待确认 |
| `oj-esp32-aiot-kit-lcd` | `OjAiotKitLcdDisplay` | ready | 见平台说明 |
| `oj-esp32-aiot-kit-oled` | `OjAiotKitOledDisplay` | unsupported | 分辨率待确认 |
| `oj-esp32s3basic` | `OjEsp32S3BasicDisplay` | ready | 见平台说明 |
| `oj-ojoy` | `OjOjoyDisplay` | ready | 见平台说明 |
| `rp2040` | `Rp2040Display` | ready | 通用参考接线 |
| `sd-grove-beginner-kit` | `SeeedGroveBeginnerKitDisplay` | unsupported | 缺少显示配置：SSD1306 128×64 I2C |
| `seeed-ee02-spectra133` | `SeeedEe02Spectra133Display` | ready | 见平台说明 |
| `seeed-ee03-103` | `SeeedEe03103Display` | ready | 见平台说明 |
| `seeed-ee04-75` | `SeeedEe0475Display` | ready | 见平台说明 |
| `seeed-ee05-75` | `SeeedEe0575Display` | ready | 见平台说明 |
| `seeed-reterminal-e1001` | `SeeedReterminalE1001Display` | ready | 见平台说明 |
| `seeed-reterminal-e1002` | `SeeedReterminalE1002Display` | ready | 见平台说明 |
| `seeed-reterminal-e1003` | `SeeedReterminalE1003Display` | ready | 见平台说明 |
| `seeed-reterminal-e1004` | `SeeedReterminalE1004Display` | ready | 见平台说明 |
| `seeed-reterminal-sticky` | `SeeedReterminalStickyDisplay` | ready | 见平台说明 |
| `seeed-trmnl-75-diy` | `SeeedTrmnl75DiyDisplay` | ready | 见平台说明 |
| `seeed-xiao-75-panel` | `SeeedXiao75PanelDisplay` | ready | 见平台说明 |
| `waveshare-esp32-s3-touch-lcd-4b` | `WaveshareTouchLcd4B` | ready | 见平台说明 |
| `wio-terminal` | `WioTerminalDisplay` | ready | 见平台说明 |
| `xiao-round` | `XiaoRoundDisplay` | ready | 见平台说明 |
| `xueersi-esp32` | `XueersiEsp32Display` | ready | 见平台说明 |

## 同步配置

源文件：`nice_gfx_editor/configs/boards/*.json`。在编辑器目录运行：

```powershell
npm.cmd run boards:library:generate
npm.cmd run boards:library:check
npm.cmd run runtime:rebuild
npm.cmd run runtime:check
```

本页、JSON 目录、库预设头和编辑器匹配记录一起生成，请修改源配置或生成器。
