# 字体图样与调用方法

内置 **22 套 1bpp 点阵字体和 4 套 4bpp 平滑字体**，均通过 `setFont()` 选择。下表尺寸为实际字符格；完整来源、覆盖范围和样例见 [图样清单](assets/fonts/previews/catalog.json)。

## 调用

包含 `<NICE_GFX.h>`，在屏幕初始化后选择字体。屏幕与 Sprite 各自保存字体设置；默认不选字体，此时不绘制文字，`textWidth()` / `fontHeight()` 返回 0。

```cpp
gfx.setFont(nice_gfx::fonts::GfxSmoothCN16);
gfx.setTextSize(1);
gfx.setTextColor(GFX_WHITE, GFX_BLACK);
gfx.drawString("温度 25℃", 8, 8);
```

`drawString()` 绘制单行，换行使用 `setCursor()` 配合 `print()` / `println()`；数值和单字使用 `drawNumber()`、`drawFloat()`、`drawChar()`。`setTextDatum()` 设置对齐，`setTextPadding()` 擦除旧文本尾部，`textWidth()` / `fontHeight()` 测量当前字体。

`setTextSize(1..7)` 是整数倍缩放。名称中的 12 / 16 / 24 是源字体栅格化字号，行高以字符格为准。`getFont()` 返回当前字体指针，`setFont(nullptr)` 清除选择。

## 抗锯齿默认开启

编辑器字体与图标面板默认开启“抗锯齿”，对应可继承样式 `antialiasing: auto`；设为 `none` 恢复二值边缘。原生库使用 `gfx.setAntialiasing(false)` 关闭，`getAntialiasing()` 查询；此设置同时作用于文字和 `drawIcon()`，不改变排版尺寸。

屏幕不能读回背景时，用 `setTextColor(前景色, 背景色)` 指定背景；图标使用带背景色的 `drawIcon()` 重载。透明抗锯齿需要可读背景或 Sprite；完整灰度效果使用 16bpp Sprite，1bpp / 4bpp Sprite 保留二值轮廓，8bpp 会量化颜色。

## 平滑字体：16 级灰度抗锯齿

中文界面可选 `GfxSmoothCN16/24`，英文读数可选 `GfxSmoothLatin16/24`。字模由思源黑体生成，编辑器与固件共用数据；优先使用原生 16px / 24px，整数放大不会增加细节。

| 对象名（`nice_gfx::fonts::`） | 字符格 | 覆盖范围 | 图样 |
| --- | --- | --- | --- |
| `GfxSmoothCN16` | 16×22 | GB2312 + ASCII，7,540 字形 | [查看](assets/fonts/previews/mono/GfxSmoothCN16.png) |
| `GfxSmoothCN24` | 24×32 | 同上 | [查看](assets/fonts/previews/mono/GfxSmoothCN24.png) |
| `GfxSmoothLatin16` | 10×22 | ASCII 95 字符 | [查看](assets/fonts/previews/mono/GfxSmoothLatin16.png) |
| `GfxSmoothLatin24` | 15×32 | ASCII 95 字符 | [查看](assets/fonts/previews/mono/GfxSmoothLatin24.png) |

编辑器中选择“GfxSmooth 平滑黑体”，16px / 24px 文字至少预留 22px / 32px 行高。完整中文字库较大，固定文案使用“中文字体与空间优化 → 仅项目文字”，并补充运行时文案；子集保留灰度数据及许可。

## 22 套 1bpp 点阵字体

英文字体均包含 U+0020～U+007E 共 95 个可打印 ASCII 字符。

![英文字体图样](assets/fonts/previews/mono-ascii.png)

