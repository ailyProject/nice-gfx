# 编译型 HTML/CSS 触摸界面

示例位于 NICE_GFX 库的标准 `examples/` 目录。使用 Arduino CLI 时，从工作区根目录运行 `arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 --library nice_gfx nice_gfx/examples/HTML_UI/GfxTouchDemo`；在 Arduino IDE 中安装 NICE_GFX 后从示例菜单打开本例。

本例使用 XIAO ESP32S3 + Round Display（GC9A01 240×240、CHSC6X），接线来自库内已有示例。安装 NICE_GFX，Arduino IDE 选择 XIAO_ESP32S3，打开 `GfxTouchDemo.ino` 编译。

当前 `ui.h` 由统一编译器生成，适配 NICE_GFX 1.0.0 / UI ABI 15。

界面包含中文文本、亮度滑块、双页滑动分页、设置路由、复选框与返回按钮；设置页可从空白左边缘右滑返回。

四份主源码是 `ui.h`、`GfxTouchDemo.ino`、`Hardware.h`、`TouchAdapter.h`。`ui.h` 只定义界面资源、路由与动作绑定辅助函数；主程序中的 `AppData` 保存亮度和通知状态，回调通过 `context` 更新数据，并调用 `setText` / `setValue` 更新当前页。滑动亮度滑块会更新标题，切换复选框会更新通知文字；示例不会改变背光 PWM 或外部执行器。

```cpp
struct AppData {
  int32_t brightness = 60;
  bool notificationsEnabled = true;
};
// 完整定义见 GfxTouchDemo.ino。
void on_brightnessChanged(void *context, nice_gfx::htmlui::Event &event) {
  auto &data = *static_cast<AppData *>(context);
  data.brightness = event.value;
  refreshUi(data);
}
// setup 中只绑定一次；布局或元素 ID 改变时由 ui.h 更新具体绑定目标。
// ui_config::bind_brightnessChanged(ui, on_brightnessChanged, &appData);
```

`setText` / `setValue` 不会递归触发 `input` / `change`，也不会自动修改 `AppData`。它们只查找当前页面，因此示例还监听全局 `RouteChange`，每次切换后用应用数据刷新进入的页面。界面内 `data-route` / `data-action` 的导航本身不需要业务回调。

`data/project.gfx.json` 可直接导入设计器。HTML/CSS 已离线编译进 `ui.h`，Arduino IDE 编译本例不需要 Node.js。后续修改布局、样式、文案或路由时，单独下载并替换 `ui.h` 即可，保留自己修改的 `.ino`。新增或删除动作时须更新主程序绑定；改变屏幕或触摸硬件时还需核对 `Hardware.h` / `TouchAdapter.h`。不要反复用完整工程 ZIP 覆盖业务代码。

本例使用安装到 Arduino IDE 的 NICE_GFX 库；设计器完整 ZIP 则自带 `src/nice_gfx` 运行库，两种方式使用同一份 `ui.h`。头文件采用 C++11 兼容的单例资源与硬件对象，可由多个业务 `.cpp` 引入。

维护本仓库示例时，在工作区根目录运行 `node tools/generate_html_example.mjs`，它从本例的 `data/project.gfx.json` 重新生成四份源码，并恢复上述示范业务逻辑。该脚本会覆盖本仓库示例的主程序，不应用来更新你的业务工程。
