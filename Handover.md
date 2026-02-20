# Handover: Phase 4 — Concrete Services (InspectorService + ScreenReaderService)

## 완료된 Phase 상태

| Phase | Status |
|-------|--------|
| 2.5 (GDBus) | **DONE** — `dbus-gdbus.cpp`, unit 48개 + integration 42개 테스트 통과 |
| 2.6 (TIDL) | **Stage A DONE** — scaffold + tidlc 코드 생성 검증. Stage B/C는 Tizen 디바이스 필요 |
| 2.7 (Tree embedding) | **DONE** — Socket/Plug embed/unembed/setOffset 테스트 완료 |
| 3 (AccessibilityService) | **DONE** — 55개 서비스 테스트 + 56개 기존 테스트 통과 |
| **4 (Concrete Services)** | **DONE** — InspectorService (47 tests) + ScreenReaderService (120 tests) |

## Phase 4 아키텍처

```
AccessibilityService (base, Phase 3)
  │
  ├── InspectorService (Step 1)
  │     NodeProxyQueryEngine + WebInspectorServer
  │     트리 snapshot → REST API + Web UI
  │
  ├── ScreenReaderService (Step 2, Full mode)
  │     ReadingComposer + TtsCommandQueue + TtsEngine
  │     FeedbackProvider + SettingsProvider + ScreenReaderSwitch
  │     Gesture dispatch → navigate → read → TTS + sound/haptic
  │
  └── TvScreenReaderService (Step 2, TV mode)
        ReadingComposer (suppressTouchHints, includeTvTraits)
        Focus event → read → TTS (no gestures)
```

## Step 1: InspectorService (47 tests)

### 생성된 파일 (7)

| File | Description |
|------|-------------|
| `tools/inspector/inspector-query-interface.h` | `InspectorQueryInterface` 추상 인터페이스 (GetRootId, Navigate, NavigateChild/Parent) |
| `tools/inspector/node-proxy-query-engine.h` | NodeProxy 기반 snapshot engine header |
| `tools/inspector/node-proxy-query-engine.cpp` | Thread-safe 트리 snapshot (mutex 보호, HTTP 서버 접근) |
| `accessibility/internal/service/inspector-service.h` | `InspectorService` header (extends AccessibilityService) |
| `accessibility/internal/service/inspector-service.cpp` | startInspector/stopInspector, WINDOW_CHANGED → auto-refresh |
| `test/test-inspector-service.cpp` | 47개 유닛 테스트 |
| `tools/inspector/inspector-service-main.cpp` | Standalone binary |

### 수정된 파일 (4)

| File | Change |
|------|--------|
| `tools/inspector/direct-query-engine.h/.cpp` | `InspectorQueryInterface` 구현, `NavigateChild`/`NavigateParent` 추가 |
| `tools/inspector/web-inspector-server.h/.cpp` | `Start()` takes `InspectorQueryInterface&`, child/parent 핸들링 |
| `build/tizen/CMakeLists.txt` | `BUILD_INSPECTOR_SERVICE_TESTS` + `BUILD_INSPECTOR_SERVICE` 옵션 |

## Step 2: ScreenReaderService (120 tests)

### API 헤더 (7)

| File | Key Types |
|------|-----------|
| `accessibility/api/tts-engine.h` | `TtsEngine` (speak/stop/pause/resume/purge/callbacks), `SpeakOptions{discardable, interrupt}`, `CommandId` |
| `accessibility/api/feedback-provider.h` | `FeedbackProvider` (playSound/vibrate), `SoundType` enum (7 types) |
| `accessibility/api/reading-composer.h` | `ReadingComposer`, `ReadingComposerConfig{suppressTouchHints, includeTvTraits}` |
| `accessibility/api/settings-provider.h` | `SettingsProvider`, `ScreenReaderSettings` (7 fields: readDescription, haptic, keyboard, sound, lcd, ttsSpeed, multiWindow) |
| `accessibility/api/screen-reader-switch.h` | `ScreenReaderSwitch` (setScreenReaderEnabled, getScreenReaderEnabled, setIsEnabled, setWmEnabled) |
| `accessibility/api/direct-reading-service.h` | `DirectReadingService` (start(TtsEngine&), stop()) |
| `accessibility/api/screen-reader-service.h` | `ScreenReaderService` (7 deps) + `TvScreenReaderService` (4 deps) |