| 对象名（`nice_gfx::fonts::`） | 字符格 | 图样 |
| --- | --- | --- |
| `TomThumb4x6` | 4×6 | [查看](assets/fonts/previews/mono/TomThumb4x6.png) |
| `Scientifica5x11` | 5×11 | [查看](assets/fonts/previews/mono/Scientifica5x11.png) |
| `ProFont6x12` | 6×12 | [查看](assets/fonts/previews/mono/ProFont6x12.png) |
| `Fixed6x13` | 6×13 | [查看](assets/fonts/previews/mono/Fixed6x13.png) |
| `Cozette6x13` | 6×13 | [查看](assets/fonts/previews/mono/Cozette6x13.png) |
| `Tamzen7x14` | 7×14 | [查看](assets/fonts/previews/mono/Tamzen7x14.png) |
| `Spleen8x16` | 8×16 | [查看](assets/fonts/previews/mono/Spleen8x16.png) |
| `GfxTerm8x16` | 8×16 | [查看](assets/fonts/previews/mono/GfxTerm8x16.png) |
| `GfxTermBold8x16` | 8×16 | [查看](assets/fonts/previews/mono/GfxTermBold8x16.png) |
| `GfxSansLatin16` | 10×22 | [查看](assets/fonts/previews/mono/GfxSansLatin16.png) |
| `GfxSansLatin24` | 15×32 | [查看](assets/fonts/previews/mono/GfxSansLatin24.png) |
| `Spleen12x24` | 12×24 | [查看](assets/fonts/previews/mono/Spleen12x24.png) |
| `TamzenBold8x16` | 8×16 | [查看](assets/fonts/previews/mono/TamzenBold8x16.png) |

![中文字体图样](assets/fonts/previews/mono-cjk.png)

| 对象名（`nice_gfx::fonts::`） | 字符格 | 覆盖范围 | 图样 |
| --- | --- | --- | --- |
| `UnifontCN16` | 16×16 | GB2312 + ASCII，7,540 字形 / 6,763 汉字 | [查看](assets/fonts/previews/mono/UnifontCN16.png) |
| `GfxSansCN12` | 12×16 | 同上 | [查看](assets/fonts/previews/mono/GfxSansCN12.png) |
| `GfxSansCN16` | 16×22 | 同上 | [查看](assets/fonts/previews/mono/GfxSansCN16.png) |
| `GfxSansCN24` | 24×32 | 同上 | [查看](assets/fonts/previews/mono/GfxSansCN24.png) |
| `GfxSerifCN24` | 26×32 | 同上 | [查看](assets/fonts/previews/mono/GfxSerifCN24.png) |
| `GfxKaiCN16` | 18×20 | 9,000 汉字 + GB2312 非汉字 + ASCII，共 9,777 字形 | [查看](assets/fonts/previews/mono/GfxKaiCN16.png) |
| `GfxKaiCN24` | 26×30 | 同上 | [查看](assets/fonts/previews/mono/GfxKaiCN24.png) |
| `GfxSansBoldCN24` | 24×32 | 同上 | [查看](assets/fonts/previews/mono/GfxSansBoldCN24.png) |
| `GfxSerifHeavyCN24` | 28×33 | 同上 | [查看](assets/fonts/previews/mono/GfxSerifHeavyCN24.png) |

输入字符串使用 **UTF-8**。中文字体中的 ASCII 也占全宽字符格；GB2312 和 9,000 汉字仅表示覆盖范围，不包含全部 Unicode。缺字使用 `?` 替代。

## 自定义字体与中文子集

完整中文字体仅在非 AVR 平台提供；AVR 使用短文本子集，并检查 near-PROGMEM 地址范围。编辑器可按项目文案导出 `.h` / `.cpp` 和许可文件，应用包含生成头后调用 `gfx.setFont(nice_gfx::fonts::ProjectCN)`。新增文案后重新生成，并补齐运行时文本。

内置字模位于 `src/Fonts/Mono/GFX_Font_*.cpp`，只包含声明不会引用全部字库；启用链接器段裁剪后，未引用的字库可移除。ESP-IDF 可按字体选择源文件，见 [构建配置](Architecture.md#esp-idf-构建)。

自定义 [MonoFont](../src/Fonts/Monospace.h)、字库占用和来源见 [字体说明](../src/Fonts/README.md)、[新增字体](../src/Fonts/EditorFonts.md) 与 [扩展设备字体](../src/Fonts/ExtendedDeviceFonts.md)。分发原字模或子集时保留 [字体许可及版权声明](../src/Fonts/licenses)。
