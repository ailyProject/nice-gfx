# Arduino 组件交互与联动

编辑器把 HTML/CSS 编译为 `ui.h`，由 `nice_gfx::htmlui::View` 处理控件状态、事件与绘制。业务数据、存储、通信和外设操作由应用维护。完整运行库与 `ui.h` 应一起更新，资源 ABI 以 [Compiled.h](../src/UI/Compiled.h) 为准。

## 接入导出工程

| 文件 | 用途 |
| --- | --- |
| `ui.h` | 页面资源、稳定 ID、路由、界面动作与业务绑定入口 |
| `Hardware.h` | `displayInstance()` / `gfx`、显示配置、UI 尺寸与 tile 预算 |
| `TouchAdapter.h` | `beginDisplayHardware()`、触摸采样与 `pollUi()` |
| `ButtonAdapter.h` | 配置实体按键时生成，负责 GPIO 采样与消抖 |
| 主 `.ino` | 应用数据、回调与循环，由开发者维护 |
| `src/nice_gfx/` | 完整导出附带的运行库 |
| `data/` | 可继续编辑的工程与页面源码，设备不在运行时解析 HTML/CSS |

显示接线与库预设匹配时，`Hardware.h` 直接复用 [开发板预设](BoardPresets.md)；自定义接线使用显式配置。首次导出建议使用完整 Arduino 工程。后续仅改界面且硬件、接口与 ABI 兼容时，可替换 `ui.h`，保留业务源码。地图等扩展还需同步其生成文件。

主程序依次包含 `Hardware.h`、`ui.h`、`TouchAdapter.h`。完整导出使用随附运行库；使用已安装的 Arduino 库时，先包含 `<NICE_GFX.h>`，让构建工具发现库。

## 最小联动示例

在设计器的首页 `/` 放置以下内容，按实际屏幕尺寸导出完整工程：

```html
<main style="width:100%;height:100%;padding:8px;gap:8px">
  <input id="level" type="range" min="0" max="100" value="50"
    data-on-input="levelChanged">
  <progress id="meter" max="100" value="50"></progress>
  <p id="reading">50 %</p>
</main>
```

将主 `.ino` 改为：

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
  Serial.begin(115200);
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

应用先更新 `AppData`，再刷新展示。普通 `setValue/setText` 保持静默，不会递归触发 Input/Change。多页面项目在 RouteChange 中从应用数据刷新当前页；业务任务应排队或分步执行，避免回调阻塞输入与绘制。完整示例见 [GfxTouchDemo](../examples/07_UI/GfxTouchDemo/GfxTouchDemo.ino)。

## ID 与事件绑定

`ui_config::id("level")` 使用元素 ID，不加 `#`；它不检查元素是否存在。ID 应在工程内唯一，编译器检查重复和哈希冲突。getter/setter 只查当前路由页，缺失时 setter 返回 false，`text()` 返回空串，`value()` 返回 0。

`setText()` 复制输入字符串，`text()` 返回借用指针，更新文字、重新挂载或销毁 View 后不要继续持有。回调 `context` 必须长期有效，Event 只在本次回调内有效。

```cpp
void clicked(void *, nice_gfx::htmlui::Event &event) {
  // 按 event.target 或 event.component 区分来源。
}
void routeChanged(void *, nice_gfx::htmlui::Event &) {
  // 按 ui.path() 从应用数据刷新当前页。
}
// setup 中，load 成功后绑定一次：
// ui.on(ui_config::id("save"), nice_gfx::htmlui::EventType::Click, clicked);
// ui.on(0, nice_gfx::htmlui::EventType::RouteChange, routeChanged);
```

`data-on-click/input/change/submit/cancel/longpress/pagechange` 生成 `ui_config::bind_动作名()`。同名动作可用于多个节点，一次绑定即可；实际函数名以生成的 `ui.h` 为准。`bind_*` 和 `on()` 追加监听，不放入 loop，也不要在再次 load 后重复绑定已有业务监听。

| Event 字段 | 含义 |
| --- | --- |
| `type`、`phase` | 事件类型与 Capture / Target / Bubble 阶段 |
| `target`、`currentTarget` | 原始目标与当前监听节点 |
| `component` | 封装组件公开根 ID；普通节点为 0 |
| `value`、`optionIndex` | 当前整数值与选项意图索引；无选项时索引为 -1 |
| `x/y`、`scrollX/Y` | 指针位置与目标滚动偏移 |
| `from/to/routeAction` | 路由来源、目标与动作 |