### Pure Logic (5 files, platform-agnostic)

| File | Description |
|------|-------------|
| `accessibility/internal/service/screen-reader/reading-composer.cpp` | ~30 roles, state/description traits, TV/Full config |
| `accessibility/internal/service/screen-reader/tts-command-queue.h` | Config{maxChunkSize=300}, Command queue header |
| `accessibility/internal/service/screen-reader/tts-command-queue.cpp` | Discard policy, 300-char word-boundary chunking, pause/resume |
| `accessibility/internal/service/screen-reader/symbol-table.h` | 53 symbol→spoken text mappings header |
| `accessibility/internal/service/screen-reader/symbol-table.cpp` | Static unordered_map lookup |

### Service 구현 (2)

| File | Description |
|------|-------------|
| `screen-reader/screen-reader-service.cpp` | Full mode: PIMPL, gesture dispatch (flick→navigate, double-tap→activate, etc.), event→read+sound, key events |
| `screen-reader/tv-screen-reader-service.cpp` | TV mode: focus event→read, WINDOW_CHANGED→announce, gesture no-op |

### Stubs — macOS/CI (5)

| File | Description |
|------|-------------|
| `screen-reader/stub/stub-tts-engine.h` | Console-print TTS (speak→stdout) |
| `screen-reader/stub/stub-feedback-provider.h` | No-op sound/haptic |
| `screen-reader/stub/stub-settings-provider.h` | Returns defaults for all 7 settings |
| `screen-reader/stub/stub-screen-reader-switch.h` | No-op enable/disable |
| `screen-reader/stub/stub-direct-reading-service.h` | No-op D-Bus service |

### Tizen Scaffolds (5)

| File | Description |
|------|-------------|
| `screen-reader/tizen/tizen-tts-engine.h` | Wraps Tizen `tts.h` API (TODO scaffold) |
| `screen-reader/tizen/tizen-feedback-provider.h` | Wraps `mm_sound` + `haptic` (TODO scaffold) |
| `screen-reader/tizen/tizen-settings-provider.h` | Wraps `vconf` for 7 keys (TODO scaffold) |
| `screen-reader/tizen/tizen-screen-reader-switch.h` | D-Bus property set on `org.a11y.Status` + WM (TODO scaffold) |
| `screen-reader/tizen/wm-gesture-provider.h` | D-Bus signal listener on `org.tizen.GestureNavigation` (TODO scaffold) |

### Test/Mock 파일 (5)

| File | Description |
|------|-------------|
| `test/mock/mock-tts-engine.h` | Records speak/stop/pause/purge calls, configurable CommandId |
| `test/mock/mock-feedback-provider.h` | Records playSound/vibrate calls |
| `test/mock/mock-settings-provider.h` | Configurable settings, fire callbacks |
| `test/mock/mock-screen-reader-switch.h` | Records enable/disable call counts |
| `test/test-screen-reader-service.cpp` | 120 tests (13 test functions) |

### 수정된 파일 (2)

| File | Change |
|------|--------|
| `accessibility/internal/service/file.list` | `accessibility_common_screen_reader_src_files` 변수 추가 (5 .cpp) |
| `build/tizen/CMakeLists.txt` | `BUILD_SCREEN_READER_TESTS` 옵션 + `accessibility-screen-reader-test` 타겟 |

## 테스트 결과 (전체 278 tests)

```bash
cd ~/tizen/accessibility-common/build/tizen/build

# 기존 브릿지 테스트
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-test
# === Results: 56 passed, 0 failed ===

# AccessibilityService 테스트
cmake .. -DENABLE_ATSPI=ON -DBUILD_SERVICE_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-service-test
# === Results: 55 passed, 0 failed ===

# InspectorService 테스트
cmake .. -DENABLE_ATSPI=ON -DBUILD_INSPECTOR_SERVICE_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-inspector-service-test
# === Results: 47 passed, 0 failed ===

# ScreenReaderService 테스트
cmake .. -DENABLE_ATSPI=ON -DBUILD_SCREEN_READER_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-screen-reader-test
# === Results: 120 passed, 0 failed ===
```

