ESP-Mosaico DVP 帧采集稳定性可选适配包

状态
适用于 ESP32-S31、Arduino SDK 的 ESP-IDF v6.1-803-g94639ab7251、
qio_opi 配置与官方 RISC-V GCC 15.2.0。仅修改 libesp_driver_cam.a。
当前准备和验证记录见 VALIDATION.txt。是否已解决具体硬件问题，以实机日志为准。

用途与来源
公开的 ESP-Mosaico BSP 自动应用同一套 DVP 修复：
https://github.com/esp-mosaico/esp-mosaico-bsp/blob/e110720fc6eb1f1d208fcbc42f7881ca8ce782d6/components/mosaico_module_camera/CMakeLists.txt
原始 ESP-IDF:
https://github.com/espressif/esp-idf/tree/94639ab7251fc8230ca725df638c9a57dbb17153
官方补丁:
https://github.com/esp-mosaico/esp-mosaico-bsp/blob/e110720fc6eb1f1d208fcbc42f7881ca8ce782d6/components/mosaico_module_camera/patches/esp-idf-dvp-frame-capture-stability.patch

DVP 修复内容
- 非 JPEG 帧的 DMA 描述符链末尾增加独立 64 字节内部 DMA tail buffer。
  对外帧大小不变，边界多余采样不会越过用户帧缓冲区。
- 每帧重启 DMA 前清空 CAM FIFO，丢弃上一 VSYNC 边界残留的部分采样。
- 非 JPEG 数据少于应有帧大小时返回 received_size=0。
- 接收缓存同步长度按 64 对齐且不超过实际帧缓冲区。
应用应对 bytesused=0 / V4L2_BUF_FLAG_ERROR 的帧执行 QBUF 归还并丢弃，
而不是把首个不完整帧当作永久故障。

文件
patched/components/esp_driver_cam/ ：修改后的两个 C 源、两个私有头与共享控制头。
original/components/esp_driver_cam/ ：精确 IDF 原始源，便于对比与重新应用补丁。
bsp/ ：两个原始 BSP 补丁、BSP 当时的 CMakeLists.txt、README.md 和 LICENSE。
esp-idf-94639ab-dvp-stability.patch ：适配到精确 IDF 的补丁。
PROVENANCE.json ：官方仓库、固定提交、Git blob SHA 与 URL。
SOURCE-MANIFEST.json ：全部源文件 SHA256 与受支持原始 archive SHA256。
LICENSE ：Apache-2.0 全文。保留源文件版权及许可证声明。
apply-dvp-stability.ps1 ：可复现编译和可选替库脚本。

唯一源码适配
BSP 补丁使用更新的 ESP_ALIGN_UP / ESP_ALIGN_DOWN 名称。
此 IDF 对应函数分别名为 ALIGN_UP_BY / ALIGN_DOWN_BY。
适配补丁仅替换这两个宏名；修复语义保持一致。
修改的三个文件带有明确修改说明。
本包收录的 IDF / BSP 代码和派生修改均使用 Apache-2.0，不包含其他项目的 MIT 代码。

使用
先复制并配置一份隔离 SDK，保留当前可用固件。不要直接用全局 Arduino SDK 试验。
参数中的 SDK 根应包含 flags/、include/、qio_opi/、lib/；
GCC 根应包含 bin/riscv32-esp-elf-gcc.exe。

仅编译验证（不修改 SDK）：
  .\apply-dvp-stability.ps1 -SdkRoot 'C:\path\to\isolated-sdk\esp32s31' -GccRoot 'C:\path\to\riscv32-esp-elf' -BuildOnly

编译并将两个 object 写入明确指定的隔离 SDK：
  .\apply-dvp-stability.ps1 -SdkRoot 'C:\path\to\isolated-sdk\esp32s31' -GccRoot 'C:\path\to\riscv32-esp-elf'

脚本要求原始 libesp_driver_cam.a SHA256 精确等于
979daf20cc2db48e9405ce157d8baf4d1d28f9b3dce59d905be68ae02ef3c6d4，
并核验目标、IDF 版本、工具链版本、源文件 SHA。
它只重编 esp_cam_ctlr_dvp_cam.c.obj 与 esp_cam_ctlr_dvp_gdma.c.obj。
因为私有结构和 init 参数一起改变，两个 object 必须使用同一个补丁后的私有头。
脚本验证原 archive 所有成员和全局符号保留；不重编传感器、esp_video 或整个 IDF。
成功前保留原库备份和校验值，并写出 lib/nice-gfx-mosaico-dvp-stability.json。
相同脚本再次运行会核验已安装状态；遇到不同版本或未知库会拒绝覆盖。
编译临时文件保留在系统 TEMP，脚本输出具体位置。

替库后必须让 Arduino sketch 重新链接这份隔离 SDK。
使用新的 sketch build 目录或清除其已生成链接产物，防止旧静态库继续被缓存。

回滚
停止使用相关构建目录，读取 lib/nice-gfx-mosaico-dvp-stability.json 的 backupPath。
校验备份 SHA 等于 originalArchiveSha256，再将该备份复制回 libesp_driver_cam.a，
删除此适配器 marker，重新链接固件。不要删除唯一备份。

SC101IOT PCLK 补丁
bsp/esp-cam-sensor-sc101iot-pclk-delay.patch 单独保留用于来源核对。
apply-dvp-stability.ps1 不应用它、不修改传感器库。
该补丁把 SC101IOT 寄存器 0x3640 低两位改为 0，选择 0 ns PCLK delay
（原 720p 模式表低两位为 3，即 6 ns）。
这是独立时序调整，应用若需要应单独验证，避免同时改变 VGA、DMA 和 PCLK。

