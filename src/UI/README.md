# Native compiled UI runtime (NICE_GFX 1.0.0, resource ABI 16)

ABI 16 adds immutable chart styling and reference-line descriptors. Line charts
support solid/gradient areas, step traces, finer grids and point markers; bars
support mixed-sign stacking. The editor and exported runtime share these options.
Regenerate complete projects so compiled resources and the runtime match.

The main library is the canonical source for the native runtime and fonts.
The editor application lives only in the sibling `nice_gfx_editor/` directory;
the compiler lives only in `nice_gfx_editor/packages/html-ui/`. Its preview fonts
and firmware archive are generated from this library.
It includes the editor's media, linear gradients, Gauge/Ring/LineChart/BarChart/
Spinbox, physical-key bindings and focus navigation. See
[version unification and migration](../../../design/UI_VERSION_UNIFICATION.md).

`UI/View.h` provides the designer-generated UI runtime. Generated `ProjectDesc`
resources mount into `nice_gfx::htmlui::View`; `View.cpp` is compiled independently
from the canonical `src/` tree. Each text-bearing node explicitly references a
`MonoFont` through `Style::font`; `Style::fontSize` is an integer multiplier from
1 through 7 of that font's native character cell. Measurement, wrapping,
alignment and drawing use the selected font's width and height. The runtime
does not reference a default font or maintain a font registry, so unused fonts
can be removed by the linker. Generated headers explicitly include the font
declarations they use. Export resources with the current editor/compiler;
headers with a different resource ABI report a version mismatch.

Arduino sketches using an installed library include `<NICE_GFX.h>` before their
generated headers so Arduino can discover the library. Plain C++ and ESP-IDF
can include module headers directly. Complete designer exports carry their own
runtime under `src/nice_gfx/` and use relative includes.

Text, Label, Button and Checkbox nodes require a font even when their initial
text is empty, so later `setText()` calls have a defined appearance. Other nodes
may use a null font. Font descriptors, bitmap data and Unicode indices must
remain immutable and alive for the mounted resource's lifetime. Mount validates
font dimensions, bitmap presence, complete contiguous ASCII or a strictly
sorted Unicode index containing the fallback `?`, once per distinct font.

The editor's ten authoring components (image button, setting row, list, select,
choice, tabs, popup, loading, value slider and confirmation) compile into existing
native nodes and immutable component links. They do not need browser-side logic or
additional native node kinds. Generated `ui_config::load()` installs these links
before input/focus setup. Direct View setters and physical-key targets accept
public authoring IDs: SettingRow/ValueSlider proxy values, Select/Choice expose
selection indices, Tabs proxies page indices, and linked captions proxy text.
Select/Popup/Confirm expose `setExpanded/expanded`. Unsupported component setters
return false. Select/Choice commit after a cancelable option Click and emit root
Input/Change with `Event::component` and `optionIndex`; ordinary setters remain
silent except Pager page notifications. `NICE_GFX_UI_COMPONENTS_VERSION = 1`
declares this capability without changing the resource ABI. See the
[Arduino component guide](../../docs/ArduinoComponentInteraction.md) for all 33
components, public IDs, event semantics and runtime limits.

The designer emits a single `ui.h`: `ui_config::load(view)` mounts its immutable
resources and routing options. Bind declared actions with `ui_config::bind_action`
and push user-owned data with `view.setText/setValue(ui_config::id("element"), ...)`.
Keep application data and callback implementations in the sketch. See
[UI/logic separation](../../../design/UI_LOGIC_SEPARATION.md) for the complete workflow.

Media nodes, including empty `gfx-media` placeholders, support runtime image
updates through `setImage`, immutable owning `Image` handles, and
`setMediaResource`. Updates participate in the normal tiled renderer and retain
old pixels for in-flight snapshots. `clearImage` blanks content; `resetImage`
restores the authored source. See [dynamic images](../../docs/DynamicImages.md)
for copied uploads, ownership transfer, compiled resource switching and memory
contracts. This capability uses `NICE_GFX_UI_DYNAMIC_IMAGE_VERSION = 1` without
changing the resource descriptor ABI.

Media pixels honor the node's `border-radius`, including dynamic replacements,
frame playback and transformed animations. The radius is capped at half the
border box's shorter side, then inset by each border and padding edge to clip
the content corners. An 80 x 80 node with a 40px radius is circular; zero keeps
the rectangular fast path. This uses the existing style field without an ABI
change; export the updated runtime with the generated UI.

