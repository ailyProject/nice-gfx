# 使用文档

第一次使用？从 [中文入门](../README_ZH.md#快速开始) 开始，再选择 [可运行示例](../examples/README.md)。

## 连接屏幕

| 需要做什么 | 文档 |
| --- | --- |
| 查询彩色屏、触摸芯片与构造参数 | [设备支持](DeviceSupport.md) |
| 使用固定开发板接线 | [开发板预设](BoardPresets.md) |
| 使用 SSD1306、SH1106 或指定模组的电子纸局刷 | [单色屏与电子纸](MonochromeAndEpaper.md) |
| 按原 u8g2 型号选择屏幕 | [U8g2 兼容](U8g2Compatibility.md) |
| 使用 GxEPD2 系列电子纸、多色或灰阶面板 | [电子纸兼容](EpaperCompatibility.md) |
| 使用 Seeed 墨水屏与板级供电配置 | [Seeed 墨水屏](SeeedEpaper.md) |

## 开发界面

| 需要做什么 | 文档 |
| --- | --- |
| 绑定事件、更新数据、切换页面 | [组件交互](ArduinoComponentInteraction.md) |
| 在运行时更新图片 | [动态图片](DynamicImages.md) |
| 选择字体、查看图样、生成中文子集 | [字体](font.md) |
| 配置缓存、内存预算与性能统计 | [渲染性能](RenderingPerformance.md) |
| 了解源码分工、ESP-IDF 构建与配置维护 | [架构](Architecture.md) |

开发板完整接线见 [BoardPresets.json](BoardPresets.json)；资源生命周期和完整 UI 约定见 [UI 运行库](../src/UI/README.md)。