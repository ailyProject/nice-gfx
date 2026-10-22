# Wio Field Lab / 随身观察仪

Wio Terminal 的独立完整应用示例，版本 **v4.1 IR fix**。打开 [WioFieldLab.ino](WioFieldLab.ino) 即可编译上传；界面、图片、字体子集和业务代码均已包含，无需安装或运行 nice_gfx_editor。

保留七页界面、透明绿叶素材、局部动效、首页右对齐，以及冷启动红外/静音卡死修复。单块 320×16 RGB565 绘图缓冲占 10240 字节，无需 PSRAM 或整屏缓存。

## 安装与运行

1. 将当前 NICE_GFX 库安装到 Arduino 草图目录的 `libraries` 下。
2. 安装 **Seeed SAMD Boards 1.8.6**，选择 **Seeeduino Wio Terminal**，FQBN 为 `Seeeduino:samd:seeed_wio_terminal`。
3. 默认启用 SD、Wi-Fi 和 IR，安装 `Seeed Arduino rpcWiFi@1.1.0` 及其依赖，以及 `IRremote@4.4.1`。rpcWiFi 的依赖包括 Seeed Arduino FS、SFUD、rpcUnified 和 mbedtls。
4. 从 Arduino IDE 的 NICE_GFX 示例菜单打开 `08_Applications → WioFieldLab`，或直接打开本目录的 `WioFieldLab.ino`，编译上传。板载屏幕、传感器、按键和红外无需外接线。
5. 串口波特率为 **115200**。复制整个 `WioFieldLab` 文件夹到其他草图目录也能编译，仍需安装 NICE_GFX 与启用功能所需的库。

从工作区根目录使用 Arduino CLI：

```powershell
arduino-cli compile --fqbn Seeeduino:samd:seeed_wio_terminal --library nice_gfx nice_gfx/examples/08_Applications/WioFieldLab
arduino-cli upload --fqbn Seeeduino:samd:seeed_wio_terminal -p COM12 nice_gfx/examples/08_Applications/WioFieldLab
```

将 COM12 改为设备实际端口。修改 [Config.h](Config.h) 可关闭 `WIO_ENABLE_SD`、`WIO_ENABLE_WIFI`、`WIO_ENABLE_IR`；三个开关均设为 `0` 时，只需 NICE_GFX 和 Seeed SAMD 核心即可编译实时仪表。生成资源需要支持 UI ABI 17 与透明色键的当前 NICE_GFX 库。

## 页面与操作

| 页面 | 功能 |
| --- | --- |
| HOME | 实时光照 ADC、三轴加速度、六个应用入口 |
| LEVEL | 静态双轴倾角、平滑气泡与拖尾、ZERO 校零反馈 |
| SOUND | 麦克风相对包络、扫描游标、HOLD/RESUME、峰值复位 |
| RECORDER | microSD CSV 记录、重试、记录流点与行数 |
| INFRARED | 三组有限 NEC 发送和扩散反馈 |
| RADIO | 异步 Wi-Fi 扫描、固定圆心雷达与五级拖尾 |
| SETTINGS | 背光、蜂鸣器测试与静音 |

五向方向键移动焦点，中键或顶部 **C** 确认；顶部 **B** 返回，**A** 回首页。顶部实体键从左到右为 **C SELECT / B BACK / A HOME**。Wio Terminal 没有板载触摸屏。

## 文件与硬件

| 文件 | 用途 |
| --- | --- |
| `WioFieldLab.ino` | 标准 Arduino 入口 |
| `FieldLab.h` | 传感器、记录、红外、无线与界面业务 |
| `ui.h` | 已编译界面、字体、图标与透明 RGB565 素材 |
| `Hardware.h` | 显示预设、旋转、按键与 16 行缓冲 |
| `ButtonAdapter.h` / `TouchAdapter.h` | 按键去抖、焦点与刷新调度 |
| `Buzzer.h` | 蜂鸣器初始化保护，避免首次发声前调用 noTone 卡死 |
| `WifiSurvey.h` | SSID UTF-8 保留和信号排序 |
| `WioPerf.h` | 串口性能测量与内存报告 |
| `Config.h` | 功能开关、NEC 地址与命令 |
| `data/project.gfx-project.json` | 可编辑界面源项目，固件运行时不读取此文件 |

LCD 使用 SPI3 / SERCOM7，SD 使用 SPI2，LIS3DHTR 使用 Wire1，并探测 0x18 / 0x19 与器件 ID。背光引脚为 72，旋转为 3。应用不依赖 TFT_eSPI。

光照与声音是未经 lux/dB 标定的相对 ADC 数值，声音曲线是包络；HOLD 时仍继续采样。LIS3DHTR 仅提供加速度，倾角适用于静止或缓慢运动；将设备放在参考平面后按 ZERO。

SD 每次创建新 `WIO000–999.CSV`，不覆盖已有文件；每五行 flush，STOP 关闭文件后再移除卡。无卡时其他页面仍可使用。IR 地址 `0x00`、命令 `0x45/0x46/0x47` 为测试配置，需按接收设备修改；本机没有红外接收与学习能力，单帧发送约 68 ms。

Wi-Fi 仅手动请求时异步扫描，期间仍可采样和切页；完成、失败或超时会停止雷达。SSID 优先显示具名网络并按信号排序，保留最多 32 字节的完整 UTF-8 字符；超出字体子集的字符需重新导出字库。背光、静音和校零值保存在 RAM，重启后恢复默认。

## 串口诊断

- `0`–`6` 切换页面；`n/a/b/c` 注入逻辑下一项/确认/返回/首页。串口 `a` 对应实体 C 确认。
- `v` 输出遥测；IR 页附发送状态与动画数量。`IR_TX` 给出发送耗时，`KEYS` 单独报告实际 GPIO 边沿。
- `m` 预热后测量约六秒已完成的 UI 局部刷新；`s` 在 RADIO 页启停明确标注的纯动效预览，不扫描网络。
- `p` 读取全屏 GRAM RGB888；`P` 读取全屏 RGB565 RLE；`r` 读取 128×61 雷达区域 RGB565 RLE。RLE 为文本头后接 `[count 1..255, RGB565 高字节, 低字节]`，解码到声明像素数结束。读回期间暂停绘制，不用于连续帧率测量。

## 维护与素材

`data/project.gfx-project.json` 可选导入 nice_gfx_editor 编辑；当前固件直接使用本地 `ui.h`，编译和运行均不访问编辑器。编辑器仓库的 `scripts/export-wio-project.mjs` 可重新同步界面，修改时保留业务元素 ID 和回调。

叶子/芯片为生成图的透明 RGB565 切图，12544 个像素中有 8470 个透明像素，仅增加两字节共享色键。Seeed 字标来自官方 [品牌素材](https://files.seeedstudio.com/wiki/Seeed_Studio_LOGO.zip)，保留比例和留白；商标属于其所有者，本示例不是 Seeed 官方应用。
