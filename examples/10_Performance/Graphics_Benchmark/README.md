# Graphics_Benchmark：综合绘图与 UI 性能测试

九个场景、统计和 UI 调度共用于原生 [ESP-IDF 对照示例](../../../../nice_gfx_idf_examples/examples/34_graphics_benchmark/README.md)。核心逻辑和测量参数位于 `BenchmarkApp.h`，Arduino 的计时、串口和内存接口由 `BenchmarkPlatformArduino.h` 提供。

从 Arduino IDE 的 **NICE_GFX → 10_Performance → Graphics_Benchmark** 打开。
示例自动运行九个场景，在屏幕底部显示完成帧 FPS、累计帧数、平均/P95/最大帧间隔、
绘制及 I/O 耗时、空闲内存和观察到的错误。每个场景预热 750 ms，再测量 6 秒；
一轮结束显示汇总，12 秒后重新运行。无需触摸屏即可完成自动 UI 操作。

## 硬件和运行

- 使用支持原生 UI 的 32 位 Arduino 开发板，显示器旋转后的尺寸至少 240×240。
  原生 UI 使用 C++ 标准库和动态内存；不适用于 AVR。
- 主程序使用 `ExampleDisplay gfx`，接线遵循
  [Hardware.h](Hardware.h)。ESP32-S3 默认是
  **CH13613 480×480 QSPI**：CS 45、reset 7、clock 17、D0…D3 为 8/16/15/18。
  经典 ESP32 默认 ILI9341：CS 5、DC 2、reset 4、SCK 18、MOSI 23、MISO 19。
  这些是示例引脚，必须与实际模块一致。其他屏幕可直接替换主程序的显示器对象和构造参数。
- 顶部 `kRotation` 可修改方向；`kWarmupMs`、`kMeasureMs` 可调整测试时间。
- 不需要 SD 卡、网络或外部图片。支持单像素窗口的屏幕不需要 PSRAM。Sprite 最大为 192×120 RGB565，
  分配失败时逐级缩小；仍失败则标记 `SKIP`，不会算作完成帧。
- 如需真实触摸，将 `kAttachTouch` 改为 `true`，并先调整
  `exampleAttachTouch()` 的控制器和引脚、校准触摸坐标。默认关闭；ESP32-S3
  示例中的外接 XPT2046 接法不代表 CH13613 自带该控制器。
  真实触摸可用于 widgets/transitions 场景；自动 pointer input 场景独占指针。

