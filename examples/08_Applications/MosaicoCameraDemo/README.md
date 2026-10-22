# ESP-Mosaico V1.2 摄像头实时预览与识别叠加

本示例使用设计器的 `esp-mosaico` 预设：ESP32-S31 CoreBoard V1.2、CO5300 QSPI 480×480、CST9220 触摸。画面区域为 320×240，采集与转换在后台任务执行，现有 `gfx-media` / `setImage(Image)` 负责显示。无需修改 NICE_GFX 核心。

识别示例是真实的**颜色阈值识别**：在当前摄像头画面中寻找红、绿、蓝、黄区域，叠加对应颜色的矩形和标签。它没有使用神经网络，不提供人脸/物体类别识别，也不生成模拟检测框。每种颜色最多一个包围框；多个同色物体会合并。阈值受光照和白平衡影响，可在 `FrameProcessing.h::colorClass()` 调整。

## 硬件与软件

- 使用 **ESP-Mosaico CoreBoard V1.2**，摄像头板插在**左侧 H2 槽**。
- 官方 CameraBoard V1.2 为 OV3640；V1.4 为 SC101IOT。两者引脚相同，使用摄像板自带晶振。主板版本和摄像头板版本是两回事。
- Arduino-ESP32 **4.x，且实际包含 ESP32-S31 / Espressif ESP-Mosaico V1.2 板型**。3.x 的普通 ESP32-S3 板型不能替代。
- 核心 SDK 必须编入 `esp_video` DVP 支持，以及所用传感器的驱动、DVP 自动探测与一个 RGB565/UYVY/YUYV 输出模式。SDK 配置由核心的预编译库决定，在草图中添加 `#define CONFIG_CAMERA_...` 不能补入缺失的驱动。
- 使用当前 NICE_GFX 库，支持 `NICE_GFX_UI_DYNAMIC_IMAGE_VERSION >= 1`、UI ABI 15；启用板型默认 OPI PSRAM。

所需 SDK 配置（按所用摄像头启用对应项）：

```text
CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE=y
CONFIG_CAMERA_OV3640=y
CONFIG_CAMERA_OV3640_AUTO_DETECT_DVP_INTERFACE_SENSOR=y
# 或 SC101IOT：
CONFIG_CAMERA_SC101IOT=y
CONFIG_CAMERA_SC101IOT_AUTO_DETECT_DVP_INTERFACE_SENSOR=y
CONFIG_SPIRAM=y
```

预编译库若缺少这些功能，需要使用提供它们的 Arduino 核心构建，或按下面的可选适配说明准备 SDK 副本。摄像头驱动由所选 SDK 提供。

本次使用的 S31 SDK 已包含 SC101IOT，但缺少 OV3640。针对同版本 `esp_cam_sensor 2.6.0`，附带 [OV3640 可选适配](data/ov3640/README.md)：在 **SDK 副本** 中重建两个小对象并补入传感器静态库，保留原有驱动和检测表。该脚本核验组件版本、源文件校验和及编译器，并备份原库；请按其说明执行，不能只在草图中补宏。此次提供的固件已链接 OV3640 与 SC101IOT 两种检测器。

## 首帧后停流的驱动适配

实机最初收到一帧后，`VIDIOC_DQBUF` 持续返回 `ETIMEDOUT (116)`。当前 `idf-release_v6.1-94639ab7-v1` 的 DVP 驱动尚未包含 Mosaico BSP 的帧尾稳定性修复：非 JPEG DMA 链需要额外的独立 64 字节尾缓冲，并在重新接收前清空 FIFO。该修复属于底层摄像头驱动，单纯增加 UI 缓冲或延长等待超时不能替代它。

示例的 `data/dvp-stability/` 保存了对应精确 IDF 版本的源码、补丁、来源和重建说明，只适用于所注明的 SDK 副本。另一个 BSP 适配将 SC101IOT 的 PCLK 延迟由 6ns 调整为 0ns；应用必须先验证传感器 PID，再修改对应寄存器，不能将 SC101IOT 的寄存器写入 OV3640。

