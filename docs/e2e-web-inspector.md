# E2E Accessibility Web Inspector — Testing Guide

## Overview

The E2E web inspector proves the full pipeline:

```
DALi rendering -> ActorAccessible / ControlAccessible -> DirectQueryEngine -> WebInspectorServer -> Browser
```

There are two ways to run it:

| Binary | Repo | Tree Source | Requires DALi? |
|--------|------|-------------|----------------|
| `accessibility-web-inspector-direct` | accessibility-common | TestAccessible (hardcoded demo) | No |
| `accessibility-inspector.example` | dali-demo | Live DALi actors (real UI) | Yes |

---

## Prerequisites

Set the install prefix (all commands below assume this):

```bash
export DESKTOP_PREFIX=$HOME/tizen/dali-env
```

---

## Option A: Standalone Inspector (No DALi Required)

Uses `TestAccessible` objects — good for verifying the inspector infrastructure in isolation.

### Build

```bash
cd ~/tizen/accessibility-common/build/tizen
mkdir -p build && cd build

cmake .. \
  -DENABLE_ACCESSIBILITY=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_WEB_INSPECTOR_DIRECT=ON \
  -DENABLE_PKG_CONFIGURE=OFF

make -j8
```

### Run tests (should print "31 passed, 0 failed")

```bash
./accessibility-test
```

### Run the standalone inspector

```bash
./accessibility-web-inspector-direct
```

Output:

```
=== Direct Web Accessibility Inspector ===

Snapshot built: root=1000, 11 elements
Web inspector: http://localhost:8080
Press Ctrl+C to stop.
```

Open http://localhost:8080 in a browser. You should see:

```
[WINDOW] "Main Window"
  [PANEL] "Header"
    [PUSH_BUTTON] "Menu"
    [LABEL] "My Tizen App"
  [PANEL] "Content"
    [PUSH_BUTTON] "Play"
    [SLIDER] "Volume"
    [LABEL] "Now Playing: Bohemian Rhapsody"
  [PANEL] "Footer"
    [PUSH_BUTTON] "Previous"
    [PUSH_BUTTON] "Next"
```

### Verify via curl

```bash
# Tree
curl -s http://localhost:8080/api/tree | python3 -m json.tool

# Element detail (e.g., Play button, id=1005)
curl -s http://localhost:8080/api/element/1005 | python3 -m json.tool
```

Press `Ctrl+C` to stop.

---

## Option B: Full E2E with DALi Demo App

Uses live DALi actors — proves the real `ActorAccessible` -> inspector pipeline.

### Step 1: Build and install accessibility-common with inspector library

```bash
cd ~/tizen/accessibility-common/build/tizen/build

cmake .. \
  -DENABLE_ACCESSIBILITY=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_INSPECTOR_LIB=ON \
  -DENABLE_PKG_CONFIGURE=OFF \
  -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX \
  -DLIB_DIR=$DESKTOP_PREFIX/lib \
  -DINCLUDE_DIR=$DESKTOP_PREFIX/include

make -j8
./accessibility-test          # verify 31 passed
make install                  # installs libaccessibility-inspector.a + headers
```

Verify install:

```bash
ls $DESKTOP_PREFIX/lib/libaccessibility-inspector.a
ls $DESKTOP_PREFIX/include/tools/inspector/direct-query-engine.h
```

### Step 2: Build the DALi demo app

```bash
cd ~/tizen/dali-demo/build/tizen

cmake -DBUILD_EXAMPLE_NAME=accessibility-inspector \
  -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/tizen/vcpkg/scripts/buildsystems/vcpkg.cmake .

make accessibility-inspector.example -j8
```

You should see in the cmake output:

```
-- Included dependencies for accessibility-inspector
--   Inspector lib: .../libaccessibility-inspector.a
--   Accessibility lib: .../libaccessibility-common.dylib
```

### Step 3: Run the demo

