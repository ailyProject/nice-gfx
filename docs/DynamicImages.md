# Arduino 图片组件的运行时更新

通过 `nice_gfx::htmlui::View` 更新图片，使像素参与正常的裁剪、动画、页面与缓存管理。直接调用 `gfx.pushImage()` 不会更新组件状态。

此功能要求 `NICE_GFX_UI_DYNAMIC_IMAGE_VERSION >= 1`，Media 资源格式以 [UI/Compiled.h](../src/UI/Compiled.h) 中的 `ResourceVersion` 为准。更新工程时同步运行库与设计器导出的 `ui.h`。

## 1. 在设计器中准备图片区域

插入“图片 / 视频”，给组件设置稳定 ID，例如 `photo`，勾选“设备端更新”。可以选择预置资源作为初始图，也可以保留空图片区域：

```html
<gfx-media id="photo" data-media-dynamic="true"
  style="width:240px;height:180px"></gfx-media>
```

空区域仍保留 Media 节点、几何与事件。换图不改变组件尺寸，使用最近邻缩放填满扣除边框和 padding 的内容区。

图片、逐帧播放与动画均支持 `border-radius` 裁剪。例如 80×80px 组件设置 `border-radius:40px` 可显示圆形图片。半径上限为短边一半，内容区再扣除边框与 padding；支持单一非百分比长度、最大 255px，0 为矩形，不支持 `50%`。

“设备端更新”要求显式 ID；带初始资源的普通图片也能调用更新接口。下载与解码由应用完成。

## 2. 普通更新：默认复制像素

对于解码器、传感器、相机或业务程序产生的 RGB565 数组：

```cpp
#include "Hardware.h"
#include "ui.h"

using nice_gfx::Status;
using nice_gfx::htmlui::ImageView;
constexpr auto photoId = ui_config::id("photo");

// 使用现有 View ui；pixels 是逐行排列的普通 uint16_t RGB565 色值。
Status updatePhoto(const uint16_t *pixels, size_t width, size_t height,
                   size_t availableBytes) {
  return ui.setImage(photoId, ImageView(pixels, width, height, availableBytes));
}
```

`Status::Ok` 表示像素已复制，可立即释放或复用输入；屏幕刷新由后续 `pollUi()` / `tick()` 完成。默认 `PixelOrder::Native` 表示 `uint16_t` 颜色值，例如红色为 `0xF800`，与 `gfx.setSwapBytes()` 无关。复制结果为紧凑的高字节优先像素。

对于已经保存为高字节优先的 RGB565 字节流：

```cpp
ui.setImage(photoId, ImageView(bytes, width, height, byteCount,
  nice_gfx::htmlui::ImageFormat::RGB565, nice_gfx::PixelOrder::Wire));
```

RGB332 使用每像素一个字节。解码器输出有行填充时，最后一个参数是**字节**步距：

```cpp
ui.setImage(photoId, ImageView(bytes, width, height, byteCount,
  nice_gfx::htmlui::ImageFormat::RGB332, nice_gfx::PixelOrder::Native, rowStrideBytes));
```

支持 RGB565 / RGB332。PNG、JPEG 等先在应用侧解码；像素源无独立 alpha 通道，透明图需先合成背景。

## 3. 大图：交接不可变缓冲区所有权

已经完整解码到 PSRAM 或堆内存的大图，可以用 `Image::adopt()` 避免另一份全图复制：

```cpp
#include <cstdlib>
using nice_gfx::htmlui::Image;
using nice_gfx::htmlui::ImageView;

void freePixels(void *, const void *pixels) {
  std::free(const_cast<void *>(pixels));
}

Status installDecodedPhoto(uint16_t *pixels, size_t width, size_t height,
                          size_t availableBytes) {
  Image image;
  const Status result = Image::adopt(
    ImageView(pixels, width, height, availableBytes), freePixels, nullptr, image);
  if (result != Status::Ok) {
    // 本函数约定接管 malloc 缓冲区；adopt 失败时仍由本函数释放。
    std::free(pixels);
    return result;
  }
  return ui.setImage(photoId, image);
}
```

`pixels` 必须由与 `std::free` 匹配的分配器创建；其他分配器提供对应 release 回调。adopt 仅成功时接管，之后不可改写或自行释放。局部 Image 可在 setImage 后销毁，View 与在途帧持有各自引用；setImage 失败也会正确释放已接管的局部图像。