在工作区根目录编译（替换开发板 FQBN）：

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32s3 --library nice_gfx "nice_gfx/examples/10_Performance/Graphics_Benchmark"
```

Wio Terminal 使用 `Seeeduino:samd:seeed_wio_terminal` 编译。
`tools/wio-terminal-test.ps1 -Mode Benchmark -Rounds 3` 自动烧录、采集三轮并保存串口记录，
默认旋转 3（320×240），可用 `-Rotation` 修改。SAMD SPI 后端保持同步提交模式；
底层 Seeed SPI core 可使用同步 DMA，这与公开的异步提交能力是不同指标。
64 位计数先转为十进制字符串，兼容 Seeed 工具链的 newlib-nano 格式化实现。

上传后以 **115200 baud** 打开串口。单字符命令无需换行：

| 命令 | 操作 |
| --- | --- |
| `n` | 跳过当前场景；在汇总画面开始新一轮 |
| `p` | 暂停；再次按下从当前场景预热重新开始 |
| `r` | 清除本轮结果，从第一项重新开始 |
| `d` | 切换 Sprite 场景的 DMA/同步提交，清除本轮结果并重新开始 |

CH13613 等要求窗口按 2×2 像素对齐的屏幕，前五场景在一块全屏 RGB565 Sprite 中绘制，再提交完整内容区，标记为 `BUFFERED`。480×480 缓冲约占 450 KiB，建议启用 PSRAM；分配失败会报告 `BENCH_FATAL`。HUD 和汇总文字共用该缓冲，Sprite 动画的提交位置按窗口网格对齐，UI 继续使用原有分块刷新。

模式按实际路径显示：支持直接绘图时为 `DIRECT`，对齐屏幕离屏绘图为 `BUFFERED`；Sprite 的 `DMA` 只有 `initDMA()`
成功时才显示，否则为 `SYNC`。UI 调度器自行请求异步提交，按后端能力显示
`UI-ASYNC` 或 `UI-SYNC`，不受 `d` 控制。每帧均等待完成，因此这是逐帧完成测量。
UI 内部仍可分块交叠绘制和传输；
Sprite 使用一块缓冲并逐帧排空，没有跨帧双缓冲流水线。

## 测试内容

| 场景 | 实际工作 |
| --- | --- |
| Fill / RGB565 | 整个内容视口进行 RGB565 单色填充并轮换颜色 |
| Lines / clipping | 清屏，绘制 48 条含越界端点的线和边框，检查裁剪效果 |
| Shapes / antialias | 实心三角形、平滑圆、平滑圆角矩形及动态圆弧 |
| Text / UTF-8 | 不断更新多行文本，切换 6×13 和 16 px 字体，用微型后备字模显示 UTF-8 温度符号 |
| Gradients | 水平/垂直渐变和移动圆形 |
| Sprite / animation | 在 RGB565 Sprite 上绘制网格及八个运动粒子，提交并等待传输完成 |
| UI / widgets | 使用真实 `View` 更新文本、进度条、滑块、开关和折线图 |
| UI / pointer input | 分步注入实际 pointer down/move/up，点击按钮/开关、拖动滑块和列表；验证回调与值变化 |
| UI / transitions | 在两页之间切换，循环滑动/淡入转场，同时运行原生 fade/float/pulse 动画 |

UI 资源是方便阅读和修改的手写 `ProjectDesc` 夹具，位于 `BenchmarkUi.h`，
使用正式的原生资源 ABI 和公共 `View` API。页面物理尺寸与屏幕一致，组件区域
裁剪在底部 HUD 之上。无需设计器导出文件。场景尺寸、字体、总线和提交模式
会影响结果，比较不同板卡时应保持一致；各场景工作量不同，不生成混合总分。

## 指标口径

- **FPS** = 测量阶段成功完成帧数 ÷ 实际墙钟时间。包括 HUD、串口处理、
  `yield()` 和 UI 空闲时间；预热和场景初始化不计入。
  UI 使用 `runtimeStats().completedFrames` 的增量，空闲轮询不算作帧。
- **avg / max ms** 是从上一个成功完成帧至当前完成帧的间隔（首帧从测量起点计）。
  失败或空闲轮询造成的间隔会保留，不能用零工作把 FPS 抬高。
- **p95 ms** 使用最近最多 **256 个完成帧间隔**的 nearest-rank 第 95 百分位，
  不是整轮全部样本的 P95；平均值和最大值覆盖整个测量阶段。
- **draw / io+other ms** 是每个成功完成帧的平均工作耗时，不包含 HUD。
  `DIRECT` 的 `draw` 包含同步总线写入，`io+other` 主要是末尾等待。
  `BUFFERED` 的 `draw` 为内存光栅化，`io+other` 包含整个内容区的提交和等待。
  Sprite 的 `draw` 是内存光栅化，`io+other` 是 `flush()` 加 `wait()`。
  UI 的 `draw` 是 `renderMicros` 增量；其余时间包含 setter、输入、布局管理、
  提交及等待，不能当作纯总线时间或 CPU 使用率。
- **heap / min / PS** 单位为 KiB。`min` 是当前测量阶段采样到的最低空闲 heap，
  不是启动以来的最低值。ESP32 分别报告内部 heap 和 PSRAM；ESP8266、
  Earle RP2040 报告 heap。没有可靠 API 的平台显示 N/A。CSV 中未知值为
  `4294967295`。UI 的 `ui_bytes` 仅是 `memoryStats().totalBytes()` 保留容量估计，
  不包括驱动、分配器元数据或 Flash 资源。
- **pixels** 对填充、`BUFFERED`、Sprite、UI 记录明确的像素工作量；`BUFFERED` 是每帧提交的内容区像素数，UI 包含 raster pixels
  和 direct blit pixels；其他直接绘图场景填 0，表示没有测量精确像素数。
  它不是总线字节数、面板扫描像素数或像素读回校验。
- **actions / events** 分别是脚本完成的操作和运行库实际投递的回调数。
  **err / observed_errors** 统计观察到的状态失败与脚本断言失败；
  驱动 `lastError()` 可被后续调用覆盖，因此不是底层所有 I/O 错误的完整日志。
  预热阶段错误单独保留为 `warmup_errors`。

这是**应用绘制和传输完成速率**，不能等同于面板物理 VSYNC/扫描刷新率。
HUD 每 500 ms 更新；转场重绘整个页面时立即恢复 HUD，因此其额外开销会反映在 FPS
和帧间隔中。计时使用无符号 32 位微秒差值，允许时钟自然回绕；单轮须短于约 71 分钟。

测量组织参考 [LVGL benchmark 的分场景统计](https://github.com/lvgl/lvgl/blob/master/demos/benchmark/lv_demo_benchmark.c)
和 [Khronos 对异步绘制计时的说明](https://wikis.khronos.org/opengl/Performance)：
预热、分开呈现绘制与提交成本，并在完成等待之后计帧。这些设计原则应用于本库，
没有移植其他库的评分或宣称跨框架结果可直接比较。

## 串口与验证

每轮输出 `BENCH_ENV` 硬件环境、`BENCH_CSV` 列名和各场景结果；
初始化/分配失败输出 `BENCH_SKIP`，人为跳过输出 `BENCH_ABORT`。
CSV 状态为 `OK`、`ERROR` 或 `NO_FRAMES`，不能将 SKIP/ERROR 当成性能通过。
记录固件版本、板卡/屏幕、供电、引脚、总线时钟和提交模式后，再比较多轮结果。

在 Arduino IDE 中选择实际目标板后编译并上传本示例。物理屏幕上应观察颜色/文字、
裁剪、动画、控件动作和汇总信息，并分别在 Sprite DMA/同步模式下保留串口结果。
