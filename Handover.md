# Handover: Phase 4.6 — TV Screen Reader Demo (DALi FocusManager + TvScreenReaderService)

## 완료된 Phase 상태

| Phase | Status |
|-------|--------|
| 2.5 (GDBus) | **DONE** — `dbus-gdbus.cpp`, unit 48개 + integration 42개 테스트 통과 |
| 2.6 (TIDL) | **Stage A DONE** — scaffold + tidlc 코드 생성 검증. Stage B/C는 Tizen 디바이스 필요 |
| 2.7 (Tree embedding) | **DONE** — Socket/Plug embed/unembed/setOffset 테스트 완료 |
| 3 (AccessibilityService) | **DONE** — 55개 서비스 테스트 + 56개 기존 테스트 통과 |
| 4 (Concrete Services) | **DONE** — InspectorService (47 tests) + ScreenReaderService (120 tests) |
| 4.5 (Screen Reader Demo) | **DONE** — 실제 DALi 앱 + 임베디드 ScreenReaderService + macOS TTS |
| **4.6 (TV Screen Reader Demo)** | **DONE** — DALi KeyboardFocusManager + TvScreenReaderService + macOS TTS |

## Phase 4.6: TV Screen Reader Demo

### 개요

TV 프로파일용 screen reader 데모. 기존 Phase 4.5 데모가 gesture 기반(키보드로 flick 시뮬레이션)인 반면, TV 데모는 DALi의 `KeyboardFocusManager`가 직접 focus를 이동하고, `FocusChangedSignal`을 통해 `TvScreenReaderService`에 이벤트를 전달하여 TTS 발화. 리모컨(방향키) 기반 TV 경험을 macOS에서 재현.

### ScreenReaderService vs TvScreenReaderService

| | ScreenReaderService (Phase 4.5) | TvScreenReaderService (Phase 4.6) |
|---|---|---|
| Trigger | `STATE_CHANGED detail="highlighted"` | `STATE_CHANGED detail="focused"` |
| Navigation | Service가 `navigateNext()` 호출 | **KeyboardFocusManager**가 focus 이동 |
| Gesture | 키보드→gesture 매핑 필요 | **No-op** (StubGestureProvider) |
| Dependencies | 7개 | **4개** (registry, gesture, tts, settings) |
| ReadingComposer | 기본 (touch hints 포함) | TV config (touch hints 억제) |

### 아키텍처

```
DALi Application (real Controls: TextLabel, PushButton, CheckBox, ProgressBar)
  │
  ├── KeyboardFocusManager (DALi built-in)
  │     Arrow keys → MoveFocus() → FocusChangedSignal 발생
  │     Enter → FocusedActorEnterKeySignal 발생
  │
  ├── FocusChangedSignal 콜백
  │     1. ActorAccessible::Get(newFocused)
  │     2. DirectNodeProxy 생성/조회 (proxy cache)
  │     3. service->highlightNode(proxy) → currentNode 설정
  │     4. service->dispatchEvent(STATE_CHANGED, "focused", 1)
  │     5. → TvScreenReaderService::onAccessibilityEvent
  │        → readNode(getCurrentNode()) → TTS speaks
  │
  └── TvScreenReaderService (embedded in-process)
        ├── DirectAppRegistry (root accessible → NodeProxy)
        ├── StubGestureProvider (no-op)
        ├── MacTtsEngine (AVSpeechSynthesizer)
        └── StubSettingsProvider
```

### 생성된 파일 (1)

| File | Description |
|------|-------------|
| `tools/screen-reader/screen-reader-tv-demo.cpp` | Main binary — DALi 480×800 창 + 6개 실제 컨트롤 + KeyboardFocusManager + TvScreenReaderService |

### 수정된 파일 (3)

| File | Change |
|------|--------|
| `build/tizen/CMakeLists.txt` | `BUILD_SCREEN_READER_TV_DEMO` option 추가 |
| `CLAUDE.md` | Repository structure, demo 실행 방법, Important Files 추가 |
| `Handover.md` | Phase 4.6 문서화 |

### 핵심 설계 결정

1. **KeyboardFocusManager 활용**: DALi의 built-in focus manager가 방향키 처리, focus indicator 표시, focus 이동을 자동 수행. `PreFocusChangeSignal`로 navigation order만 정의.
2. **FocusChangedSignal → dispatchEvent**: Focus 변경 시 `STATE_CHANGED(focused)` 이벤트를 `TvScreenReaderService`에 전달. 서비스의 `onAccessibilityEvent`가 자동으로 `readNode()` 호출.
3. **OnEnterPressed는 로깅만**: DALi가 Enter 키를 처리할 때 `ClickedSignal`과 `FocusedActorEnterKeySignal` 모두 발생. `OnEnterPressed`에서 수동 상태 변경 시 double-toggle 발생 → 로깅만 수행하고 실제 동작은 `ClickedSignal` 콜백에 위임.
4. **Proxy cache**: Demo 클래스에서 별도 `weak_ptr` proxy cache 관리. `DirectAppRegistry`와 독립적으로 `OnFocusChanged`에서 proxy 조회.

### 빌드 및 실행

```bash
cd ~/tizen/accessibility-common/build/tizen/build

cmake .. -DENABLE_ATSPI=ON -DBUILD_SCREEN_READER_TV_DEMO=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8

export DYLD_LIBRARY_PATH=$HOME/tizen/dali-env/lib
./accessibility-screen-reader-tv-demo
```

### 키보드 단축키

| Key | Action |
|-----|--------|
| Up / Down | Focus 이동 (KeyboardFocusManager 자동 처리) |
| Left / Right | Focus 이동 (KeyboardFocusManager 자동 처리) |
| Enter | 활성화 (FocusedActorEnterKeySignal → ClickedSignal) |
| t | Accessibility tree stdout 출력 |
| Esc / q | 종료 |

### 동작 확인 사항

- DALi 480×800 창에 실제 컨트롤 표시 (title, Play, Stop, Volume, Autoplay, Status)
- **Up/Down 키**로 focus 이동 → TTS가 "Play, Button" 등 읽어줌 (TV 프로파일: touch hint 미발화)
- **Enter 키**로 활성화 → 버튼 클릭, 체크박스 토글 (double-toggle 없이 정상 동작)
- **t** → accessibility tree 출력
- **Esc/q** → 종료

## 테스트 결과 (전체 278 tests)

```bash
# ScreenReaderService 테스트 (기존 120개 모두 통과)
cmake .. -DENABLE_ATSPI=ON -DBUILD_SCREEN_READER_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-screen-reader-test
# === Results: 120 passed, 0 failed ===
```

## 다음 Phase

### Phase 5: Toolkit Integration
- `dali-adaptor`가 accessibility-common에 의존 (코드 포함 대신)
- `dali-toolkit`의 `ControlAccessible`이 `Accessible` interface 구현
- DALi adaptor lifecycle에서 `PlatformCallbacks` 연결
- Zero behavior change 보장

### Tizen 디바이스 필요 작업
- **Phase 2.6 Stage B/C**: TIDL backend 실제 구현
- **Tizen scaffolds 채움**: tizen-tts-engine, tizen-feedback-provider, tizen-settings-provider, tizen-screen-reader-switch, wm-gesture-provider
- **SystemNotifications**: battery/wifi/BT/display status announcements
- **Language span parsing**: Multi-language TTS

### Deferred
- `AurumService` — automated accessibility testing service
- `WindowOrderingPolicy` — multi-window navigation ordering
- Key event handling 확장
