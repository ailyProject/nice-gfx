# More device font styles

Six additional device fonts use the same committed bitmap data in C++ firmware
and the editor. They can be bundled with commercial products without payment.
Keep the complete copyright, license and derived-font notices when distributing
font data. The four Chinese faces remain under SIL OFL 1.1; OFL permits embedding
and bundling with commercial software, prohibits selling the font alone, and
does not change the license of the application using it.

| Object | Visual style | Raster em | Fixed cell | Baseline | Han / total glyphs | Data bytes |
| --- | --- | ---: | --- | ---: | --- | ---: |
| `GfxKaiCN16` | Informal handwritten Kai | 16 | 18×20 | 16 | 9,000 / 9,777 | 606,174 |
| `GfxKaiCN24` | Informal handwritten Kai | 24 | 26×30 | 23 | 9,000 / 9,777 | 1,192,794 |
| `GfxSansBoldCN24` | Bold sans, strong UI labels | 24 | 24×32 | 24 | 9,000 / 9,777 | 958,146 |
| `GfxSerifHeavyCN24` | Heavy serif, prominent titles | 24 | 28×33 | 24 | 9,000 / 9,777 | 1,310,118 |
| `Spleen12x24` | Large pixel text | 24 | 12×24 | 19 | 0 / 95 | 4,560 |
| `TamzenBold8x16` | Compact bold pixel text | 16 | 8×16 | 12 | 0 / 95 | 1,520 |

The em size is not the cell height. Cells preserve original stroke proportions,
baseline, ascenders, descenders and serif overhangs; the generator rejects any
cropped stroke. Latin glyphs in Chinese faces are centered within the same fixed
advance. All faces are monochrome 1-bpp bitmap fonts, not runtime vector fonts.
The extended Chinese objects are available on non-AVR targets; use a small
application-specific subset on AVR.

```cpp
#include "../Graphics/Gfx.h"
// After configuring and starting your display:
gfx.setFont(nice_gfx::fonts::GfxKaiCN24);
gfx.drawString("温度25℃ 设备正常", 8, 8);
gfx.setFont(nice_gfx::fonts::GfxSansBoldCN24);
gfx.drawString("运行状态", 8, 48);
```

HTML/CSS can select `font-family:GfxKaiCN24;font-size:24px`, or any object name
from the table. Export supports `full`, `used`, and `preset` with `presetCount`
of 3,000 / 5,000 / 9,000, plus optional `extraText`, using actual coverage. Required UI/translation/extra
characters are retained before preset fill; GB2312 level-one, level-two and
remaining available Unicode Han glyphs form the fill order. The 9,000 preset is
the count of Han glyphs, not total characters. Existing GB2312 faces contain only
6,763 Han glyphs and cannot supply 9,000; insufficient coverage is reported.

`full` keeps all 9,777 glyphs for each new Chinese face. Subset exports reduce
firmware Flash usage; fonts absent from application references are removed by
the linker. Declaration in `NICE_GFX.h` does not register or retain every font.

Sources and licenses:

- `GfxKaiCN16/24` derive from [LXGW WenKai 1.522](https://github.com/lxgw/LxgwWenKai/tree/a22e2a064f471fffd194af2a69072944bbe218dc),
  copyright LXGW and the Klee Project Authors. Keep [the complete OFL](licenses/LXGWWenKai-OFL-1.1.txt)
  and each `GfxKai*-NOTICE.txt`. Derived primary names omit all reserved names.
- `GfxSansBoldCN24` derives from [Adobe Source Han Sans CN Bold 2.005R](https://github.com/adobe-fonts/source-han-sans/tree/6c709ca72d3d7c46ab42ebecc1a26e7d69595a37).
  Keep [its OFL](licenses/SourceHanSans-OFL-1.1.txt) and `GfxSansBoldCN24-NOTICE.txt`.
- `GfxSerifHeavyCN24` derives from [Adobe Source Han Serif CN Heavy 2.003R](https://github.com/adobe-fonts/source-han-serif/tree/7889f11bf31170b5d092a083b357c8c8130f89e0).
  Keep [its OFL](licenses/SourceHanSerif-OFL-1.1.txt) and `GfxSerifHeavyCN24-NOTICE.txt`.
- `Spleen12x24` uses original ASCII pixels from [Spleen](https://github.com/fcambus/spleen/tree/57f9219328c9f5873085320fe8bc8f7dd34b8791),
  copyright Frederic Cambus; retain [BSD 2-Clause](licenses/Spleen-BSD-2-Clause.txt).
- `TamzenBold8x16` uses original ASCII pixels from [Tamzen](https://github.com/sunaku/tamzen-font/tree/3255e8259bc9b880c60ab8b737ec8aa574e00d75),
  copyright Scott Fial and Suraj N. Kurapati; retain [both permissive notices](licenses/Tamzen.txt).

ZCOOL XiaoWei and KuaiLe were evaluated but contain only 6,763 and 6,766 BMP
Han glyphs respectively. They are not bundled as substitutes for a 9,000-glyph
font; the three chosen outline styles each have over 27,000 source Han glyphs.

The original new TTF/OTF files are retained in [tools/font-sources/cjk](../../../tools/font-sources/cjk/README.md).
Official revisions and SHA-256 hashes are pinned in [cjk-sources.json](../../../sources/Fonts/Mono/cjk-sources.json);
pixel BDF source hashes are pinned in [font source manifest](../../../nice_gfx_editor/scripts/font-sources/sources.json).
Offline rasterization uses Pillow 12.3.0 / FreeType 2.14.3 / fontTools 4.64.0.
Run the following commands from the workspace root.

```sh
python -m pip install -r tools/font-requirements.txt
python tools/generate_cjk_fonts.py tools/font-sources/cjk --font GfxKaiCN16 --font GfxKaiCN24 --font GfxSansBoldCN24 --font GfxSerifHeavyCN24
node tools/generate_editor_fonts.mjs
python tools/preview_fonts.py
```

To generate a small custom Kai subset instead, use `--font GfxKaiCN24 --text
ui-text.txt --name ProjectKai --output /path/to/sketch`. This writes a separate
`.h`/`.cpp` plus licenses and a notice. Normal firmware builds and editor use
require no Python, fonts installed in the OS, or network access.
