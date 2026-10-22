# Wio Terminal 硬件测试与性能优化

测试日期：2026-10-22（Asia/Shanghai）。实机：COM12，Seeeduino Wio Terminal / SAMD51 / ILI9341。

官方 main/HEAD 经 git ls-remote 核对为 [bbd9b097](https://github.com/ailyProject/nice-gfx/commit/bbd9b0977b4815d972999e1a21589f29f32fb283)，与本地 HEAD 一致。
本次以已有工作区改动为基础，没有覆盖或清理这些改动；“基线”指优化前的工作区版本。
库版本 1.0.0，资源 ABI 17，Seeed SAMD core 1.8.6，Arduino CLI 1.5.1，ARM GCC 7-2017q4，gnu++14 / -Os。

## 实测结果

两版均使用 320×240、rotation=3、40 MHz 请求频率、162 px 内容区、2048 像素 UI tile。
每个场景预热 750 ms，再测量约 6 秒；每版完整运行三轮。表内 FPS 与最近 256 帧 P95 均取三轮中位数。
这是绘制及提交完成速率，包含 HUD 和调度开销；不是面板扫描刷新率。
SCK 未使用逻辑分析仪测量，40 MHz 表示请求值，实际分频由 Seeed core 决定。

| 场景 | 基线 FPS | 优化 FPS | 变化 | P95 ms，基线 → 优化 |
| --- | ---: | ---: | ---: | ---: |
| Fill / RGB565 | 67.130 | 69.082 | +2.9% | 13.731 → 13.384 |
| Lines / clipping | 4.212 | 10.210 | +142.4% | 303.629 → 136.825 |
| Shapes / antialias | 7.569 | 17.026 | +124.9% | 168.169 → 96.693 |
| Text / UTF-8 | 13.450 | 13.936 | +3.6% | 111.815 → 108.018 |
| Gradients | 18.211 | 18.874 | +3.6% | 92.281 → 89.833 |
| Sprite / animation | 55.826 | 55.981 | +0.3% | 16.427 → 16.437 |
| UI / widgets | 44.752 | 45.134 | +0.9% | 21.044 → 20.914 |
| UI / pointer input | 95.692 | 96.481 | +0.8% | 16.901 → 16.904 |
| UI / transitions | 39.090 | 39.810 | +1.8% | 100.301 → 97.848 |

54 条性能结果全部为 OK，预热和测量错误均为 0。UI 输入测试每轮有实际 actions/events；
UI、Sprite 场景的小幅提升不作显著性结论。Sprite 和指针输入的 P95 基本持平，未宣称所有延迟指标均改善。

## 实现变化

- [Bus.cpp](../src/Platform/Arduino/Bus.cpp)：小图元仅初始化实际使用的填充像素，避免每个像素都初始化整个传输缓冲。
- Wio 的填充/Native 像素转换 staging 从 384 增至 1536 字节，摊薄同步 DMA 启动成本；Wire 格式仍可直接传输。
- 16 字节以内的 Wio SPI 写入使用轮询，其余保留 Seeed core 的同步 DMA。实测 100 次 4 字节调用为轮询 113 µs / DMA 333 µs；16 字节为 434 / 516 µs，32 字节则 DMA 更快（860 / 749 µs）。
- [FastPin.h](../src/Platform/Arduino/FastPin.h)：SAMD 通过 variant 物理引脚映射缓存 OUTSET/OUTCLR，使用原子寄存器写控制 CS/DC。
- [BenchmarkStats.h](../examples/10_Performance/Graphics_Benchmark/BenchmarkStats.h)：64 位计数先转十进制字符串，修复 newlib-nano 不支持 %llu 导致 CSV 后续列错位的问题。

两版都先应用统计格式修复再采集正式结果，避免把统计变化计为库优化。小包阈值来自核心 SPI API 的实机测量。
本轮未增加异步 API 能力：SYNC/UI-SYNC 表示调用方仍等待提交完成；底层同步 DMA 与异步提交不是同一指标。
一次扩大缓冲但仍全量初始化的候选出现小图元退化，已修正，未纳入最终对照表。

基准程序存储占用 279028 → 279140 字节（+112），data=1824、bss=9624 字节均保持不变。
填充/转换路径单次调用的 staging 栈空间增加 1152 字节，不增加持久 heap；更深调用栈或 RTOS 小栈任务需要将此计入预算。

## 正确性与稳定性

最终烧录固件通过 **97663 项硬件断言，0 失败**，包括：

- 八种扫描方向，直接绘图与 Sprite 参考图的整行逐像素 GRAM 读回对照。
- 图元、抗锯齿圆、渐变、文字、边界、裁剪、RGB565 颜色和 Flash 常量图片。
- Native/Wire 字节序、stride、非对齐源、轮询和 staging 边界，无效参数拒绝及后续提交恢复。
- 1/4/8/16 位 Sprite、滚动、透明提交与 153600 字节全屏图片，覆盖 65535 字节 DMA 描述符分段边界。
- 2000 次不同尺寸/色深的 Sprite 分配释放，已分配 heap 从 856 回到 856 字节。

优化前的较早版硬件用例也通过 94440 项断言。最终版本增加了非对齐源和边界覆盖。
主机完整回归通过（含 UBSan、内存失败注入、View 像素一致性、帧生命周期、板级预设、构建配置和新增加的 SPI/统计回归）。
实际 Wio SDK 的板级构造编译和不支持板型诊断检查通过。

GRAM 校验验证的是显示存储内容。可见画质、撕裂、背光亮度、实体按键及外接触摸未进行人工验收；
Wio 原机没有触摸控制器，UI 输入使用正式 pointer API 自动注入。本次不包含 Wi-Fi/SD/传感器功能测试或长时间老化。
基准中的 heap_min/psram_free=4294967295 表示无可靠平台 API，不能作为可用内存；分配压力检查单独使用 mallinfo。

## 原始证据与复现

- [基线三轮 CSV](../tests/hardware/wio-terminal/baseline.csv)、[优化三轮 CSV](../tests/hardware/wio-terminal/optimized.csv)、[统计及范围](../tests/hardware/wio-terminal/comparison.json)。
- [最终硬件回归与 SPI 原始计时](../tests/hardware/wio-terminal/validation.txt)、[早期基线回归](../tests/hardware/wio-terminal/validation-baseline.txt)、[主机回归日志](../tests/hardware/wio-terminal/host-tests.txt)。
- [环境、固件和源码 SHA-256](../tests/hardware/wio-terminal/environment.json)。
- [自动硬件用例](../examples/10_Performance/WioTerminal_Validation/README.md)。

在库根目录运行（Arduino CLI 不在 PATH 时传 -ArduinoCli 完整路径）：

```powershell
.\tools\wio-terminal-test.ps1 -Mode Validation -DataDirectory ..\.wio-tools\data
.\tools\wio-terminal-test.ps1 -Mode Benchmark -Rounds 3 -Rotation 3 -DataDirectory ..\.wio-tools\data
node tools/compare-wio-benchmarks.mjs --baseline=baseline.txt --optimized=optimized.txt --out=results
```

默认输出保存在 .build/wio-terminal。最后一次实机烧录保留通过验证的固件；串口 115200，发送 r 可重跑。
