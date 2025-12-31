# MMFI surface comparison: XLua vs. Duktape port

This document highlights the exposed pieces of the `mmf` table in the Duktape-backed extension and how they map to XLua's MMFI API. XLua's shipping MMFI exposes five default objects (`frame`, `keyboard`, `mouse`, `runtime`, `window`) and constructors (`newObject`, `newObjectClass`) alongside a rich set of object/class helpers. The current port keeps the namespace compatible while only implementing a subset of the runtime helpers.

## Implemented compatibility points
- `mmf.interpreter` and `mmf.api` strings are present so scripts can detect the port and version.
- `mmf.runtime.currentFrame()` mirrors XLua's ability to read the current frame index.
- `mmf.runtime.objectCount()` exposes the active object count just like XLua's runtime helper.
- `mmf.runtime.triggerEvent(id)` lets scripts raise Fusion events from JavaScript.
- `mmf.frame` now exposes live properties for `xLeft`, `xRight`, `yTop`, `yBottom`, `width`, `height`, `virtualWidth`, and `virtualHeight` plus `testPoint`/`testRect` helpers that validate coordinates against the current viewport.
- `mmf.keyboard.keyDown(key)` / `keyUp(key)` poll the state of a virtual key, and `VK_*` constants (arrows, space, return, escape, shift, control, alt) are populated on Windows builds for convenience.
- `mmf.mouse` surfaces `x`, `y`, `clientX`, `clientY`, `wheelDelta`, `buttonDown(button)`, and `buttonUp(button)` (1=left, 2=middle, 3=right, 4/5=XButtons) using Fusion's runtime state.
- `mmf.window` provides `width`, `height`, `clientWidth`, `clientHeight`, `frameWidth`, and `frameHeight` derived from the main and edit window handles on Windows.

## Stubbed, not yet implemented
- `mmf.newObject(...)` and `mmf.newObjectClass(...)` exist but immediately raise an error to highlight that object/class construction is not yet supported.
- Frame collision helpers still perform viewport-bound checks only; backdrop/object collision parity with XLua is not yet available.

## Behavior when missing features are used
Calling any of the stubbed members will raise a Duktape error identifying the missing feature so scripts can fall back or guard against unavailable APIs at runtime.
