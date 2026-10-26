# Arduino 组件交互

设计器把 HTML/CSS 编译为 `ui.h`，`nice_gfx::htmlui::View` 负责控件状态、事件与绘制；业务数据、存储、通信和外设由应用维护。升级时重新导出完整工程，同步运行库、`ui.h` 与适配器，保留业务主程序。资源兼容性以 [Compiled.h](../src/UI/Compiled.h) 和生成头文件的断言为准。

## 接入与最小示例

完整导出包含 `Hardware.h`（显示配置）、`ui.h`（界面资源）、`TouchAdapter.h`（初始化与 `pollUi()`），按需生成按键和地图适配器。运行库随工程附带，业务代码写在主程序中。

主程序按下例顺序包含生成文件。使用已安装的 Arduino 库时，先包含 `<NICE_GFX.h>`，供构建工具发现库。

在首页 `/` 放置滑块、进度条和读数：

```html
<main style="width:100%;height:100%;padding:8px;gap:8px">
  <input id="level" type="range" min="0" max="100" value="50"
    data-on-input="levelChanged">
  <progress id="meter" max="100" value="50"></progress>
  <p id="reading">50 %</p>
</main>
```

主 `.ino`：

```cpp
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"
#include <cstdio>

nice_gfx::htmlui::View ui(displayInstance(), kTilePixels);
bool uiReady = false;
struct AppData { int32_t level = 50; } app;

void refreshUi() {
  char text[24];
  std::snprintf(text, sizeof(text), "%ld %%", static_cast<long>(app.level));
  ui.beginUpdate();
  ui.setValue(ui_config::id("level"), app.level);
  ui.setValue(ui_config::id("meter"), app.level);
  ui.setText(ui_config::id("reading"), text);
  ui.endUpdate();
}

void levelChanged(void *context, nice_gfx::htmlui::Event &event) {
  static_cast<AppData *>(context)->level = event.value;
  refreshUi();
}

void setup() {
  if (!beginDisplayHardware()) return;
  if (ui_config::load(ui) != nice_gfx::Status::Ok) return;
  if (!ui_config::bind_levelChanged(ui, levelChanged, &app)) return;
  refreshUi();
  uiReady = true;
}

void loop() {
  if (uiReady) uiReady = pollUi(ui, millis());
  delay(1);
}
```

先改应用数据，再更新展示。`setText/setValue` 不触发 Input/Change；多页项目在 RouteChange 中刷新当前页。完整工程见 [GfxTouchDemo](../examples/07_UI/GfxTouchDemo/GfxTouchDemo.ino)。

## ID 与事件

- 给业务访问的元素设置工程内唯一 ID，调用 `ui_config::id("level")`，不加 `#`。getter/setter 只查当前页；节点缺失时 setter 返回 false。
- `setText()` 复制字符串；`text()` 返回借用指针，文字更新、重新挂载或销毁后失效。
- `data-on-click/input/change/submit/cancel/longpress/pagechange/animationend` 生成 `bind_动作名()`，具体名称以 `ui.h` 为准。同名动作绑定一次即可。
- 在 load 成功后绑定一次，避免放入 loop 或重复追加监听。回调 context 须长期有效，Event 只在回调内使用。

也可直接监听：

```cpp
void clicked(void *, nice_gfx::htmlui::Event &event) {
  // event.target：来源节点；event.optionIndex：选项索引，无选项时为 -1。
}
void routeChanged(void *, nice_gfx::htmlui::Event &) {
  // 按 ui.path() 从应用数据刷新当前页。
}
// setup 中：
// ui.on(ui_config::id("save"), nice_gfx::htmlui::EventType::Click, clicked);
// ui.on(0, nice_gfx::htmlui::EventType::RouteChange, routeChanged);
```

| 事件 | 使用要点 |
| --- | --- |
| Click / LongPress | 点击 / 长按；可用 target、component、optionIndex 区分来源 |
| Input / Change | 拖动持续发 Input，释放且值改变时发 Change；程序 setter 不触发这两者 |
| Submit / Cancel | 确认 / 放弃输入；确认值未改变也可发 Submit |
| Scroll / PageChange | 滚动 / 页内分页；Pager setter 也可能通知 |
| BeforeNavigate | 可取消导航，也可用 `setRouteGuard()` 否决 |
| RouteChange / NavigationCancel | 路由提交 / 取消，用 ID 0 监听 |

普通事件按捕获、目标、冒泡传播，Scroll 与路由事件不冒泡。`preventDefault()` 仅取消可取消事件的默认动作；Input/Change 等通知不可取消。数值、文字、显隐可在回调中更新；打开输入、focus 等操作宜排队到 loop，节点回调中的导航在处理后提交。

## 组件接口速查

