# accessibility-common

Platform-agnostic accessibility library extracted from [DALi](https://review.tizen.org/git/?p=platform/core/uifw/dali-adaptor.git).

Provides the AT-SPI bridge, accessible tree interfaces, IPC abstraction layer, and AT-side service framework (AccessibilityService, ScreenReaderService).

## Architecture

```
accessibility-common/
├── accessibility/
│   ├── api/             # Public API (Accessible, Bridge, NodeProxy, AccessibilityService, ...)
│   └── internal/
│       ├── bridge/      # App-side AT-SPI bridge
│       │   ├── ipc/     # IPC abstraction (Server, Client interfaces)
│       │   ├── dbus/    # D-Bus backend (GDBus + eldbus)
│       │   └── tidl/    # TIDL backend (scaffold)
│       └── service/     # AT-side service framework
│           └── screen-reader/
├── tools/
│   ├── inspector/       # CLI / Web accessibility inspector
│   └── screen-reader/   # Screen reader demos (gesture + TV focus)
├── test/                # 278 tests (bridge 56, service 55, inspector 47, screen-reader 120)
└── docs/                # Architecture docs, design rationale
```

## Build

```bash
cd build/tizen && mkdir -p build && cd build

# macOS (stub D-Bus)
cmake .. -DENABLE_ACCESSIBILITY=ON -DENABLE_PKG_CONFIGURE=OFF
# Ubuntu (with GDBus)
cmake .. -DENABLE_ACCESSIBILITY=ON

make -j$(nproc)
```

### Build Options

| Option | Description |
|--------|-------------|
| `-DBUILD_TESTS=ON` | Bridge tests (56) |
| `-DBUILD_SERVICE_TESTS=ON` | AccessibilityService tests (55) |
| `-DBUILD_SCREEN_READER_TESTS=ON` | ScreenReader tests (120) |
| `-DBUILD_SCREEN_READER_DEMO=ON` | Gesture-based screen reader demo (requires DALi) |
| `-DBUILD_SCREEN_READER_TV_DEMO=ON` | TV focus-based screen reader demo (requires DALi) |
| `-DBUILD_WEB_INSPECTOR_DIRECT=ON` | Web inspector (in-process, no D-Bus) |
| `-DBUILD_WEB_INSPECTOR_GDBUS=ON` | Web inspector (real D-Bus IPC) |

## Tests

```bash
./accessibility-test                # 56 passed
./accessibility-service-test        # 55 passed
./accessibility-screen-reader-test  # 120 passed
```

## GDBus Web Inspector

Interactive browser-based accessibility inspector with real D-Bus IPC round-trip.

**Prerequisites:** `dbus-daemon` and `gio-2.0` (`sudo apt install dbus libglib2.0-dev` on Ubuntu, `brew install dbus glib` on macOS)

```bash
cd build/tizen && mkdir -p build && cd build
cmake .. -DENABLE_ACCESSIBILITY=ON -DBUILD_WEB_INSPECTOR_GDBUS=ON
make accessibility-web-inspector-gdbus -j$(nproc)
./accessibility-web-inspector-gdbus    # opens on http://localhost:8080
```

Navigate with the web UI or curl:
```bash
curl -s http://localhost:8080/api/tree | python3 -m json.tool
curl -s -X POST http://localhost:8080/api/navigate \
  -H 'Content-Type: application/json' -d '{"direction":"next"}'
```

See [docs/inspector-architecture.md](docs/inspector-architecture.md) for full details.

## Documentation

See [docs/architecture-overview.md](docs/architecture-overview.md) for full design, phase history, and decision log.

## License

Apache License 2.0 - Samsung Electronics Co., Ltd.
