# 动态图片示例

默认硬件为 OJoy ESP32-S3 / CH13613 480×480 / CHSC6417 触摸，使用当前设计器的实际硬件预设。按自己的设备配置修改并重新导出 Hardware/TouchAdapter；页面尺寸必须与屏幕一致。

- Generate：约每 100 ms 生成一次 128×96 RGB565 图，setImage 复制后立即清零原数组，用于展示调用方可安全复用输入。
- Resource：同一个组件切换预置资源，包括未用作初始图的 demo-stripes；不复制整图到 RAM。
- Clear：清空当前像素，保留图片组件几何。
- Reset：恢复作者初始 checker 图片。

该示例不下载文件，不内置 JPEG/PNG 解码器，也不保证实际屏幕达到某个帧率。View 负责图片状态、分块绘制及在途帧所有权；下载/解码由应用实现。

在已安装新版 NICE_GFX 库的 Arduino 环境打开 DynamicImageDemo.ino。工程源保存在 data/project.gfx.json，可导入设计器。复现生成文件：

```powershell
node tools/generate_dynamic_image_example.mjs
```

详见 [动态图片 API 与内存契约](../../../docs/DynamicImages.md)。