| 组件 | 常用接口 |
| --- | --- |
| 文字、按钮 | `setText/text`、`setEnabled` |
| Checkbox / Switch | `setValue/value`，0 / 1 |
| Range / Progress / Gauge / Ring / Spinbox | `setValue/value`；Spinbox 支持加减、长按和数值键盘 |
| InputField | `setText/text`，数值型另支持 `setValue/value` |
| LineChart / BarChart | `setChartSeries/setChartValue/appendChartValue/chartValue` |
| Media / Map | 见 [动态图片](DynamicImages.md)；地图使用独立扩展 |
| Scroll / Pager | `scrollTo(id,x,y)` / `setPage/pageIndex`；页码从 0 开始 |
| 容器、图形、图标 | 显隐、样式与动画；形状和图标在编译时确定 |

封装组件使用作者设置的根 ID：SettingRow、ValueSlider 代理数值；ImageButton 代理标题；Tabs 代理 Pager；Select/Choice 读写选项索引；Select、Popup、Confirm 通过 `setExpanded/expanded` 控制展开。选项 Click 的 `optionIndex` 表示意图，提交后的 Input/Change 用 `value` 表示索引。

## 声明式联动

展示联动写入 `data-ui-*`，业务动作写入 `data-on-*`：

```html
<input id="level" type="range" min="0" max="100" value="50"
  data-ui-input='[{"action":"value","target":"meter","value":"$value"},{"action":"format","target":"reading","value":"{value} %"}]'
  data-on-change="applyLevel">
<progress id="meter" max="100" value="50"></progress>
<p id="reading">50 %</p>
```

动作包括 `text/append/backspace/format/value/increment/decrement/toggle/visible/enabled/page/animate/show/hide`。每个事件最多 16 项，目标必须同页；format 恰好包含一个 `{value}`。程序 setter 不执行声明式 Input/Change 联动，应用应同步关联展示。

## 输入与图表

`gfx-input` 支持 text/number/password/pin/tel。number 的 `data-decimals` 为 0–6，Spinbox 为 0–3。键盘不含中文输入法，中文初值和动态文字须提前导出字形。

| API | 含义 |
| --- | --- |
| `beginTextInput(id)` | 打开共享编辑会话 |
| `inputDraft()` | 读取草稿；`text/value` 仍是确认值 |
| `inputText/backspaceInput/deleteInput/moveInputCursor` | 按 UTF-8 字符边界编辑 |
| `commitTextInput()` | 校验并提交；失败保留草稿 |
| `cancelTextInput()` / `hideTextInput()` | 放弃草稿 / 收起键盘并保留草稿；离页取消会话 |
| `setKeyboardTheme()` / `resetKeyboardTheme()` | 设置 Light / Dark，或恢复字段配置 |

Gauge/Ring/Spinbox、数值 InputField 和图表使用整数定点数：`data-decimals=1` 时，C++ 的 235 表示 23.5，HTML 的 value/min/max 仍写小数。InputField 的 `text()` 是确认字符串，Spinbox 的 `text()` 是标题。

```cpp
constexpr auto chartId = ui_config::id("temperature-chart");
int32_t samples[] = {231, 235, 240};
ui.setChartSeries(chartId, 0, samples, 3);
ui.appendChartValue(chartId, 0, 242);
ui.setChartValue(chartId, 0, 1, 236);
```

图表复制样本，最多 4 个序列，每序列容量 2–512，满后淘汰最旧样本；`INT32_MIN` 表示缺失。`setValue()` 不更新图表样本。

## 导航、输入与调度

`push/replace` 使用工程中存在的路径，`back/home` 返回历史页 / 初始页。导航成功不表示转场或传输完成，新页内容在 RouteChange 中刷新。已访问页保留控件状态；`releasePage(path)` 可释放未被导航或在途帧引用的非活动页。

实体按键使用生成的 ButtonAdapter 或 `inputButton()`，逻辑动作使用 `inputAction()`；`focus(id)/focused()` 管理焦点。

生成工程通常使用 Coordinated 交互模式；直接 `mount()` 或未声明版本的资源使用 Legacy。手工接入用 `configureInteraction()` 配置，`GesturePolicy` 控制滚动、模态层与导航归属。自定义拖动在 PointerDown/Move 中申请 `claimPointer()`，在 GestureStart/Update/End 中修改状态并处理 GestureCancel。传播与手势完整约定见 [UI 运行库](../src/UI/README.md)。

`setVisible()` 立即显隐，`show/hide` 可播放声明动画；隐藏不重排兄弟节点。`setStyle()` 修改支持的样式，`setBounds()` 修改布局矩形，`layoutChildren()` 只排列直接子项。

所有 View API 从同一 UI 任务调用。后台任务或 ISR 通过队列交付数据，耗时操作分步执行；持续调用 `pollUi()` 或 `tick()` 推进输入、动画与显示刷新。`Status::Busy` 表示仍有工作。

`beginUpdate/endUpdate` 合并绘制调度，setter 立即生效且不提供回滚。`finish()` 仅排空当前快照，不推进未来动画。完整 UI 需要 STL 和足够动态内存；预算见 [渲染性能](RenderingPerformance.md)。

更多接口见 [View.h](../src/UI/View.h)；标签与属性见 [HTML/CSS 支持](../../nice_gfx_editor/packages/html-ui/HTML_CSS_SUPPORT.md)；地图接入见 [地图组件](../../nice_gfx_editor/docs/map-component.md)。