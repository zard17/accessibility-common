# Inspector Architecture

> Parent document: [architecture-overview.md](architecture-overview.md)

Both the CLI and web inspectors share a common engine (`AccessibilityQueryEngine`) that encapsulates bridge initialization, demo tree construction, D-Bus queries, and navigation.

```
                    AccessibilityQueryEngine
                   (query-engine.h/.cpp)
                    |                    |
          +---------+----------+   +----+-----+
          |                    |   |          |
   CLI Inspector        Web Inspector    (future consumers)
   (inspector.cpp)      (web-inspector.cpp)
          |                    |
   stdin/stdout         HTTP REST API
                        (cpp-httplib)
                              |
                         Browser UI
                   (web-inspector-resources.h)
```

---

## AccessibilityQueryEngine (tools/inspector/query-engine.h/.cpp)

Reusable engine that initializes a demo accessible tree and provides query/navigation methods. Consumers call engine methods instead of making raw D-Bus queries.

```
AccessibilityQueryEngine
  +-- Initialize()              # MockDBusWrapper + PlatformCallbacks + bridge + demo tree
  +-- Shutdown()                # Bridge teardown
  +-- GetRootId() / GetFocusedId() / SetFocusedId()
  +-- GetElementInfo(id)        # Returns ElementInfo (name, role, states, bounds, children)
  +-- BuildTree(rootId)         # Returns TreeNode hierarchy
  +-- Navigate(id, forward)     # Forward/backward via bridge GetNeighbor
  +-- NavigateChild(id)         # First child
  +-- NavigateParent(id)        # Parent
```

Internally, the engine uses `DBusClient` to query the bridge's registered AT-SPI interfaces through MockDBusWrapper — the same code path as a real screen reader.

```
AccessibilityQueryEngine
  |
  +-- DBusClient.method("GetRole").call()
  |     |
  v     v
Bridge (BridgeImpl)
  |
  +-- Registered AT-SPI interfaces (same code as production)
  |
  v
MockDBusWrapper (in-process routing)
```

---

## Demo Tree

The engine builds a demo tree modeling a Tizen media player app:

```
[WINDOW] "Main Window"                       <- ACTIVE
  [PANEL] "Header"
    [PUSH_BUTTON] "Menu"                     <- FOCUSABLE + HIGHLIGHTABLE
    [LABEL] "My Tizen App"                   <- HIGHLIGHTABLE (not FOCUSABLE)
  [PANEL] "Content"
    [PUSH_BUTTON] "Play"                     <- FOCUSABLE + HIGHLIGHTABLE
    [SLIDER] "Volume"                        <- FOCUSABLE + HIGHLIGHTABLE
    [LABEL] "Now Playing: Bohemian Rhapsody" <- HIGHLIGHTABLE (not FOCUSABLE)
  [PANEL] "Footer"
    [PUSH_BUTTON] "Previous"                 <- FOCUSABLE + HIGHLIGHTABLE
    [PUSH_BUTTON] "Next"                     <- FOCUSABLE + HIGHLIGHTABLE
```

**FOCUSABLE vs HIGHLIGHTABLE**: Buttons and sliders are both FOCUSABLE (can receive keyboard focus) and HIGHLIGHTABLE (navigable by screen reader cursor). Labels are HIGHLIGHTABLE only — the screen reader can navigate to them and announce their text, but they don't accept keyboard focus. The bridge's `GetNeighbor()` walks elements with HIGHLIGHTABLE state; `IsObjectAcceptable()` in `bridge-accessible.cpp` checks VISIBLE + HIGHLIGHTABLE.

---

## CLI Inspector (tools/inspector/inspector.cpp)

Thin interactive wrapper (~220 lines) over `AccessibilityQueryEngine`. Provides single-key commands for tree exploration and TTS:

| Key | Action |
|-----|--------|
| `p` | Print accessibility tree (`>>` marks focused element) |
| `n`/`b` | Navigate forward/backward |
| `c`/`u` | Navigate to first child / parent |
| `r` | Read current element details |
| `s` | Speak current element via system TTS |
| `h` | Show help |
| `q` | Quit |

TTS uses `AVSpeechSynthesizer` on macOS (`tts-mac.mm`) and prints to console elsewhere (`tts-stub.cpp`).

---

## Web Inspector (tools/inspector/web-inspector.cpp)

Browser-based GUI served via embedded HTTP server (~230 lines). Uses cpp-httplib (single-header, MIT licensed, vendored at `third-party/cpp-httplib/httplib.h`).

**REST API:**

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Serves embedded HTML/CSS/JS page |
| `/api/tree` | GET | Full tree JSON + current focusedId |
| `/api/element/:id` | GET | Element details (name, role, states, bounds, children, parent) |
| `/api/navigate` | POST | Navigate: `{"direction": "next\|prev\|child\|parent"}` |

**Frontend** (`web-inspector-resources.h`): Embedded as a C++ raw string literal (`R"HTMLPAGE(...)HTMLPAGE"`) for single-binary deployment. Features:
- Dark Catppuccin-themed two-panel layout (tree + detail)
- Click-to-select tree nodes with collapse/expand
- Navigation buttons: Prev, Next, Child, Parent, Refresh
- Keyboard shortcuts: Tab/Shift+Tab, Enter, Backspace, S (speak), R (refresh)
- TTS via Web Speech API (browser-side, no server dependency)

A mutex protects the engine from concurrent HTTP request handlers. JSON is serialized manually (no external JSON library — only 3 endpoints).
