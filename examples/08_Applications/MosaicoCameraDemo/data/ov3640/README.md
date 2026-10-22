# ESP-Mosaico OV3640 可选适配

官方 CameraBoard V1.2 使用 OV3640；CameraBoard V1.4 使用 SC101IOT。部分 Arduino-ESP32 4.x S31 预编译 SDK 已含 SC101IOT，却没有 OV3640。本目录为前者补入官方 OV3640 驱动及静态检测表，不需要重建整个 ESP-IDF。

这是针对 Windows、ESP32-S31、qio_opi、esp_cam_sensor 2.6.0 和官方 RISC-V GCC 15.2.0 的可选适配。它不会给其他组件版本或芯片强行覆盖库。主板 CoreBoard V1.2 和 CameraBoard V1.2 不是同一个版本概念。

## 使用

先复制需要使用的 SDK，再传入该副本的 esp32s31 目录，以及编译器的 riscv32-esp-elf 根目录。脚本会明确修改指定 SDK 副本中的一个静态库，并保留原库备份；请确认 Arduino 构建实际使用此副本。

    .\apply-ov3640.ps1 -SdkRoot 'C:\my-sdk\esp32s31' -GccRoot 'C:\my-tools\riscv32-esp-elf'

SdkRoot 应包含 flags、include、lib 和 qio_opi；GccRoot 应包含 bin\riscv32-esp-elf-gcc.exe、ar.exe 和 nm.exe，以及相应标准库头。脚本仅使用已在本机准备的 SDK、编译器和本目录源文件，不下载依赖。

脚本先核验目标、组件版本、静态检测配置、编译器和源文件校验值，再使用该 SDK 自己的 flags/defines、flags/c_flags、flags/includes 编译两个 C 对象。候选库经过传感器符号和原成员保留检查后才替换原库。原子替换前还会再次校验原库，拒绝覆盖并发改动。

成功后，请使用新的 Arduino 构建目录重新编译，让链接器确实使用更新后的静态库。仅重新烧录旧 bin 不会生效。

## 修改内容

只新增 ov3640.c.obj，并替换 esp_cam_sensor_detect.c.obj。检测表继续依据原 SDK 的配置保留 SC101IOT 和其他已启用传感器。未修改 sdkconfig.h、其他 SDK archive 或 Arduino core 源码。

新增模式是板载 24 MHz 振荡器下的 OV3640 640 × 480 UYVY、30 FPS 传感器配置。示例再把帧缩小至预览尺寸并限制处理频率；此配置不保证 UI 或完整流水线达到 30 FPS。

本适配不负责电源、引脚或显示初始化。Mosaico 相机必须插左槽 H2；MCU XCLK 为 GPIO_NUM_NC，使用摄像板自己的振荡器。请保留示例中的 GPIO60 软启动、I2C 分配、闪光灯开漏高电平和帧归还逻辑。

## 备份与回退

成功运行会打印原库备份路径，并在 SDK 的 lib 目录写入 nice-gfx-ov3640-adapter.json。该记录包含原库与新库的 SHA-256、来源 commit 和编译器版本。

再次运行时，只有记录、新库和备份三者校验均通过，脚本才报告已安装并退出。若 SDK 已自行包含 OV3640，但没有本适配记录，脚本会拒绝覆盖。

需要回退时，停止相关构建，按记录的 backupPath 将原库复制回 libespressif__esp_cam_sensor.a，并移走对应适配记录。不要把本备份复制给另一 SDK 版本。

## 来源和许可证

源来自 Espressif 官方 esp-video-components，固定 commit：

    ee31057dcf3c652284959283780f591e3d6320f2

组件版本为 2.6.0。七个官方源/头文件保留原版权和 SPDX 头；完整 Apache-2.0 许可见 LICENSE。PROVENANCE.json 记录上游路径、Git blob SHA 和本地文件 SHA-256。ov3640_config.h 与 PowerShell 脚本是本示例集成文件。

本适配已使用实际 S31 SDK 完成 C 对象编译和 archive 符号检查。摄像头画面、颜色和整条流水线的性能仍需要在目标硬件上验证。

