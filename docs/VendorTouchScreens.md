# 模组与触摸接入

显示芯片、触摸芯片与板级接线分别配置。完整设备和预设列表见 [设备支持](DeviceSupport.md)，源码职责见 [架构说明](Architecture.md)。同一外观或尺寸的模组可能使用不同芯片，应以实际硬件版本和接线为准。

| 驱动或协议族 | 头文件 | 当前公开类型 |
| --- | --- | --- |
| XPT2046 | [XPT2046.h](../src/Drivers/Touch/XPT2046.h) | `Xpt2046`，通过 `TouchSpi` 或 `TouchSoftwareSpi` 接入 |
| CST816 | [CST816.h](../src/Drivers/Touch/CST816.h) | `Cst816`、S/T/D 别名与 `Cst820` |
| FT 系列 | [FT6x36.h](../src/Drivers/Touch/FT6x36.h) | `Ft6x36`、`Ft6236`、`Ft6336`、`Ft6336U`、`Ft5x06`、`Ft3168`、`Ft6146` |
| CHSC6X | [CHSC6X.h](../src/Drivers/Touch/CHSC6X.h) | `Chsc6x` |
| AXS5106L / AXS15231B | [AXS5106L.h](../src/Drivers/Touch/AXS5106L.h)、[AXS15231B.h](../src/Drivers/Touch/AXS15231B.h) | `Axs5106L`、`Axs15231bTouch` |
| Goodix | [GT911.h](../src/Drivers/Touch/GT911.h)、[GT1151.h](../src/Drivers/Touch/GT1151.h) | `Gt911`、`Gt1151`，后者提供多点 Frame API |
| CST328 / CST3530 | [CST328.h](../src/Drivers/Touch/CST328.h)、[CST3530.h](../src/Drivers/Touch/CST3530.h) | `Cst328`、`Cst3530` |
| CST92xx | [CST92xx.h](../src/Drivers/Touch/CST92xx.h) | `Cst9217`、`Cst9220` |
| SPD2010 | [SPD2010.h](../src/Drivers/Touch/SPD2010.h) | `Spd2010Touch` |
| 四线电阻屏 | [TouchResistive.h](../src/Drivers/Touch/TouchResistive.h) | `FourWireTouch`，通过 `TouchAnalog` 接入 |

这些类型位于 `nice_gfx` 命名空间。I²C 驱动使用公共 `I2cBus`；Arduino 应用可使用 [TouchWire.h](../src/Platform/Arduino/TouchWire.h)，在草图中初始化 Wire 和实际引脚。旧的 [TouchI2c.h](../src/Drivers/Touch/TouchI2c.h) 继续提供统一包含入口。

可直接运行的接入示例：

- [Vendor_Capacitive_Touch](../examples/Generic/Vendor_Capacitive_Touch/Vendor_Capacitive_Touch.ino)：ST7789V2 与 CST816。
- [ESP32S31_GT1151](../examples/Generic/ESP32S31_GT1151/ESP32S31_GT1151.ino)：GT1151 采样、释放和错误状态。
- [Waveshare4B](../examples/HTML_UI/Waveshare4B/README.md)：ST7701、GT911 和示例本地 TCA9554 板级控制。
- [Touch_calibrate](../examples/Generic/Touch_calibrate/Touch_calibrate.ino)：XPT2046 校准。

显示模组变体保留独立初始化与扫描方向：JD9853A/JD9853 的 172×320 变体位于 [JD9853A.h](../src/Drivers/Display/JD9853A.h)，ST7789P 240×284 位于 [ST7789P.h](../src/Drivers/Display/ST7789P.h)。纯尺寸组合使用 [ModulePanels.h](../src/Display/Presets/ModulePanels.h) 与 `SizedPanel`。旧 Shield 的 [ST7781R](../src/Drivers/Display/ST7781R.h) 和 [SPFD5408](../src/Drivers/Display/SPFD5408.h) 保留各自初始化及来源声明；触摸电极接线由应用单独配置。
