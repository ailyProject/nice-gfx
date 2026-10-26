# 渲染性能与内存配置

UI 使用分块绘制，并按显示能力选择缓存、帧缓冲合成和平台加速。升级库后重新编译；编辑器项目需同步运行库并重新导出。

## 测量与调度

| 接口 | 用途 |
| --- | --- |
| `runtimeStats()` | 查看绘制工作量与完成帧 |
| `memoryStats()` | 估算 UI 保留容量；不含分配器、驱动和任务栈 |
| `setProfilingClock()` | 启用分类微秒计时 |
| `configureRenderBudget()` | 设置每次调度的像素与时间预算 |

渲染预算在没有进行中的光栅帧时配置。像素预算须满足屏幕对齐要求，可能限制整帧加速；时间预算是软截止，单次驱动调用可能超出预算。

分别记录切页首帧延迟、后续帧间隔、冷/热缓存及 p95。`captureMicros`、`repairMicros` 包含内部绘制耗时，不能与其子项相加。面板扫描次数、UI 完成帧和帧缓冲交换次数应分别统计；静态页面没有损伤时无需持续绘制。

## 内存预算

- RGB565 帧缓冲占 `宽×高×2` 字节，双缓冲加倍。
- 自有 tile 在同步传输时使用一块，异步传输时使用两块；显式传入的缓冲由调用者管理。
- 编译节点、图片描述符和像素应保持只读静态资源；重新导出 `ui.h` 可同步资源生成方式。
- 动态图片切换可能短暂保留新旧两份，详见 [动态图片](DynamicImages.md)。

### 活动页资源缓存

`configureResourceCache(maxBytes, reserveInternalBytes)` 在 `mount/load` 前配置，库默认关闭。它最多缓存当前页与转场来源页的节点表；预算包含描述符目录，实际占用计入 `memoryStats().stateBytes`。预算不足或分配失败时使用 Flash 资源。

```cpp
// 在 ui_config::load(ui) 前调用；示例上限 96 KiB，保留 64 KiB 内部 RAM。
ui.configureResourceCache(96 * 1024, 64 * 1024);
```

第二个参数只在分配时检查空闲量，仍需为任务栈、业务数据和 DMA 预留空间。传入零关闭缓存。该接口缓存节点元数据；`configureRenderCache()` 使用调用者提供的像素页面缓存，两者独立。

## Korvo 显示时序

`Esp32S31Korvo1Display` 使用 800×480、26 MHz 时钟：H pulse/back/front 为 1/40/20，V 为 1/10/5。总量为 861×496，理论扫描率约 60.88 Hz；实际 UI 帧率取决于页面、传输与业务负载。

预设启用原子双帧缓冲，完整帧提交后才切换；第二个 RGB565 缓冲额外占 768,000 字节 PSRAM。配置见 [BoardPresets.h](../src/Display/Presets/BoardPresets.h)，通过 `gfx.copyStats()` 与 `gfx.scanStats()` 分别观察拷贝和扫描。其他板卡应使用自己的模组时序。

## ESP32-S31 可选参数

以下宏应作为全工程 C++ 编译选项设置，使 `View.cpp` 同时生效：

| 宏 | 默认值 | 作用 |
| --- | ---: | --- |
| `NICE_GFX_S31_MOTION_TEXTURE_BUDGET` | 0 | 只读纹理缓存上限；1048576 表示 1 MiB，0 关闭 |
| `NICE_GFX_S31_MOTION_STAGE` | 1 | 大图片使用内部行块暂存；0 使用普通 span 路径 |
| `NICE_GFX_S31_PPA_BLEND_BURST` | 128 | PPA 突发长度，可选 64 / 128 |

缓存和加速受格式、对齐及内存约束；不适用时使用常规绘制。配置后应在实际工程测量帧间隔、输入响应与内存峰值。