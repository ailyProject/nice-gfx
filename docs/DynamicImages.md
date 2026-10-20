# Arduino 图片组件的运行时更新

图片更新通过 `nice_gfx::htmlui::View` 完成，新图与文字、数值一样成为组件状态，参与分块绘制、裁剪、缩放、动画、页面切换和缓存管理。不要在持续运行的 View 上用 `gfx.pushImage()` 绕过组件状态。

此功能要求 `NICE_GFX_UI_DYNAMIC_IMAGE_VERSION >= 1`，当前使用 ABI 15 的 Media 资源。更新工程时同步运行库与设计器导出的 `ui.h`。

## 1. 在设计器中准备图片区域

插入“图片 / 视频”，给组件设置稳定 ID，例如 `photo`，勾选“设备端更新”。可以选择预置资源作为初始图，也可以保留空图片区域：

```html
<gfx-media id="photo" data-media-dynamic="true"
  style="width:240px;height:180px"></gfx-media>
```

空区域仍导出为原生 Media，保留几何与事件。图片更新改变像素源，不自动改变组件位置或尺寸；当前采用最近邻缩放，拉伸到组件扣除边框和 padding 后的内容区。

图片内容支持 `border-radius` 裁剪，包括 `setImage()` / `setMediaResource()` 换图、逐帧播放及组件动画。半径先限制为组件短边的一半，再减去各边的边框与 padding，形成内容区内圆角。正方形组件设置半径为边长一半即可显示圆形图片，例如：

```html
<gfx-media id="avatar" data-media-dynamic="true"
  style="width:80px;height:80px;border-radius:40px"></gfx-media>
```

半径设为 0 恢复矩形；支持单一非百分比长度，最大 255px，不支持 `50%`。圆角效果由渲染器裁剪，不依赖源图片透明度。完整导出工程会携带更新后的运行库；手动维护固件时需同步运行库源码。

“设备端更新”是作者意图提示，并要求显式 ID；带初始资源的普通图片同样可以调用新接口。没有额外的后台数据绑定或图片 URL 表达式。

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

`Status::Ok` 返回时像素已复制，调用方可以立即复用或释放原缓冲区。新图不会在此调用中阻塞等待屏幕显示完毕；正常运行 `pollUi()` / `tick()` 即可完成刷新。

输入 RGB565 的默认字节序是 `PixelOrder::Native`，即数组元素是 `0xF800` 这样的颜色数值，与 `gfx.setSwapBytes()` 无关。复制时将其转换为紧凑的高字节优先像素，绘制不用反复转换。

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

格式范围为 RGB565 / RGB332。PNG、JPEG 等压缩文件需要在应用侧先解码；网络下载、文件系统与解码器不耦合到 View。当前像素源没有独立 alpha 通道，透明图需先合成到目标背景。

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
    // 本函数约定接管调用方交来的 malloc 缓冲区，因此失败也自行释放。
    // adopt 本身失败时不会接管或释放它。
    std::free(pixels);
    return result;
  }
  return ui.setImage(photoId, image);
}
```

这里 `pixels` 必须来自与 `std::free` 匹配的分配方式；其他分配器提供对应 release 回调。成功 adopt 后不能继续写入或自行释放像素。更新组件后，Image 局部变量可以销毁，View 和已经提交的帧会持有各自引用。

若 setImage 失败，例如组件不在当前路由页，局部 Image 销毁时仍正确释放已经接管的像素。release 在最后一个句柄、页面状态、帧快照或缓存引用释放时调用一次；不一定紧随 setImage，也可能在最后释放句柄的任务运行。回调应快速、非阻塞，不抛异常、不重入 View，context 必须活到它执行时。

可把一个 Image 交给多个组件，像素只持有一份。`Image::copy(source, image)` 提供显式创建可共享复制图的方式；`view()` 返回借用像素描述，不能通过它修改存储。

## 4. 同一个组件切换预置资源

所有工程媒体资源都在导出的 ui.h 中拥有稳定查找入口，包括没有用作某个组件初始图的资源。无需为每个候选图片建立重叠节点：

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

该路径复制资源描述符并借用不可变预置像素，避免把 Flash 中的整图搬入 RAM。资源像素必须在所有页面、快照和缓存引用释放前一直有效。生成 ui.h 的静态资源满足该契约；可变解码缓冲区应使用 setImage 或 adopt。

预置多帧资源也可切换。换源会从第 0 帧重新开始，tick 按资源帧率推进；用单帧动态图覆盖动画后，原动画不再驱动该组件。恢复初始资源时重新开始初始动画。

## 5. 清空与恢复默认

```cpp
ui.clearImage(photoId); // 清空像素内容，组件布局、外观和交互仍保留。
ui.resetImage(photoId); // 恢复 ui.h 中该组件的初始资源；初始为空则恢复空内容。
```

与此不同，`setVisible(photoId, false)` 隐藏整个组件。隐藏不会主动丢弃其图片，重新显示仍能绘制现有图。

## 6. 失败、调度与内存约定

| 返回值 | 含义 |
| --- | --- |
| Ok | 已安装新源或完成清空/恢复 |
| NotInitialized | View 尚未 mount |
| InvalidArgument | 元素缺失/不在当前页/不是 Media，或图片参数无效 |
| NoMemory | 无法分配像素或所有权描述块 |

失败保留之前的组件图片和 Image 输出句柄。参数会在读取和分配之前检查：宽高 1–1024，缓冲区至少包含最后一个可见像素，行步距足够，算术不溢出，格式与字节序合法，输入总字节数不超过 8 MiB。

所有 View 更新从一个 UI 任务执行。后台下载/解码可准备独立 Image，通过队列把它交给 UI 任务；入队交接必须保证句柄有效、像素不可变及适当同步。路由切换后 setter 只查当前页：跨页结果留在应用中，进入目标页再安装。

在 DMA 帧途中换图不会修改旧帧：旧帧完整使用旧源，随后新帧使用新源。缓存比较图片身份，避免同一帧号误命中旧缓存。旧图片释放时机由帧和缓存引用决定，不需要应用用 delay 猜测。

内存要允许旧、新图片短暂共存。例如 240×180 RGB565 是约 84.4 KiB 像素；复制上传期间输入与复制图、在途旧图可能同时存在。大图优先 adopt，仍须为在途帧保留的旧图预留空间。`memoryStats()` 去重计入拥有的像素及所有权描述块，预置 Flash 像素不计入动态像素占用。

图片更新使用组件的布局、样式、裁剪、滚动、分块缓冲和批处理机制；更新像素源不会建立整屏帧缓冲或执行 HTML/CSS 重排。刷新吞吐取决于图片尺寸、缓冲预算和实际硬件总线。

## 7. 示例与实现

- [完整 Arduino 动态图片示例](../examples/HTML_UI/DynamicImageDemo/DynamicImageDemo.ino)
- [图片输入与所有权 API](../src/UI/Image.h)
- [View 公共接口](../src/UI/View.h)
- [组件交互说明](ArduinoComponentInteraction.md)

主机回归覆盖像素格式、行步距、裁剪缩放、复制/所有权交接、失败原子性、异步快照、资源动画、跨页保留与缓存失效。实际屏幕的颜色、吞吐和输入仍需按硬件配置验证。
