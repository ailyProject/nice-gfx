# Arduino 组件交互与联动

设计器把 HTML/CSS 编译为 `ui.h`，`nice_gfx::htmlui::View` 负责控件状态、事件与绘制；业务数据、存储、通信和外设由应用维护。升级时重新导出完整工程，同步运行库、`ui.h` 与适配器，保留业务主程序。资源兼容性以 [Compiled.h](../src/UI/Compiled.h) 和生成头文件的断言为准。

## 接入与最小示例

完整 Arduino 导出包含 `Hardware.h`（显示配置与 `displayInstance()`）、`ui.h`（资源与绑定）、`TouchAdapter.h`（初始化与 `pollUi()`），按配置生成 `ButtonAdapter.h` 和地图文件。`src/nice_gfx/` 是随附运行库，`data/` 保存可编辑工程，设备不解析 HTML/CSS。硬件预设见 [开发板预设](BoardPresets.md)。

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

## ID、绑定与事件

`ui_config::id("level")` 使用元素 ID，不加 `#`，不检查节点是否存在。ID 应在工程内唯一；getter/setter 只查当前页，缺失时 setter 返回 false，`text()` 返回空串，`value()` 返回 0。封装组件使用作者设置的根 ID，避免依赖生成的内部 ID；`ui_config::load()` 会安装组件链接。

`setText()` 复制字符串；`text()` 返回借用指针，更新文字、重新挂载或销毁 View 后不要继续持有。回调 context 必须长期有效，Event 仅在回调内有效。

`data-on-click/input/change/submit/cancel/longpress/pagechange/animationend` 生成 `ui_config::bind_动作名()`。同名动作可用于多个节点，一次绑定即可；函数名以 `ui.h` 为准。`bind_*` 和 `on()` 追加监听，应在 load 成功后绑定一次，不放在 loop 中；重新 load 也不要重复追加已有业务监听。

```cpp
void clicked(void *, nice_gfx::htmlui::Event &event) {
  // 用 event.target、event.component 或 event.optionIndex 区分来源。
}
void routeChanged(void *, nice_gfx::htmlui::Event &) {
  // 按 ui.path() 从应用数据刷新当前页。
}
// setup 中，load 成功后：
// ui.on(ui_config::id("save"), nice_gfx::htmlui::EventType::Click, clicked);
// ui.on(0, nice_gfx::htmlui::EventType::RouteChange, routeChanged);
```

| Event 字段 | 含义 |
| --- | --- |
| `type/phase`、`target/currentTarget` | 事件类型、传播阶段、原始目标与当前监听节点 |
| `component`、`optionIndex` | 公开组件根 ID（普通节点为 0）；选项意图索引（无选项为 -1） |
| `value`、`x/y`、`scrollX/Y` | 整数值、指针坐标、目标滚动偏移 |
| `from/to/routeAction` | 路由来源、目标与动作 |

普通事件按捕获、目标、冒泡传播；Scroll 不冒泡，路由事件用 ID 0 监听且不传播。`stopPropagation()` 停止后续节点，`stopImmediatePropagation()` 还停止当前节点其余监听。`preventDefault()` 仅取消可取消事件的默认行为，不回滚任意业务修改。

| 事件 | 关键语义 |
| --- | --- |
| Click | 可取消控件激活与声明式路由；Checkbox/Switch 监听先看到临时切换值，取消后回滚且不发 Input/Change |
| PointerDown/Move/Up | 可取消；具体默认行为取决于交互模式 |
| Input / Change | Range 拖动持续发 Input，释放值与起始值不同才发 Change；取消拖动不提交 Change |
| Submit / Cancel | 输入确认 / 放弃；Submit 允许确认值未改变 |
| Scroll / PageChange | 滚动偏移 / 页内分页通知；Pager 程序 setter 也可能产生通知 |
| LongPress / PointerCancel | 长按通知 / 接触取消；长按识别需要持续 tick |
| BeforeNavigate | 可取消导航；`setRouteGuard()` 也可同步否决 |
| RouteChange / NavigationCancel | 路由提交 / 取消通知 |

Input、Change、Scroll、PageChange、LongPress、PointerCancel 是不可取消通知。声明事件属性不保证节点产生事件，例如 Progress 没有用户数值编辑。`data-on-submit/cancel` 用于 `gfx-input`；Spinbox 对应事件使用 C++ `on()`。

## 交互模式与手势

新建工程使用 `interactionVersion: 2`（Coordinated）；缺省版本和直接 `mount()` 保持 Legacy。生成的 `ui_config::load()` 安装模式与策略，可用 `interactionVersion()` 查询。手工接入在 mount 后调用 `configureInteraction(InteractionVersion::Coordinated, policies, count)`；策略会复制，配置时不能处于接触或回调处理中。