The runtime requires STL containers and sufficient dynamic memory. AVR is not a
compiled-UI target: `View.cpp` excludes its implementation there so Arduino can
still compile the existing display-only library and examples without STL headers.

The compiler supplies all colors and control-part dimensions. The runtime does
not select a theme. Static resource arrays must remain alive and immutable until
the View is destroyed or another project is mounted. Dynamic `setText` input is
copied; the `text()` result is borrowed until the next setter/mount/destruction.
ID-based getters, setters and bounds queries address the active page. Keep data
for other pages in application state and refresh the active page on RouteChange;
an ID absent from the active page makes a setter return false without a mutation.

Call all APIs from one UI task. Pointer input is a single pointer sequence; a new
down cancels the previous sequence. Reentrant pointer injection from an event
handler is ignored. Events propagate capture, target, then bubble; listeners for
the current event are snapshotted. Scroll reaches capture and target listeners
but does not bubble. Input, Change, PointerCancel, Scroll, PageChange and LongPress
are non-cancelable notifications. Click and PointerDown/Move/Up are cancelable.
`stopImmediatePropagation()` also stops remaining listeners on the current node.
`preventDefault` on Click suppresses activation
and declarative routing. PointerDown/Move prevention does not disable scrolling
or subsequent Click. Checkbox/Switch click listeners observe the tentatively
toggled value; a canceled Click rolls it back and emits no Input or Change.
Range Input is continuous and Change occurs on release only if the final value
differs from the value at pointer down. Programmatic value/text setters are silent
(Pager setters emit Scroll/PageChange). ValueSlider's built-in readout and
selection captions still synchronize on silent value updates; other widgets'
declarative event actions are not replayed. Number InputField value setters reject
invalid bounds; non-number fields accept text, not numeric value setters.

Calling `pointerCancel()` from an event callback ends the active sequence,
including during PointerUp, Click, Input, or gesture ownership transfer. Hiding
or disabling the pointer target, its ancestors, the active control (including
a label-associated control in another subtree), or the drag owner has the same
effect. The current event keeps its normal propagation, but no later pointer
defaults, Change notification, or declarative navigation run for that sequence.
A tentative Checkbox/Switch value is rolled back if cancellation happens during
Click; a value already reported by Input is retained. Canceling in an edge-back
BeforeNavigate callback or route guard clears the preview and preserves history.

Scroll and Pager cancel the child pointer sequence when their drag wins. Range
drag is horizontal and independent of touch-action's native scroll restrictions.
Scroll accepts one configured axis with bounded, damped inertia; a new down stops
inertia at its current offset and consumes a click inside the moving viewport.
Pager is horizontal, follows the pointer,
uses a velocity threshold on release, then snaps with a 180 ms ease-out animation.
Call `tick(nowMilliseconds)` regularly for animation and long-press timing. An
interrupted snap stays at its current offset, consumes the next click, and settles
after release. `finish()` drains pixel transfers for the current render snapshot;
it does not advance animation time or automatically drain a pending next frame.

Register route events with `on(0, EventType::BeforeNavigate/RouteChange/
NavigationCancel, ...)`. BeforeNavigate can prevent the transition; other route
notifications are non-cancelable. Route events do not bubble. RouteGuard
is synchronous. Navigation requested from a node event is deferred until the
whole pointer operation ends. Callbacks triggered by a route transition can queue
another transition for a later tick. Push history is limited to 64 entries;
page values and scroll positions are initialized on first visit and retained until
mount or explicit release. Back returns to an existing retained page. Key navigation
retains page focus. `releasePage(path)` reclaims an inactive page outside history,
transitions and in-flight snapshots; revisiting reconstructs compiled defaults.
Edge back is disabled by default; `setEdgeBackEnabled(true)` enables a 24-pixel
left-edge gesture on non-control content. Guards run before a preview is prepared;
the previous/current pages are drawn together during the drag. Release completes
or rolls back over 180 ms with pointer input gated. Canceled drags leave history
unchanged; a successful animation commits history only at its completion.

`GestureBindingDesc` adds optional trailing `interaction` and `transition` fields
in navigation API version 3; existing eight-field resources keep Release/null.
Release triggers after a direction-dominant swipe reaches its threshold. Null
inherits project/page and component departure effects; an explicit descriptor
uses one paired page transition with the complete target pose, overriding page
phases and component entry/departure.
Explicit gesture directions are never reversed for Back/Home. Instant commits
without page or component entry/departure motion and renders the final target pose.
Same-page actions and Back without history do not animate.

