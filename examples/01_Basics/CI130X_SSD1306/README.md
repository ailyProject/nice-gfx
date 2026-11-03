# CI130X SSD1306

在 arduino-ci130x 开发板上运行 128×64 SSD1306 I2C OLED。示例使用内置
`Fixed6x13` 字体显示运行时间，每秒更新一次；不需要语音资源宏或网络服务。
已在 easyVoice 1306 dev + SSD1306 实板完成烧录与 I2C/帧写入检查；画面效果按下方预期目视确认。

## 接线

| 开发板配置 | OLED SDA | OLED SCL |
| --- | --- | --- |
| CI1302 / CI1303 | 2 / PA2 | 3 / PA3 |
| CI1306 / CI-D06GT01D | 15 / PB7 | 16 / PC0 |
| easyVoice 1306 dev | 11 / PB3 | 12 / PB4 |

同时连接匹配模块要求的电源和 GND。示例使用 `SDA`、`SCL` 别名，具体引脚随
Arduino IDE 中选择的开发板变化。除 easyVoice 1306 dev 板载上拉外，检查模块或外部电路的上拉电阻。
通用 CI1302/CI1303/CI1306 的 I2C 与 `Serial1` 共用引脚，不应同时启用；
CI1306 的部分 PDM 配置也会占用 I2C 引脚。

## 运行

1. 安装 ChipIntelli CI130X Arduino 开发板包和 NICE_GFX，选择实际开发板。
2. 打开 `CI130X_SSD1306.ino`，确认 `kOledAddress`。默认 `0x3C`，部分模块为 `0x3D`。
3. 编译并上传；串口波特率为 115200，输出引脚、地址应答及逐帧状态；初始化失败时会持续提示原因。
4. OLED 应显示 `CI130X + NICE_GFX`、总线速率和每秒更新的运行时间。

应用先调用并检查 `Wire.begin(SDA, SCL, 400000)`，再通过 `I2cWire` 将已初始化的总线交给显示对象。
NICE_GFX 不会替应用重新配置 Wire。CI13XX 的 Wire 为固定路由，不能任意换脚，最高支持 400 kHz。
一组绘图通过 `startWrite()` / `endWrite()` 合并刷新，循环中的 `delay(10)` 让出系统调度时间。

正常串口输出包含 `OLED address 0x3C ACK status=0` 和每秒递增的
`OLED frame t=...s status=0`。地址应答与绘图状态均为 `0` 表示通信与提交成功。

128×64 单色像素平面为 1,024 字节；面板缓存、事务备份、字体和系统任务还需要额外空间。
更多板型差异、软件 SPI 限制、资源预算和编译检查命令见 [CI130X 使用说明](../../../docs/CI130X.md)。