普通事件依次经过祖先捕获、目标、祖先冒泡。`stopPropagation()` 停止后续节点，`stopImmediatePropagation()` 还停止当前节点其余监听。`preventDefault()` 只取消可取消事件的默认行为，不撤销先前回调的任意修改。

| 事件 | 使用要点 |
| --- | --- |
| Click | 有效激活；可取消控件默认行为或路由 |
| Input / Change | Range 拖动中报告 Input，释放值与起始值不同才报告 Change；取消拖动不提交 |
| Input / Change（开关） | Click 先看到临时切换值；拒绝 Click 后回滚且不发这两个通知 |
| LongPress | tick 推进长按识别；Spinbox 长按加减另有重复机制 |
| PointerCancel | 输入错误、目标隐藏/禁用或拖动接管时取消接触 |
| Scroll / PageChange | 滚动偏移 / 页内分页提交；Scroll 不冒泡 |
| Submit / Cancel | 输入确认 / 取消；Submit 允许确认值未改变 |
| BeforeNavigate | 可取消导航；用 ID 0 监听 |
| RouteChange / NavigationCancel | 路由提交 / 取消通知；用 ID 0 监听 |

声明属性不代表控件一定产生该事件，例如 Progress 不产生用户数值编辑。`data-on-submit/cancel` 用于 `gfx-input`；Spinbox 的对应通知通过 C++ `on()` 监听。

回调可更新文字、数值、显隐，或用 `pointerCancel()` 结束接触。打开/提交编辑会话、focus、取消导航等受重入限制，应记录请求并在 loop 执行。不要递归注入指针事件。节点事件中的路由请求会延后处理，新页更新放在 RouteChange。

## 组件接口速查

| 组件 / 标签 | 更新与交互 |
| --- | --- |
| 图形 `gfx-ellipse/rect/line/polyline/polygon` | 形状来自编译资源，可显隐、移动和动画；无通用顶点或矢量颜色 setter |
| 图片 `gfx-media`、`img` | `setImage/setMediaResource/clearImage/resetImage`，见 [动态图片](DynamicImages.md) |
| 地图 `gfx-map` | 可选 `nice_gfx_map` 扩展输出动态 Media；见下文 |
| 容器、卡片 `div/main/section/gfx-card` 等 | 子树显隐、禁用与事件传播 |
| 文字 `p/span/h1…h6/pre/code`、`label` | `setText/text`；label 的 for 只关联同页 Checkbox/Switch |
| 图标 `gfx-icon` | 图案在编译时确定，可改样式颜色、显隐与动画 |
| 按钮 `button`、路由链接 `a` | Click/LongPress，`setText/setEnabled`；a 仅支持项目内路由 |
| 输入框 `gfx-input`、受支持的 `input` 别名 | 自动键盘，Input/Change/Submit/Cancel；数值使用定点整数 |
| 复选框 / 开关 | `setValue/value`，0/1；复选框可 `setText` 改标题 |
| 滑块 `input[type=range]`、`gfx-slider` | Input/Change，`setValue/value`；支持水平及 `data-direction="vertical"` |
| 进度 `progress` | `setValue`，建议显式 max=100 |
| 仪表 / 环形进度 `gfx-gauge/ring` | `setValue/value`，单位、色段与数值动画由资源配置 |
| 图表 `gfx-line-chart/bar-chart` | `setChartSeries/setChartValue/appendChartValue/chartValue` |
| 步进 `gfx-spinbox` | 加减、长按重复与数值键盘；`setValue/value` |
| 滚动 `gfx-scroll` | 单轴滚动，`scrollTo(id,x,y)` |
| 分页 `gfx-pager` | `setPage/pageIndex`，不改变路由 |

显示型节点没有用户数值编辑行为；需要点击时明确声明事件或使用交互容器。某些原生节点允许通用 setter，但未必有对应可绘属性。标准 HTML 别名仅支持有限语义，详见 [HTML/CSS 支持](../../nice_gfx_editor/packages/html-ui/HTML_CSS_SUPPORT.md)。