Follow requires a matching SlideLeft/Right/Up/Down descriptor (null defaults to
matching 180 ms EaseOut). It prepares the guard and complete target pose when
the gesture wins, renders both pages at the pointer distance on either axis,
and commits Push/Replace/Back/Home history only after the remaining slide settles.
Settle duration is the configured duration times the remaining screen fraction,
rounded up to at least 1 ms; the configured easing applies to that remainder.
Short, reversed, cross-axis or canceled gestures return to the source, emit no
Click, and retain history. `pointerCancel()` and `cancelNavigation()` rebound a
prepared Follow gesture; route guards and BeforeNavigate can reject preparation.
Input is gated during settling; ordinary route groups keep their existing motion.

Layout is compiled for one target width/height. Mount rejects a different display
size. Geometry is page-relative, in physical pixels. Runtime text changes do not
automatically resize nodes. `setBounds/resetBounds` move or resize nodes;
`setStyle/resetStyle` change copied style descriptors. `layoutBounds` returns
geometry before scrolling/animation; `bounds` returns visual geometry.
`layoutChildren(id, LayoutAxis::Vertical/Horizontal, gap, resizeContainer)` explicitly
places visible direct children of a Container/Scroll, preserving child sizes and
moving their subtrees. Pager retains its compiled stride: resize/content-box changes
and direct page geometry changes are rejected. This bounded operation does not
implement general CSS Flex/Grid. Text wraps by Unicode codepoint with the node's font;
there is no shaping, bidi, rich-inline layout or general text selection. InputField
and Spinbox provide bounded cursor editing through the shared automatic keyboard,
with separate draft/committed values, confirmation/cancellation, UTF-8 editing and
fixed-point validation. `InputFieldDesc::keyboardHeight` uses fullscreen editing
when zero; a positive height docks at the bottom, bounded by the screen height
and a 240px minimum. Round screens always use fullscreen editing. ABI 14 adds
`InputFieldDesc::keyboardTheme` (`KeyboardTheme::Light` / `Dark`, default Light).
`setKeyboardTheme(KeyboardTheme)` overrides the field theme across field changes
and hiding; `resetKeyboardTheme()` restores the field theme; `keyboardTheme()`
returns the effective theme. Theme changes preserve draft and committed values.
The taller editing area supports tap and horizontal drag to position the cursor.
Key labels scale with each touch cell at integer bitmap sizes, shrinking longer
labels to leave 6px on each side. The default 16px font becomes 32px on an
800x480 fullscreen keyboard, up to 48px in larger cells; small, round and docked
layouts fit their available space. ASCII labels use compact half-width spacing.
The heading reserves a 48px band for rectangular keyboards at least 360px high,
and round keyboards at least 480px wide, with a 32px default-font title and 8px
above/below. Other grid layouts use a 24px band, 16px title and 4px padding.
Round-screen safe widths use the actual title glyph band; the draft and bottom
row avoid the narrow circular edges. Numeric keyboards below 360px high use
four rows, retaining backspace, confirmation and the field's sign/decimal or
telephone controls. Tall screens center the editing panel and cap the grid
height by its width. The 800x480 layout retains 84px letter keys. Confirmation
uses its Font Awesome icon when the measured text cannot fit with padding.
Long titles first shrink, then end in `...` at a complete UTF-8 boundary;
validation errors use the same padded heading.
Screens below 120px wide or keyboards below 200px high use a picker instead of
compressing multiple key rows. The draft remains visible alongside a selected
character/group or command. Tap an arrow, swipe the selected cell, or use
physical previous/next keys to cycle choices; tap the cell or confirm to activate
it. A swipe locks to its initial dominant axis and cancels tap activation.
Text and symbols use six groups/pages. Hide, reveal, backspace and confirmation
remain reachable choices, and selected backspace retains hold-to-repeat.
Very short strips show the draft and selection side by side without arrows;
physical keys or swipes navigate them. Tiny panels benefit from physical keys:
pixel dimensions alone do not establish a finger-sized physical target. Picker
layouts without a heading display validation errors through the draft border.
Touch Backspace deletes one codepoint immediately. Holding it repeats after
400ms, then every 80ms; after 2 seconds held, the interval becomes 50ms. Each
`tick(now)` deletes at most one codepoint and rebases the next interval, so a
slow frame never causes a catch-up burst. Release does not add a deletion.
Moving outside the original key's touch area, cancellation, hiding, changing
input sessions or rebuilding the key layout stops the hold; moving back does
not restart it. Reaching the beginning of the draft stops repeat work. Theme
changes preserve a hold when geometry stays unchanged. Call `tick(millis())`
regularly to service repeat; `finish()` only drains rendering. Programmatic
`backspaceInput()` and `keyboardKey("backspace")` remain single operations.
`TouchInput` pauses repeats on its first no-contact sample. Contact returning
within the normal 30ms release debounce resumes the same hold and its original
deadline, without another initial deletion. Stable release ends the hold.
Custom touch adapters can use `pausePointerRepeat(true/false)` around report gaps
and `stopPointerRepeat()` for a permanent stop, before the final `pointerUp()`.
The toolbar contains only Hide and, for password / PIN fields, Reveal. Full
QWERTY layouts align Hide's right edge and width with the final first-row
cell (P in letter mode), retaining the same geometry in symbol mode. Reveal
uses the same width, with 4px gaps between controls and the draft; both controls
remain as tall as the draft field. Grouped, numeric, circular and picker layouts
retain their respective control profiles.
`keyboardKey("done")` activates the current confirmation key; `"hide"` and
`"backspace"` activate their current keys. Use `cancelTextInput()`, `clearInput()`,
`deleteInput()` and `moveInputCursor()` for direct draft editing. Regenerate
`ui.h` and update the runtime together. Text and Label nodes remain display-only.
Physical keys support sequential/spatial focus and activation. Focus outlines,
including automatic keyboard key outlines, start hidden and disappear on pointer
down, even on empty content. `inputButton/inputAction` reveal them for physical-key
operation; routing retains the last input mode. Logical focus and Focus/Blur events
remain active in touch mode. Programmatic `focus(id)` changes focus without changing
the input mode. Local media uses
preconverted RGB565 or RGB332 frames; arbitrary HTML img loading, general Grid/Flex reflow, arbitrary CSS properties,
per-part DOM nodes, per-state border/radius overrides, CSS group opacity, and browser
script execution are not implemented. The compiler must reject unsupported
syntax. Text resource input is expected to be valid UTF-8.