Coordinated 在移动 8px 后区分方向，由本地控件、滚动区域和导航竞争归属，归属确定后保持到释放或取消。普通边缘导航让本地控件先处理；导航需越过控件时显式配置 `overrideControls`。`GesturePolicy` 可限制轴、指定 Local/Parent 归属、允许导航链、关闭命中或将可见面板设为模态层。

新版生成的业务绑定使用 `onScoped()`，按策略的 `EventScope::Subtree/Self/Component` 过滤事件。`on()` 保留原始传播语义；手写监听可用 `acceptsEvent(event)` 检查作用域。Coordinated 的 Click 声明式动作由 `onDefault()` 在普通监听之后执行，`preventDefault()` 可否决；Legacy 仍按原先注册顺序执行。

自定义拖动在 PointerDown/Move 中调用 `claimPointer(ownerId, axis)` 申请归属，在 GestureStart/Update/End 中修改状态，并处理 GestureCancel。`gestureOwner()` 查询当前归属。Coordinated 中取消 PointerDown 会抑制该序列默认行为，取消 PointerMove 会抑制本次已归属 Range 更新。

回调可以更新文字、数值、显隐或调用 `pointerCancel()`。隐藏或禁用目标、祖先或拖动归属节点也会取消接触；当前事件继续传播，后续默认行为与 Change 停止。不要递归注入指针事件。打开/提交编辑会话、focus 等受重入限制的操作应排队到 loop；节点事件里的导航会延后，新页更新放在 RouteChange。

## 组件接口速查

| 组件 | 常用接口与边界 |
| --- | --- |
| 文字、标签、按钮 | `setText/text`；按钮 Click/LongPress，`setEnabled`；label 的 for 仅关联同页 Checkbox/Switch |
| Checkbox / Switch | `setValue/value`，0/1；Checkbox 可改标题 |
| Range / Progress | `setValue/value`；Range 支持水平或 `data-direction="vertical"`，Progress 建议显式 max=100 |
| Gauge / Ring / Spinbox | `setValue/value`；Spinbox 支持加减、长按重复与数值键盘 |
| InputField | `setText/text`、`setValue/value`（仅数值型）与共享编辑会话，见下文 |
| LineChart / BarChart | `setChartSeries/setChartValue/appendChartValue/chartValue` |
| Media / Map | 图片见 [动态图片](DynamicImages.md)；地图由可选扩展输出动态 Media |
| Scroll / Pager | `scrollTo(id,x,y)` / `setPage/pageIndex`；分页不改变路由 |
| 容器、图形、图标 | 显隐、样式与动画；形状/图标在编译时确定，无通用顶点或矢量颜色 setter |

| 封装组件 | 公开根 ID 的接口 |
| --- | --- |
| `gfx-image-button` | `setText/text` 代理标题；Click/LongPress |
| `gfx-setting-row` | `setValue/value` 代理 Switch，`setText/text` 代理标签 |
| `gfx-list` | 生成选项的 Click/LongPress 带 optionIndex；自定义子元素用自己的 ID |
| `gfx-select` / `gfx-choice` | `setValue/value` 读写选项索引，`setText/text` 读写标题；Select 另有 `setExpanded/expanded` |
| `gfx-tabs` | `setPage/pageIndex` 或 `setValue/value` 代理 Pager |
| `gfx-popup` / `gfx-confirm` | `setExpanded/expanded`、`setText/text`；业务 Click 来自关闭 / 确认按钮 |
| `gfx-loading` | `setText/text`、`setVisible`；任务由应用执行 |
| `gfx-value-slider` | `setValue/value` 同步滑块与内置读数 |

Select/Choice 的选项 Click 用 `optionIndex` 表示意图，提交后的根 Input/Change 用 `value` 表示索引；业务枚举由应用映射。Popup/Confirm 的展开、焦点恢复与输入模态策略不能代替应用的业务事务管理。完整标签、属性与支持边界见 [HTML/CSS 支持](../../nice_gfx_editor/packages/html-ui/HTML_CSS_SUPPORT.md)。

## 声明式联动

展示联动可写在 `data-ui-*` 中，业务动作写在 `data-on-*` 中：

```html
<input id="level" type="range" min="0" max="100" value="50"
  data-ui-input='[{"action":"value","target":"meter","value":"$value"},{"action":"format","target":"reading","value":"{value} %"}]'
  data-on-change="applyLevel">
<progress id="meter" max="100" value="50"></progress>
<p id="reading">50 %</p>
```