```bash
cd ~/tizen/dali-demo/build/tizen
./examples/accessibility-inspector.example
```

Output:

```
Web inspector: http://localhost:8080
```

A DALi window opens showing the UI controls. Open http://localhost:8080 in a browser.

### Step 4: Verify in browser

Expected tree in the web inspector:

```
[WINDOW] "RootLayer"
  [REDUNDANT_OBJECT] "DefaultCamera"
  [LABEL] "Accessibility Demo"
  [PUSH_BUTTON] "Play"
  [ENTRY] "Search..."
  [CHECK_BOX] "Mute"
  [LABEL] "Web inspector: http://localhost:8080"
```

### Verify via curl (while the app is running)

```bash
# Full tree
curl -s http://localhost:8080/api/tree | python3 -m json.tool

# Individual elements (IDs are DALi actor IDs, typically small integers)
curl -s http://localhost:8080/api/element/1 | python3 -m json.tool   # RootLayer
curl -s http://localhost:8080/api/element/3 | python3 -m json.tool   # Title label
curl -s http://localhost:8080/api/element/4 | python3 -m json.tool   # Play button
curl -s http://localhost:8080/api/element/9 | python3 -m json.tool   # Mute checkbox
```

Press `ESC` in the DALi window to quit.

---

## E2E Checklist

### Standalone inspector (`accessibility-web-inspector-direct`)

- [ ] Binary starts without errors
- [ ] `http://localhost:8080` loads the web UI
- [ ] Tree panel shows 11 elements (Window + 3 panels + 7 leaf nodes)
- [ ] Clicking a tree node shows element details (name, role, states, bounds)
- [ ] Bounds are non-zero (e.g., Play button: x=200, y=300, w=80, h=80)
- [ ] States include ENABLED, VISIBLE, SHOWING for all elements
- [ ] PUSH_BUTTON elements have FOCUSABLE + HIGHLIGHTABLE states
- [ ] LABEL elements have HIGHLIGHTABLE but NOT FOCUSABLE
- [ ] `/api/tree` returns valid JSON with `focusedId` and `tree` fields
- [ ] `/api/element/:id` returns valid JSON with all fields populated

### DALi demo app (`accessibility-inspector.example`)

- [ ] DALi window opens with visible controls (title, button, text field, checkbox)
- [ ] `http://localhost:8080` loads the web UI
- [ ] Tree shows real DALi actors (RootLayer as root)
- [ ] TextLabel "Accessibility Demo" has role=LABEL
- [ ] PushButton "Play" has role=PUSH_BUTTON, states include FOCUSABLE
- [ ] TextField "Search..." has role=ENTRY, states include EDITABLE
- [ ] CheckBoxButton "Mute" has role=CHECK_BOX, states include FOCUSABLE
- [ ] Bounds are real screen coordinates (non-zero, proportional to window size)
- [ ] RootLayer bounds match window dimensions
- [ ] ESC key quits the app cleanly (no crash)

---

## Troubleshooting

### "ACCESSIBILITY ERROR: No DBusWrapper installed"

Expected on macOS. The bridge falls back to a dummy implementation, but `ActorAccessible::Get()` and `DirectQueryEngine` work without D-Bus.

### Empty tree / null root accessible

- Ensure the snapshot is built **after** controls are added to the window
- `ActorAccessible::Get(window.GetRootLayer())` should return non-null
- If it returns null, the bridge may not have created the `ActorAccessible` for the root layer yet

### "accessibility-inspector library not found"

- Run `make install` in accessibility-common first
- Check that `$DESKTOP_PREFIX/lib/libaccessibility-inspector.a` exists
- Re-run cmake in dali-demo to pick up the installed library

### Port 8080 already in use

The standalone inspector accepts a port argument:

```bash
./accessibility-web-inspector-direct 9000
```

The demo app uses port 8080 (hardcoded). Kill any existing process on that port:

```bash
lsof -ti:8080 | xargs kill
```
