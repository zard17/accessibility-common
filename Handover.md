# Handover: GDBus Web Inspector

## What was done

### GDBus Web Inspector (NEW)

Added a web inspector variant that queries the accessibility tree through **real D-Bus IPC** via a private `dbus-daemon` + `FakeAtspiBroker`. Every tree/element query goes through full GDBus serialization/deserialization, proving the end-to-end round-trip.

#### Files created/modified

| File | Status | Description |
|------|--------|-------------|
| `tools/inspector/web-inspector-gdbus-main.cpp` | New | Main entry point — private dbus-daemon, FakeAtspiBroker, HTTP server + GLib main loop |
| `tools/inspector/query-engine.h` | Modified | Added `InitializeGDBus(busAddress, pumpMainLoop)` declaration |
| `tools/inspector/query-engine.cpp` | Modified | Added `InitializeGDBus()` implementation; guarded `Initialize()` behind `INSPECTOR_NO_MOCK_DBUS` |
| `build/tizen/CMakeLists.txt` | Modified | Added `BUILD_WEB_INSPECTOR_GDBUS` option + target with `INSPECTOR_NO_MOCK_DBUS` define |
| `CLAUDE.md` | Modified | Added GDBus web inspector build/run docs + Important Files entry |

#### Design

- `InitializeGDBus()` shares 90% of `Initialize()` logic but skips `MockDBusWrapper` installation (GDBusWrapper auto-creates) and pumps GLib main context after bridge init for async D-Bus operations
- `pumpMainLoop` callback parameter avoids `#include <gio/gio.h>` in query-engine.cpp, keeping mock builds working
- `INSPECTOR_NO_MOCK_DBUS` compile definition disables `Initialize()` body and mock include for the GDBus target
- HTTP server runs on background thread; main thread pumps `g_main_context_iteration` for D-Bus events

### Test results

- Existing mock-based tests: **56/56 passed**
- GDBus unit tests: **48/48 passed**
- GDBus integration tests: **42/42 passed**
- GDBus web inspector: starts, serves tree/element JSON via real D-Bus IPC
- Direct (no-IPC) web inspector: still works correctly

## What's next

1. **Phase 2.6**: TIDL IPC backend plan
2. **Phase 3**: AccessibilityService base class (NodeProxy, AppRegistry, GestureProvider)
3. **Phase 4a**: Screen reader C++ rewrite

## Verification commands

```bash
cd ~/tizen/accessibility-common/build/tizen/build

# GDBus web inspector
cmake .. -DENABLE_ATSPI=ON -DBUILD_WEB_INSPECTOR_GDBUS=ON -DENABLE_PKG_CONFIGURE=ON
make -j8
./accessibility-web-inspector-gdbus        # default port 8080
./accessibility-web-inspector-gdbus 9000   # custom port

# All tests
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DBUILD_GDBUS_TESTS=ON -DENABLE_PKG_CONFIGURE=ON
make -j8
./accessibility-test                       # 56 passed
./accessibility-gdbus-unit-test            # 48 passed
./accessibility-gdbus-integration-test     # 42 passed
```