动作包括 `text/append/backspace/format/value/increment/decrement/toggle/visible/enabled/page/animate/show/hide`。每个事件展开后最多 16 项，目标必须同页；`$value` 是事件整数值，format 必须恰好包含一个 `{value}`。

程序 setter 不执行 `data-ui-input/change`，应用应同步其他展示；ValueSlider 内置读数会同步。Pager setter 可能发 Scroll/PageChange，同步分页器前先比较索引。预留初始隐藏节点用 `data-visible="false"`；HTML hidden 或 CSS display:none 可能消除布局空间。

## 输入、定点数与图表

`gfx-input` 支持 text/number/password/pin/tel；标准 `input` 提供有限别名。number 的 `data-decimals` 为 0–6，Spinbox 为 0–3。当前键盘没有中文输入法，中文初值和动态文字需提前导出字形。

| API / 状态 | 含义 |
| --- | --- |
| `beginTextInput(id)` | 打开共享编辑会话 |
| Input、`inputDraft()` | 草稿变化；`text/value` 仍返回确认值 |
| `inputText/backspaceInput/deleteInput/moveInputCursor` | 按 UTF-8 字符边界编辑草稿与光标 |
| `commitTextInput()` | 校验提交；改变时发 Change，明确确认发 Submit；失败保留草稿 |
| `cancelTextInput()` / `hideTextInput()` | 放弃草稿 / 收起键盘并保留草稿；离页取消会话 |

数值 InputField 的 `text()` 是确认字符串，Spinbox 的 `text()` 是标题；数值用 `value()`。字符数与 UTF-8 字节容量分别受限。

Gauge/Ring/Spinbox、数值 InputField 和图表使用整数定点数：`data-decimals=1` 时，C++ 的 235 表示 23.5，HTML value/min/max 仍写可读小数。Range/Progress 按范围与步长归一化，输入字段严格校验，越界可能失败。

```cpp
constexpr auto chartId = ui_config::id("temperature-chart");
int32_t samples[] = {231, 235, 240};
ui.setChartSeries(chartId, 0, samples, 3);
ui.appendChartValue(chartId, 0, 242);
ui.setChartValue(chartId, 0, 1, 236);
```

图表最多 4 个序列，每序列容量 2–512，满后淘汰最旧样本；索引按时间顺序，`INT32_MIN` 表示缺失。样本由 View 复制，结构与样式来自资源，`setValue()` 不更新样本。

## 导航、样式与调度

`push/replace` 的路径必须存在于项目中，`back/home` 返回历史页 / 初始页；导航返回 true 不代表转场和传输完成。已访问页保留控件状态；`releasePage(path)` 仅释放不被历史、转场或在途快照引用的非活动页，下次访问恢复编译默认值。页内分页从 0 开始，不改变 `ui.path()`。

实体按键使用生成的 ButtonAdapter 或 `inputButton(keyId,gesture,now)`；逻辑动作使用 `inputAction(InputAction::Next/Confirm/Activate…)`。`focus(id)/focused()` 管理焦点，程序 focus 保留当前输入模式。

`setVisible()` 立即显隐；`show/hide` 可播放资源声明的动画。隐藏不重排兄弟节点。`style/setStyle` 修改支持的样式，`layoutBounds/setBounds` 操作布局矩形，`bounds()` 返回视觉位置；`layoutChildren()` 仅排列 Container/Scroll 的直接子项，不执行完整浏览器重排。动画由 `playAnimation/stopAnimation/animationRunning` 控制。

所有 View API 从同一 UI 任务调用。后台任务/ISR 通过队列交付数据，耗时业务分步执行。`pollUi()` 已处理输入与 tick；自定义输入也需持续 tick 并正确发送取消事件。`Status::Busy` 表示仍有工作，继续调度。

`beginUpdate/endUpdate` 暂缓新帧启动，setter 立即改变状态，不提供回滚。`finish()` 仅排空当前快照，不推进未来动画；无限动画下不能等待 busy 永久变为 false。编译 UI 需要 STL 和足够动态内存，AVR 仅支持基础绘图。刷新与内存预算见 [架构说明](Architecture.md#刷新与内存)。

地图使用独立 `nice_gfx_map` 扩展，当前自动导出支持 ESP32 Arduino；安装扩展后调用生成的 `ui_maps::begin/poll`。详情见 [地图组件](../../nice_gfx_editor/docs/map-component.md)。

完整接口与生命周期见 [View.h](../src/UI/View.h)、[UI 运行时](../src/UI/README.md)；封装与插槽见 [组件模型](../../nice_gfx_editor/docs/component-model.md)。