Shape fill/stroke colors, stroke width and points belong to immutable ShapeDesc
resources; changing Style does not rewrite those vector attributes. Use a dynamic
container background, icon color or preauthored shape visibility for status colors.
Expanded-panel focus transfer requires InputConfig; generated physical-key/focus
settings install it. Popup/Confirm do not block background input or trap focus.

Each submitted frame copies scalar state and shares immutable dynamic text,
chart samples and style/geometry overrides. Writers copy chart samples only while
a frame still owns the old data; completed snapshots release shared references.
Compiled text stays in the resource storage until replaced. TileCanvas reads the
snapshot while RefreshScheduler protects DMA buffer ownership; next-frame changes
retain their dirty rectangles. Geometry, ancestor clipping and text line spans are
prepared once per frame. A 32-band bitset selects candidate nodes in original paint
order for each tile; text drawing uses spans without temporary strings. Setters,
visibility and component animations invalidate affected old/new subtree rectangles;
routing, page transitions and edge preview still repaint the screen.

`beginUpdate/endUpdate` nest and postpone new frames until the outer update ends.
An already submitted frame can drain during the batch; setters are immediately
visible to getters and input, so this is a render batch, not a rollback transaction.
Font/gradient resources used by style overrides must remain alive and immutable
until every referring page and in-flight frame releases them.

`memoryStats()` estimates retained state/snapshot/cache/payload/tile/scratch capacity
with shared payloads counted once; it excludes allocator metadata, driver storage
and immutable flash resources. `runtimeStats/resetRuntimeStats` expose cumulative
frame, geometry, text layout, candidate, paint and transfer-pixel work. See
[optimization measurements and examples](../../../design/UI_RUNTIME_OPTIMIZATION.md).

For component APIs, events, input handling and application scheduling, see the
[Arduino component interaction guide](../../docs/ArduinoComponentInteraction.md).

## Declarative component animations (ABI 4; independent exit requires ABI 10)

Each NodeDesc has an AnimationDesc: effect, easing, trigger, duration/delay in ms,
repeat count (0 = infinite), and distance in pixels. Zero initialization disables
animation. Effects: FadeIn/FadeOut, SlideLeft/Right/Up/Down, Float, Bounce, Shake,
Pulse (opacity breathing), Blink. SlideLeft/Right start on the corresponding side;
SlideUp starts below and SlideDown starts above. Easing: Linear, EaseIn, EaseOut,
EaseInOut, BackOut and BounceOut. One preset per node, without layout reflow;
ABI 15 adds the optional compiled keyframes and local transforms described below.