### 테스트 상세 (120 screen reader tests)

| Category | Count | Key Tests |
|----------|-------|-----------|
| SymbolTable | 5 | "."→"dot", "@"→"at sign", ","→"comma", unknown→empty |
| ReadingComposer role traits | 10 | PUSH_BUTTON→"Button", CHECK_BOX→"Check box", SLIDER→"Slider", ~30 roles |
| ReadingComposer state traits | 10 | checked/unchecked, selected, expanded/collapsed, disabled, read-only, required |
| ReadingComposer description traits | 6 | slider value+hint, button hint, TV suppress, TV popup count |
| ReadingComposer compose | 5 | full composition, name priority, textIfceName fallback, empty |
| TtsCommandQueue | 20 | speak, queue ordering, completion advance, purge, pause/resume, interrupt, chunking |
| ScreenReaderService lifecycle | 11 | start/stop, switch+WM enable, double start/stop, destructor, accessors |
| ScreenReaderService gestures | 10 | flick right/left, double tap, two-finger tap, three-finger tap, feedback |
| ScreenReaderService events | 5 | highlighted, property changed, window changed, after stop, sound disabled |
| ScreenReaderService keys | 2 | placeholder key event tests |
| TvScreenReaderService | 12 | lifecycle, destructor, focused/property/window events, gesture no-op |
| Settings/Switch | 5 | settings callback, language callback, keyboard callback, switch records |
| ReadNode | 3 | null node, not running, valid node |

## 핵심 설계 결정

1. **Composition over Inheritance**: `ScreenReaderService`와 `TvScreenReaderService` 둘 다 `AccessibilityService`를 직접 상속. 공통 로직 (ReadingComposer, TtsCommandQueue)은 composition으로 공유. `#ifdef SCREEN_READER_TV` 완전 제거.
2. **ReadingComposerConfig**: TV vs Full 차이를 `{suppressTouchHints, includeTvTraits}` flag로 처리. Subclass 불필요.
3. **TtsCommandQueue**: Pure C++ queue — 300-char word-boundary chunking, discardable/non-discardable commands, pause/resume. `TtsEngine` 추상 인터페이스를 통해 platform TTS 호출.
4. **Stub/Scaffold 패턴**: 5개 Tizen scaffold (TODO bodies)와 5개 macOS stub (no-op/defaults)로 디바이스 없이 전체 빌드+테스트 가능.
5. **InspectorQueryInterface**: `DirectQueryEngine`과 `NodeProxyQueryEngine` 공통 인터페이스. WebInspectorServer가 둘 다 사용 가능.
6. **start()/stop() non-virtual**: Base class의 의도적 설계. 서비스별 `startScreenReader()`/`startInspector()` 패턴.

## 다음 Phase

### Phase 5: Toolkit Integration
- `dali-adaptor`가 accessibility-common에 의존 (코드 포함 대신)
- `dali-toolkit`의 `ControlAccessible`이 `Accessible` interface 구현
- DALi adaptor lifecycle에서 `PlatformCallbacks` 연결
- Zero behavior change 보장

### Tizen 디바이스 필요 작업
- **Phase 2.6 Stage B/C**: TIDL backend 실제 구현 (`tidlc` generated proxy 코드 채움)
- **Tizen scaffolds 채움**: `tizen-tts-engine.h` (tts.h), `tizen-feedback-provider.h` (mm_sound+haptic), `tizen-settings-provider.h` (vconf), `tizen-screen-reader-switch.h` (D-Bus org.a11y.Status), `wm-gesture-provider.h` (WM gesture D-Bus signal)
- **SystemNotifications**: battery/wifi/BT/display status announcements (Full mode only)
- **Language span parsing**: Multi-language TTS (GLib json-glib dependency)

### Deferred (Pure C++ 추가 가능)
- `AurumService` — automated accessibility testing service
- `WindowOrderingPolicy` — multi-window navigation ordering (Full mode)
- Key event handling 확장 (현재 placeholder)