来源：[Mosaico DVP 稳定性补丁](https://github.com/esp-mosaico/esp-mosaico-bsp/blob/e110720fc6eb1f1d208fcbc42f7881ca8ce782d6/components/mosaico_module_camera/patches/esp-idf-dvp-frame-capture-stability.patch)、[SC101IOT PCLK 补丁](https://github.com/esp-mosaico/esp-mosaico-bsp/blob/e110720fc6eb1f1d208fcbc42f7881ca8ce782d6/components/mosaico_module_camera/patches/esp-cam-sensor-sc101iot-pclk-delay.patch)。两项适配的组合已在本机 SC101IOT 实测通过：约 35 秒串口观察中 DQ/Q 达到 499/499，超时、归还失败和不完整帧均为 0；预览帧号达到 227，UI 提交约 7–8 FPS。用户确认画面持续更新，COLOR DETECTION 开关正常。这里没有分别测量两项适配各自的效果，也没有测试 OV3640 实物。

## 使用

1. 安装当前 NICE_GFX，打开同目录 `MosaicoCameraDemo.ino`。
2. 开发板选择 **Espressif ESP-Mosaico V1.2**；USB 选 USB-OTG，USB CDC On Boot 开启，分区使用 `app3M_fat9M_16MB`。
3. 连接 Type-C USB-OTG，上传程序，打开 115200 串口监视器。无需等待串口连接即可独立运行。
4. 将红/绿/蓝/黄色物体放入镜头视野，观察框和标签。触摸 `COLOR DETECTION` 开关可开关识别与叠加。

本轮修复验证固件设有 45 帧纯采集阶段，期间暂不刷新 LCD/触摸；采集通过后自动进入预览，用于区分摄像头停流和显示处理。该阶段可通过 `CameraConfig.h::kCaptureOnlyFrames` 调整为 0。串口统计的 `DQ/Q/timeout/short` 分别为成功取帧、成功归还、取帧失败和不完整帧次数；原生 V4L2 `sequence` 在当前驱动不填写，不能用它判断帧是否推进。

当前 Arduino S31 的 USB 1200-baud 重启函数尚为空实现。此诊断固件额外支持串口发送大写 `B`，设置临时下载标志并重启到 ROM，便于后续烧录；它不修改 eFuse。首次替换旧固件仍需要按 BOOT 重连。这个入口还待实机验证。

Arduino CLI（用实际 NICE_GFX 路径替换 `<nice_gfx>`）：

```powershell
arduino-cli compile --fqbn 'esp32:esp32:esp_mosaico:USBMode=default,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB' --library '<nice_gfx>' '<nice_gfx>/examples/08_Applications/MosaicoCameraDemo'
```

右上 FPS 是**提交给 UI 的新画面速率**，不是 LCD 扫描/实际完成帧率。串口另外报告画面序号、识别颜色位掩码（bit0 红、bit1 绿、bit2 蓝、bit3 黄）、UI 完成帧数及 UI 动态内存。默认限制转换提交频率为 10 FPS，真实速率还取决于传感器默认模式和屏幕传输。

## 接线与初始化约定

| 功能 | GPIO / 配置 |
| --- | --- |
| 摄像头 D0…D7 | 16, 15, 33, 4, 14, 12, 18, 13 |
| VSYNC / HREF / PCLK | 55 / 19 / 17 |
| RESET / PWDN | 53 / 48 |
| SCCB SDA / SCL | 0 / 1，原生 I2C1 |
| 摄像头 XCLK | 外部晶振，`GPIO_NUM_NC`，MCU 输出频率 0 |
| 闪光灯 | GPIO34，低有效；本例开漏输出 HIGH，保持关闭 |
| 主板触摸 SDA / SCL | 56 / 3，生成的 TouchAdapter 使用 Wire / I2C0 |
| 扩展电源 | GPIO60，低有效，由 `esp_mosaico` 板型的 `initVariant()` 执行软启动 |

不要再次切换 GPIO60：LCD、触摸和摄像头共用外设供电。不要将 Korvo 或 ESP32-S3-EYE 的 DVP 默认引脚套到 Mosaico。摄像头占用的 GPIO33/34 与左槽 USB Serial/JTAG 共用，调试与下载请使用 Type-C USB-OTG。

## 数据与缓冲流程

```text
esp_video DVP / V4L2 mmap 帧
  → 后台转换 RGB565、等比缩放并补黑边
  → 真实颜色检测，结果附在同一个 Frame 中
  → 容量 1 的最新帧队列
  → UI: Image::adopt + setImage + setBounds/setText/setVisible
  → 最后一个 UI/快照引用释放后，归还输出帧池
```

- 原生 V4L2 查询真实宽高、格式和字节行步距；不假设摄像头直接输出 320×240。
- 接受 RGB565 LE/BE、RGB888/BGR888、UYVY/YUYV、灰度；YUV 转换使用 BT.601 limited-range。JPEG、RAW Bayer 等格式会明确报错，需要另外接解码器或选择传感器已编入的 YUV/RGB 模式。
- 两个摄像头驱动缓冲与三个 RGB565 输出缓冲分开管理。驱动帧转换后立即归还，不会被 UI 持有。
- 三个输出槽在 PSRAM 中约占 450 KiB，另有传感器原始缓冲、显示 tile、UI 和任务开销。传感器默认 1024×768/1280×720 等模式会明显增加采集缓冲占用。
- 队列满时丢弃尚未展示的旧帧，控制预览延迟；输出槽仍被 UI 使用时不会覆写。
- 所有 View、触摸和屏幕访问只在 Arduino loop/UI 任务中执行；后台任务不操作 View。
- 帧和识别框成组更新；已开始的分块绘制继续使用自己的旧快照。本示例不启用整页缓存。

## 接入自己的识别算法

替换 `CameraCapture.cpp::captureLoop()` 中的 `detectColors()` 调用即可：

1. 对当前 `Frame::pixels` 进行推理，或将原始图像上的检测坐标换算到本例 320×240 预览坐标。
2. 填写 `Frame::detections`，矩形坐标相对预览左上角；当前预留四个框，标签由 `detectionName()` 提供。
3. 保留结果所属帧的 `sequence/capturedAt`。若推理异步进行，应按帧匹配或明确处理过期结果，不能直接给新画面套上旧检测框。

分割图、热成像或已经包含叠加的图像，也可以直接写入输出 RGB565。需要更复杂的模型结果结构时，相应扩展 Frame 和 UI 更新逻辑。

## 编辑界面与重新生成

`data/project.gfx.json` 可导入 NICE GFX Studio。编辑器预览显示布局与动态画面占位，设备摄像头业务由草图提供。

本例保留 `data/home.html`、`data/home.css` 和生成脚本。用源码重新生成 UI/硬件适配文件：

```powershell
node data/generate-ui.mjs 'D:/Git/GFX/nice_gfx_editor'
```

脚本只更新 UI/硬件头和项目 JSON，不覆盖摄像头与草图业务代码。若在编辑器修改预览尺寸/位置或公开 ID，需同步 `CameraConfig.h` 和草图中的绑定。单独导出 `ui.h` 可更新外观；完整项目导出后的通用主程序不包含本例的摄像头业务。

## 验证边界

随示例提供可独立运行的像素转换与识别测试，生成脚本同时检查 UI 编译、硬件预设和公开组件几何。已使用 Arduino-ESP32 4.0.0、GCC 15.2.0 和 `idf-release_v6.1-94639ab7-v1` 完成 ESP32-S31 原生编译与链接，并核验两种传感器检测注册均进入最终固件。实际画面、颜色、方向、触摸与持续运行仍应按具体开发板检查；本例不承诺固定帧率。

在示例目录中使用宿主 `g++` 或 `clang++` 运行算法测试（不需要开发板）：

```powershell
g++ -std=c++17 -O1 -Wall -Wextra -Werror -pedantic -I . data/tests/frame_processing_tests.cpp -o frame_processing_tests.exe
./frame_processing_tests.exe
```

测试覆盖七种输入像素格式、RGB565 字节序、带行填充的输入、短缓冲与溢出拒绝、等比缩放/黑边、颜色筛选、包围框边界及输出保护区。

官方资料：[Mosaico 摄像板](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s31/esp-mosaico/user_guide_camera.html)、[ESP Video Components](https://docs.espressif.com/projects/esp-video-components/en/latest/esp32s31/Get_Started/index.html)、[OV3640 模式](https://github.com/espressif/esp-video-components/blob/master/esp_cam_sensor/sensors/ov3640/Kconfig.ov3640)、[SC101IOT 模式](https://github.com/espressif/esp-video-components/blob/master/esp_cam_sensor/sensors/sc101iot/Kconfig.sc101iot)。