ABI 10 appends `NodeDesc::exitAnimation`, an optional immutable `AnimationDesc`
pointer. Exit effects are FadeOut and SlideOutLeft/Right/Up/Down; the latter keep
their final displacement. Exit descriptors use Manual and one finite cycle.
They are independent of the existing enter/click/manual preset. Accepted ordinary
navigation plays all initially visible exit animations from the current visual
pose, waits for their final frame/DMA completion, then changes the route. A hidden
exit node does not delay navigation. Guard rejection starts no exit animations.
`cancelNavigation()` cancels an uncommitted leave or Follow slide, restores animation poses,
keeps application mutations and history, and emits NavigationCancel once.

`PageDesc::enterTransition` and `exitTransition` are optional immutable descriptor
pointers: null inherits the project default; explicit None disables that side.
When either the outgoing exit or incoming enter is overridden, page exit completes
before history/RouteChange commit and page entry follows. Component entry runs
with page entry. Without page overrides the existing paired project transition
is preserved, after any component leave. Back/Home reverse slide direction.
An explicit initial-page enter override also runs on mount; an inherited project
transition preserves the existing immediate first-page behavior.
Completed interactive group/edge-back drags retain their gesture motion and do
not replay these authored effects. Mount/reset clears pending navigation.

Leave and entry keep using integer sampling, bounded state snapshots and the tile
renderer. Opacity needs tile scratch, never an extra full-screen framebuffer.
Call tick regularly through all phases; finish drains only the current frame.

Enter animations restart on mount and every successful page navigation; their
clock starts at the first tick after entry. Click animations restart on an allowed
Click and its ancestors, after Click listeners; canceled clicks do not start them.
Manual animations run only through playAnimation. Duration is 1–60000 ms, delay
0–60000 ms, repeat 0–1000, distance 0–1024 px; invalid descriptors fail mount.
Delay applies once before the first cycle. Finite animations retain their last
sample; fade-out ends at opacity zero and slide-out retains its displacement.
Other presets return to their base pose.

~~~cpp
// ID lookup and timing use the same contract as pointer input.
ui.playAnimation(ui_config::id("status"), millis()); // restart configured preset
ui.stopAnimation(ui_config::id("status"));           // restore base pose / opacity
bool active = ui.animationRunning(ui_config::id("status"));
// Call from the application loop, including while animations are running:
ui.tick(millis());
~~~

bounds(), clipping and hit testing include the node's and ancestors' translations.
Opacity multiplies down the ancestry and composites each node once over the tile
in RGB565 (not an offscreen subtree/group opacity). Fully transparent nodes cannot
receive a new hit. Hidden subtrees do not drive busy() or repaint; an already
started hidden animation catches up on the next visible tick. Off-page animation
state does not drive busy(); re-entering resets it. Infinite cycles handle uint32
clock rollover when tick is serviced regularly. All APIs must run on one UI task.

Animation changes invalidate old/new subtree bounds to erase old positions and
overlapping descendants. Rendering uses immutable per-frame snapshots, including opacity.
Fades lazily allocate at most one extra RGB565 tile of scratch memory (2 bytes per
tile pixel); they do not require a full-screen framebuffer. When finite animations
finish and dirty tiles drain, idle tick performs no display writes. finish() only
drains the current frame, never advances animation time. Test actual bus throughput
on hardware; no fixed frame rate is promised.

## Compiled keyframes and local transforms (ABI 17, motion API 1)

`NodeDesc::motion` optionally references an immutable `MotionDesc`, with 2–16
normalized `MotionKeyframe` poses. The compiler resolves missing fields, delay
staggering and resource deduplication before export. Device sampling uses Q10
integer progress for x/y, opacity, uniform scale, clockwise integer-degree
rotation and rectangular reveal. Keyframes replace the node's main preset and
retain its `AnimationDesc` timing. A zero-frame descriptor attaches alternate
direction to a legacy preset; a null pointer keeps the original sampling path.
`repeat` counts single traversals, so alternate repeat=2 returns to the first
pose. Infinite alternate clocks rebase only whole pairs of traversals.