封装组件由 `ui_config::load()` 安装链接，需要 `NICE_GFX_UI_COMPONENTS_VERSION >= 1`。使用作者根 ID，避免硬编码生成的内部 ID；手工 mount 的应用需另行配置组件链接。

| 封装组件 | 公开根 ID 的接口 / 事件 |
| --- | --- |
| `gfx-image-button` 图文按钮 | `setText/text` 代理标题；Click/LongPress |
| `gfx-setting-row` 设置行 | `setValue/value` 代理 Switch，`setText/text` 代理标签；Input/Change |
| `gfx-list` 列表 | 生成选项 Click/LongPress 有 optionIndex；自定义子元素使用自己的 ID |
| `gfx-select` 下拉 | `setValue/value` 读写索引，`setText/text` 标题，`setExpanded/expanded` 面板 |
| `gfx-choice` 互斥组 | `setValue/value` 读写索引，`setText/text` 标题；非法索引失败 |
| `gfx-tabs` 标签分页 | `setPage/pageIndex` 或 `setValue/value` 代理 Pager，报告 PageChange |
| `gfx-popup` 弹出面板 | `setExpanded/expanded`、`setText/text` 消息；业务 Click 来自关闭按钮 |
| `gfx-loading` 加载提示 | `setText/text`、`setVisible`；任务由应用执行 |
| `gfx-value-slider` 滑块读数 | `setValue/value` 同步滑块与内置读数；Input/Change 来自 Range |
| `gfx-confirm` 确认面板 | `setExpanded/expanded`、`setText/text` 消息；业务 Click 来自确认按钮 |

Select/Choice 选项 Click 使用 `optionIndex` 表示意图，提交后的根 Input/Change 使用 `value` 表示索引。应用将索引映射为业务枚举。Popup/Confirm 提供面板展开与焦点恢复，完整模态锁定和业务事务由应用实现。

## 声明式联动

简单展示联动可放在 `data-ui-*` 中，业务操作放在 `data-on-*` 中：

```html
<input id="level" type="range" min="0" max="100" value="50"
  data-ui-input='[{"action":"value","target":"meter","value":"$value"},{"action":"format","target":"reading","value":"{value} %"}]'
  data-on-change="applyLevel">
<progress id="meter" max="100" value="50"></progress>
<p id="reading">50 %</p>
```

可用动作包括 `text/append/backspace/format/value/increment/decrement/toggle/visible/enabled/page/animate`。每个事件展开后最多 16 项，目标必须同页；`$value` 是事件整数值，format 只替换一个 `{value}`。

程序 setter 不执行 `data-ui-input/change`，应自行同步关联展示；`gfx-value-slider` 的内置读数会同步。Pager 是通知例外，`setPage()` / `setValue()` 可产生 Scroll/PageChange，同步多个分页器时先比较当前索引。

UI 动作通常先于后绑定的业务监听执行，之后拒绝 Click 不会回滚已执行动作。预留初始隐藏节点用 `data-visible="false"`；HTML hidden 或 CSS display:none 可能在编译时消除布局空间。

## 输入、定点数与图表

`gfx-input` 支持 text / number / password / pin / tel；标准 `input` 还提供 text / password / tel / number 的有限别名。number 默认整数，通过 `data-decimals` 使用 0–6 位小数；需要加减和 step 用 `gfx-spinbox`，精度为 0–3 位。当前键盘没有中文输入法，中文初值和动态文字需提前导出字形。

| 输入状态 / API | 含义 |
| --- | --- |
| Input、`inputDraft()` | 草稿变化；`text(id)` 仍是确认值 |
| Change、`text/value` | 校验通过并改变确认值 |
| Submit | 校验通过的明确确认，允许值不变 |
| `beginTextInput/commitTextInput/cancelTextInput` | 打开、校验提交、放弃草稿；提交失败保留草稿 |
| `inputText/backspaceInput/deleteInput/moveInputCursor` | 编辑草稿与光标，按 UTF-8 字符边界处理 |
| `hideTextInput()` | 收起键盘保留草稿；导航离页则取消会话 |

数值输入框的 `text()` 是确认字符串；Spinbox 的 `text()` 是标题，数值用 `value()`。字符数与 UTF-8 字节容量是不同限制。