最后一个句柄、页面、帧或缓存引用释放时，release 调用一次，可能在释放引用的任务执行。回调须快速、非阻塞、不抛异常、不重入 View，context 保持有效。

同一 Image 可供多个组件共享；`Image::copy()` 显式创建复制图，`view()` 只返回借用描述。

## 4. 同一个组件切换预置资源

工程媒体资源可按资源 ID 查找，包括未用作初始图的资源：

```cpp
// asset-online / asset-offline 是工程媒体资源 ID，不是元素 ID或文件路径。
Status result = ui_config::setMedia(ui, photoId,
                                   online ? "asset-online" : "asset-offline");
```

底层接口是 `ui.setMediaResource(photoId, *resource)`，例如：

```cpp
const auto *resource = ui_config::media("asset-online");
if (resource) ui.setMediaResource(photoId, *resource);
```

该路径复制描述符、借用不可变像素，不复制整图。像素须在页面、快照与缓存引用释放前有效；生成 ui.h 的静态资源满足要求，可变缓冲区应使用 setImage 或 adopt。

预置多帧资源也可切换。换源会从第 0 帧重新开始，tick 按资源帧率推进；用单帧动态图覆盖动画后，原动画不再驱动该组件。恢复初始资源时重新开始初始动画。

## 5. 清空与恢复默认

```cpp
ui.clearImage(photoId); // 清空像素内容，组件布局、外观和交互仍保留。
ui.resetImage(photoId); // 恢复 ui.h 中该组件的初始资源；初始为空则恢复空内容。
```

`setVisible(photoId, false)` 隐藏整个组件但保留图像。`hasImage(photoId, image)` 检查当前 Media 是否持有同一不可变 Image 源，不比较像素内容。

## 6. 失败、调度与内存约定

| 返回值 | 含义 |
| --- | --- |
| Ok | 已安装新源或完成清空/恢复 |
| NotInitialized | View 尚未 mount |
| InvalidArgument | 元素缺失/不在当前页/不是 Media，或图片参数无效 |
| NoMemory | 无法分配像素或所有权描述块 |

失败保留之前的组件图片和 Image 输出句柄。参数会在读取和分配之前检查：宽高 1–1024，缓冲区至少包含最后一个可见像素，行步距足够，算术不溢出，格式与字节序合法，输入总字节数不超过 8 MiB。

所有 View 更新从一个 UI 任务执行。后台下载/解码可准备独立 Image，通过队列把它交给 UI 任务；入队交接必须保证句柄有效、像素不可变及适当同步。路由切换后 setter 只查当前页：跨页结果留在应用中，进入目标页再安装。

在途帧继续使用旧源，后续帧使用新源；缓存按图片身份失效。旧图由引用自动保留和释放，无需用 delay 等待。

内存要允许旧、新图片短暂共存。例如 240×180 RGB565 是约 84.4 KiB 像素；复制上传期间输入与复制图、在途旧图可能同时存在。大图优先 adopt，仍须为在途帧保留的旧图预留空间。`memoryStats()` 去重计入拥有的像素及所有权描述块，预置 Flash 像素不计入动态像素占用。

换图不建立整屏帧缓冲，也不执行 HTML/CSS 重排。刷新吞吐取决于尺寸、缓冲预算与实际总线。

## 7. 示例与实现

- [完整 Arduino 动态图片示例](../examples/07_UI/DynamicImageDemo/DynamicImageDemo.ino)
- [图片输入与所有权 API](../src/UI/Image.h)
- [View 公共接口](../src/UI/View.h)
- [组件交互说明](ArduinoComponentInteraction.md)

## Transparent compiled RGB565 media

Compiled media may set `MediaDesc::bytesPerPixel` to `MediaRgb565ColorKey` (0x82). The source starts with a shared two-byte, MSB-first RGB565 transparent color, followed by ordinary wire-order RGB565 frames. Matching texels preserve the already-painted underlay, including scaled, rounded and affine rendering. Descriptor layout and resource ABI 17 remain unchanged. Check `NICE_GFX_UI_MEDIA_COLOR_KEY_VERSION >= 1` when generating these resources. Ordinary RGB565/RGB332 and dynamic `Image` sources retain their existing representation.

The editor project field is optional `media[].transparentColor` (integer 0–65535, RGB565 only). Its value is separate from the base64 pixel payload; export adds the shared prefix and capability guard. This is binary transparency, not fractional alpha. Node opacity still blends visible texels normally. Avoid using the selected key for visible foreground colors.
