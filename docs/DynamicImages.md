# 运行时更新图片

通过 `nice_gfx::htmlui::View` 更新 Media 节点，图片参与裁剪、动画、页面与缓存管理。下载和 PNG/JPEG 解码由应用完成。此功能要求 `NICE_GFX_UI_DYNAMIC_IMAGE_VERSION >= 1`；升级时同步运行库与导出的 `ui.h`，资源兼容性以 [Compiled.h](../src/UI/Compiled.h) 为准。

## 准备图片区域

在设计器插入“图片 / 视频”，设置稳定 ID（如 `photo`），勾选“设备端更新”。可以使用预置初始图，也可保留空区域：

```html
<gfx-media id="photo" data-media-dynamic="true"
  style="width:240px;height:180px"></gfx-media>
```

“设备端更新”要求显式 ID；带初始资源的普通图片也能调用更新接口。换图不改变布局，按最近邻缩放填满扣除边框与 padding 的内容区。`border-radius` 同样作用于动态图片和多帧资源；80×80px 节点设置 `40px` 可显示圆形图片，当前不支持百分比半径。

以下片段使用已加载工程的 `View ui`，在同一 UI 任务调用；完整接入见 [组件交互](ArduinoComponentInteraction.md)。

## 复制像素

默认 `setImage(ImageView)` 复制输入，适合解码器、相机或业务生成的数组：

```cpp
using nice_gfx::Status;
using nice_gfx::htmlui::ImageView;
constexpr auto photoId = ui_config::id("photo");

Status updatePhoto(const uint16_t *pixels, size_t width, size_t height,
                   size_t availableBytes) {
  return ui.setImage(photoId, ImageView(pixels, width, height, availableBytes));
}
```

`Status::Ok` 表示已完成复制，可以立即复用或释放输入；屏幕刷新仍需后续 `pollUi()/tick()`。支持以下像素格式，RGB565 字节序与 `gfx.setSwapBytes()` 无关：

| 输入 | `ImageView` 参数 |
| --- | --- |
| 普通 `uint16_t` RGB565（红色为 `0xF800`） | 默认 `ImageFormat::RGB565, PixelOrder::Native` |
| RGB565 高字节在前的字节流 | `ImageFormat::RGB565, PixelOrder::Wire` |
| RGB332，每像素一字节 | `ImageFormat::RGB332` |

最后一个参数 `strideBytes` 是字节步距，0 表示紧密排列；`byteLength` 至少覆盖最后一行的可见像素，不必包含该行末尾填充：

```cpp
ui.setImage(photoId, ImageView(bytes, width, height, byteCount,
  nice_gfx::htmlui::ImageFormat::RGB565, nice_gfx::PixelOrder::Wire, rowStrideBytes));
```

动态 `Image` 没有独立 alpha 或透明色键，透明图需先合成背景。复制结果按紧密行保存，RGB565 转为高字节在前。

## 交接大图所有权

已解码到堆或 PSRAM 的大图可用 `Image::adopt()` 避免再次复制整图：

```cpp
#include <cstdlib>
using nice_gfx::htmlui::Image;

void freePixels(void *, const void *pixels) {
  std::free(const_cast<void *>(pixels));
}

// 本函数接管与 std::free 匹配的缓冲区。
Status installDecodedPhoto(uint16_t *pixels, size_t width, size_t height,
                          size_t availableBytes) {
  Image image;
  const Status result = Image::adopt(
    ImageView(pixels, width, height, availableBytes), freePixels, nullptr, image);
  if (result != Status::Ok) {
    std::free(pixels); // adopt 失败时尚未接管。
    return result;
  }
  return ui.setImage(photoId, image);
}
```

adopt 仅在成功时接管；之后像素不可改写或自行释放。其他分配器应使用匹配的 release 回调。`setImage(Image)` 共享不可变源，局部 Image 可随即销毁；若安装失败，局部句柄销毁时正常释放其引用。

release 在最后一个句柄、页面、帧或缓存引用释放时调用一次，可能在释放引用的任务中执行。回调必须快速、非阻塞、不抛异常、不重入 View，context 须活到最后一个引用释放。旧图仍在使用时不要强制释放。

同一个 Image 可供多个组件共享；`Image::copy()` 显式创建复制图，`view()` 返回借用描述，使用期间须保持句柄有效且不变。后台可准备独立 Image，再通过队列交给 UI 任务，保证句柄有效、像素不可变及线程同步。

## 预置资源与透明色键

用工程媒体资源 ID 切换预置图片或多帧资源；该 ID 不是元素 ID 或文件路径：

```cpp
Status result = ui_config::setMedia(ui, photoId,
                                   online ? "asset-online" : "asset-offline");
// 底层等价接口：
const auto *resource = ui_config::media("asset-online");
if (resource) ui.setMediaResource(photoId, *resource);
```

`setMediaResource()` 复制描述符、借用不可变像素，像素须保持有效直到页面、快照与缓存引用释放。生成 `ui.h` 的静态资源满足要求；可变 RAM 应用 `setImage/copy/adopt`。换源从第 0 帧开始，tick 按资源帧率推进；单帧动态图覆盖后不再播放原动画，reset 后重新播放初始资源。

预置 RGB565 资源可通过工程的 `media[].transparentColor` 指定透明色（0–65535），导出器会生成相应资源并检查 `NICE_GFX_UI_MEDIA_COLOR_KEY_VERSION >= 1`。匹配像素保留底层画面，支持缩放、圆角与变换；这是二值透明，节点 opacity 仍可控制可见像素。应避开前景使用的颜色。此设置不适用于动态 Image 或 RGB332。

## 清空、恢复与离页回收

```cpp
ui.clearImage(photoId); // 清空内容，保留布局、外观与交互。
ui.resetImage(photoId); // 恢复 ui.h 中的初始资源，或初始空内容。
```

`setVisible(photoId, false)` 隐藏整个组件但保留图片；`hasImage(photoId, image)` 检查是否持有同一不可变源，不比较像素。

一般图片 setter 只查当前页。跨页下载结果应由应用保存，在目标页进入后安装；离页后可在 RouteChange 中调用 `clearPageImages(event.from)`，清空该非活动页的 RAM Image，保留其他控件状态和预置资源。未知路径或当前页返回 false；帧与缓存持有的旧图仍按引用生命周期释放。

## 返回值与内存

| 返回值 | 含义 |
| --- | --- |
| Ok | 已安装、清空或恢复；不代表屏幕传输完成 |
| NotInitialized | View 尚未 mount |
| InvalidArgument | 当前页元素缺失或不是 Media，或图片参数无效 |
| NoMemory | 无法分配像素或所有权描述块 |

失败保留原图片；`Image::copy/adopt` 失败也保留输出句柄。参数限制：宽高 1–1024、输入字节数不超过 8 MiB、步距足够、缓冲区覆盖可见像素、格式与字节序有效且计算不溢出。

在途帧继续用旧源，后续帧用新源，缓存按图片身份失效，无需 delay 等待。内存须允许旧、新图片短暂共存；240×180 RGB565 约需 84.4 KiB，复制上传期间还保留输入缓冲。adopt 可省一次复制，但不能省去仍被帧引用的旧图。`memoryStats()` 去重统计拥有的像素和描述块，预置 Flash 像素不计入动态像素占用。

示例：[DynamicImageDemo](../examples/07_UI/DynamicImageDemo/DynamicImageDemo.ino)。完整契约：[Image.h](../src/UI/Image.h)、[View.h](../src/UI/View.h)。