Rotation and uniform scale use one fixed pivot per node. Optional
`data-animation-pivot-x` and `data-animation-pivot-y` are integer percentages
from -1000 to 1000; omitted axes default to 50 (the existing center behavior).
The compiler stores them as trailing `MotionDesc::pivotX` / `pivotY` fields;
`NICE_GFX_UI_PIVOT_MOTION_VERSION=1` identifies this capability. ABI 17 changes
the motion descriptor layout; re-export resources when upgrading the runtime.
Existing C++ `{frames,count,flags}` source initializers retain the center defaults,
including Arduino toolchains using C++11; a constexpr constructor supplies the
optional pivot values without changing descriptor layout.

For Media/Icon, 0/100 percent select the node box edges. For Shape they select
the first/last pixel centers, so a filled polygon endpoint at logical
`50,100` remains fixed with pivot `50,100`. Percentages apply to the full
node, including padding and borders; authored line vertices still use their
existing half-stroke inset. A stroked line endpoint therefore differs from the
box-edge pivot by that inset. Negative values and values above 100 place the
pivot outside the node. Position/opacity/reveal retain their existing meaning;
the pivot only affects rotation/scale and follows the node's animated position.
Rendering, `bounds()`, pointer hits and old/new dirty regions share the pivot.

~~~html
<gfx-polygon id="hand" style="position:absolute;left:100px;top:30px;width:11px;height:71px"
  points="50,100 20,0 80,0" data-view-width="100" data-view-height="100"
  fill="#ffffff" stroke="none" data-animation-pivot-x="50" data-animation-pivot-y="100"
  data-keyframes='[{"at":0,"rotation":0},{"at":100,"rotation":360}]'
  data-animation-duration="60000" data-animation-repeat="infinite"
  data-animation-easing="linear"></gfx-polygon>
~~~

Scale (0.125–4) and rotation (-360–360 degrees) are supported for Media/Icon
leaves, including their background, gradient, padding and border. The tiled
renderer samples their original image/mask directly, with fixed-point inverse
coordinates, nearest-neighbor filtering and the icon's coverage. It allocates
no full-image layer, keeps the same source coordinates across tile boundaries,
and applies node opacity once. Bounds and pointer hits use the same quantized
transform. Containers, text and controls support translation, opacity and
reveal, but cannot scale/rotate. Reveal clips descendants and pointer hits from
the selected edge; it does not change layout or resize content.

`NICE_GFX_UI_SHAPE_MOTION_VERSION=1` also enables scale/rotation on all Shape
leaves without changing the resource ABI. Authored line/polygon vertices are
transformed around the configured full-node pivot (center by default); circles, ellipses and rounded rectangles
use a bounded 64-segment contour during transforms. The original renderer remains
in use for unchanged poses. Geometry and destination clipping stay independent
of display tile boundaries. A fixed 64-pixel row block composites fill, stroke,
background and border before applying opacity once, with no heap or full-image
buffer. Shape keyframes cannot combine any nonidentity scale/rotation with any
partial reveal; both the compiler and native mount reject that combination.

`show(id)` reveals a node and restarts Enter/Show motion. `hide(id)` runs its
independent exit once from the current pose, then hides it after the final
accepted refresh completes. Without an exit it hides immediately. Repeated
hide is idempotent; show, stop and immediate `setVisible` cancel a pending hide.
`setVisible` retains its existing immediate behavior. Component expansion uses
the animated operations without changing Select's accepted Click/Input/Change
sequence.

`EventType::AnimationEnd` is a bubbling, non-cancelable notification after a
finite main animation or animated hide completes its final refresh. `value=0`
identifies main motion and `value=1` hide. Infinite loops, cancellation, stop,
restart and navigation departure do not emit it. Callback restarts/navigation
are protected from stale completion epochs. `finish()` drains the current
refresh but does not advance animation clocks.

Only animation-bearing nodes own sparse motion slots; live clocks and immutable
snapshot poses are separate. Storage is reserved during page initialization.
After first-use warming, sampling, transforms and ordinary completion delivery
allocate no heap memory. Unchanged integer poses invalidate no pixels. Legacy
animations without completion listeners incur no additional end-frame fence.
The [Motion Lab project](../../../nice_gfx_editor/public/motion-lab.gfx-project.json)
demonstrates rotation, alternate scaling, reveal/slide/fade and animated show/hide.


Native pivot regression (real View, tile rendering, Media/Icon/Shape, pointer hits,
dirty cleanup and integer overflow checks): `node nice_gfx/tests/motion-pivot.mjs --sanitize`
from the workspace root. The runner uses an existing host C++ compiler only.
