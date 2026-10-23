# Wio Terminal 自动硬件回归

选择 `Seeeduino:samd:seeed_wio_terminal`，使用工作区的 NICE_GFX 编译并上传。
串口为 115200 baud；连接后自动运行，发送 `r` 可重新执行。

程序通过 ILI9341 的 MISO 读回真实 GRAM，并检查：

- 八个扫描方向的尺寸、边界及直接绘图与 Sprite 参考图的逐像素一致性。
- RGB565 颜色、Flash 常量图片、Native/Wire 字节序、带 stride 图片与视口裁剪。
- 无效参数拒绝及后续有效提交恢复。
- 1/4/8/16 位 Sprite、滚动、透明提交及跨越 65535 字节 DMA 描述符边界的全屏图片。
- 2000 次不同色深和尺寸的 Sprite 分配/释放后，`mallinfo().uordblks` 是否回到起点。

`PERF_RAW` 比较 Seeed SPI core 的逐字节轮询和同步 DMA 调用开销；测量时 CS 保持高电平。
`VALIDATE_DONE checks=... failures=0` 表示自动检查通过。串口会输出最先发生的 12 个错误。
此程序结束后显示通过/失败画面。

UI 控件、脚本指针输入、转场与连续性能测试使用相邻的
[Graphics_Benchmark](../Graphics_Benchmark/README.md)。实体按键、可见画质和面板扫描率
需另行人工观察，不属于 GRAM 读回断言。
