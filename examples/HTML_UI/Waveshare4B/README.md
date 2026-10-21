# ST7701 / GT911 / TCA9554 接线示例

选择ESP32-S3、16MB Flash、OPI PSRAM，Arduino-ESP32至少3.2。示例Hardware.h只保存板子接线和启动顺序。

- 显示芯片：[ST7701_RGB.h](../../../src/Drivers/Display/Video/ST7701_RGB.h)，Type1面板初始化。
- 触摸芯片：[GT911.h](../../../src/Drivers/Touch/GT911.h)，0x5D/0x14协议。
- 板级扩展IO：[TCA9554.h](TCA9554.h)，示例本地的GPIO及9位命令SPI实现，位于`waveshare4b`命名空间。
- Arduino总线：[I2cWire.h](../../../src/Platform/Arduino/I2cWire.h)，只处理Wire事务。

I²C为GPIO47/48；TCA9554地址0x20，P0/P1/P2提供命令SPI，P7复位LCD，P5复位GT911，P6选择GT911地址0x5D；GPIO4为低有效背光。所有这些接线都在应用Hardware.h中。

`Hardware.h`通过`RgbConfig::commandWrite`和`commandContext`注册本地命令发送回调；NICE_GFX核心只调用通用回调，像素仍通过RGB接口传输。复用此示例时请一并保留本地`TCA9554.h`；也可在应用中换用第三方TCA9554库，再提供同样签名的命令回调。

主机帧缓冲为RGB565，Type1面板使用COLMOD 0x60的RGB666 DPI引脚模式。RGB以16MHz扫描；RGB扫描中断在核心0，Arduino界面任务在核心1。电池、RTC、IMU和音频配置由应用管理。