Gauge/Ring/Spinbox、数值 InputField 和图表使用整数定点数：`data-decimals=1` 时，C++ 的 235 表示 23.5；HTML value/min/max 仍写可读小数。Range/Progress 按范围与步长归一化，输入字段按规则严格校验，越界可以失败。

```cpp
constexpr auto chartId = ui_config::id("temperature-chart");
int32_t samples[] = {231, 235, 240};
ui.setChartSeries(chartId, 0, samples, 3);
ui.appendChartValue(chartId, 0, 242);
ui.setChartValue(chartId, 0, 1, 236);
```

图表最多 4 个序列，每序列容量 2–512，满后淘汰最旧样本；索引按时间顺序，`INT32_MIN` 表示缺失。数据由 View 复制，序列结构与样式来自资源；通用 `setValue()` 不更新图表样本。

## 分页、路由与实体按键

| 操作 | API 与边界 |
| --- | --- |
| 滚动区域 | `scrollTo(id,x,y)`，偏移限制在有效范围；内容需超出视口 |
| 页内分页 | `setPage/pageIndex`，从 0 开始，不改 `ui.path()`；拖动和吸附需持续 tick |
| 路由 | `push/replace/back/home`；路径必须存在，返回 true 不表示转场和传输已结束 |
| 导航守卫 | `setRouteGuard()` 或 BeforeNavigate；同步快速返回 |
| 页面恢复 | RouteChange 中刷新应用数据；`releasePage(path)` 只释放可安全释放的非活动页 |
| 实体按键 | 生成的 ButtonAdapter，或 `inputButton(keyId,gesture,now)` |
| 逻辑动作 | `inputAction(InputAction::Next/Confirm/Activate…)` |
| 焦点 | `focus(id)/focused()`；程序 focus 保留当前输入模式 |

已访问页保留控件状态，直到重新 mount、销毁或显式释放。历史、转场和在途快照引用的页面不能释放。业务历史和持久设置仍放在应用层。

## 显隐、样式与调度

`setVisible()` 立即显隐；`show()/hide()` 可播放资源声明的进入/离场动画。`setEnabled()` 控制交互，隐藏或禁用目标及祖先可取消接触；隐藏不重新分配兄弟布局。

`style()/setStyle()` 改支持的样式，`layoutBounds()/setBounds()` 改布局矩形，`bounds()` 返回视觉位置。`layoutChildren()` 可显式排列 Container/Scroll 的直接子项，运行时不执行完整浏览器重排。Pager 步距固定，其子页几何有限制。动画由资源声明，通过 `playAnimation/stopAnimation/animationRunning` 控制。

所有 View API 从同一个 UI 任务调用。后台任务/ISR 通过队列交付数据，再由 UI 任务更新。生成的 `pollUi()` 已包含触摸采样和 tick；自定义输入需正确发送取消事件，并持续 tick。`Status::Busy` 表示绘制或传输尚未完成，继续调度即可。

`beginUpdate/endUpdate` 暂缓新帧启动，setter 仍立即改变状态；它不是回滚事务。`finish()` 仅排空当前快照，不推进未来动画；无限动画存在时不要等待 busy 永久变为 false。内存和刷新预算见 [架构说明](Architecture.md#刷新与内存)。编译 UI 需要 STL 和足够动态内存，AVR 仅支持基础显示绘图。

## 地图与进一步参考

`gfx-map` 由独立 `nice_gfx_map` 扩展绘制，通过动态 Media 接入 View。当前自动导出支持 ESP32 Arduino；工程生成 `MapRuntime.h` 等文件，应用另行安装扩展库，并调用 `ui_maps::begin/poll`。地图不提供 POI 检索或路线规划，详情见 [地图组件](../../nice_gfx_editor/docs/map-component.md)。

- [View 公共 API](../src/UI/View.h)：完整接口与返回值。
- [UI 运行时说明](../src/UI/README.md)：事件、导航、渲染与资源生命周期。
- [动态图片](DynamicImages.md)：像素格式、复制与所有权交接。
- [HTML/CSS 支持](../../nice_gfx_editor/packages/html-ui/HTML_CSS_SUPPORT.md)：标签、属性、动作语法与限制。
- [组件模型](../../nice_gfx_editor/docs/component-model.md)：封装组件与内容插槽。
