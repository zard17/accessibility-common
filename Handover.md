# Handover: Phase 4.5 — Screen Reader Demo (Real DALi App)

## 완료된 Phase 상태

| Phase | Status |
|-------|--------|
| 2.5 (GDBus) | **DONE** — `dbus-gdbus.cpp`, unit 48개 + integration 42개 테스트 통과 |
| 2.6 (TIDL) | **Stage A DONE** — scaffold + tidlc 코드 생성 검증. Stage B/C는 Tizen 디바이스 필요 |
| 2.7 (Tree embedding) | **DONE** — Socket/Plug embed/unembed/setOffset 테스트 완료 |
| 3 (AccessibilityService) | **DONE** — 55개 서비스 테스트 + 56개 기존 테스트 통과 |
| 4 (Concrete Services) | **DONE** — InspectorService (47 tests) + ScreenReaderService (120 tests) |
| **4.5 (Screen Reader Demo)** | **DONE** — 실제 DALi 앱 + 임베디드 ScreenReaderService + macOS TTS |

## Phase 4.5: Screen Reader Demo

### 개요

실제 DALi 앱의 accessibility tree와 ScreenReaderService를 in-process로 연결하는 데모. D-Bus 없이 `Accessible*`를 직접 호출하여 navigation, TTS 발화, highlight 동작.

### 아키텍처

```
DALi Application (real Controls: TextLabel, PushButton, ProgressBar, CheckBox)
  │
  ├── Window + RootLayer
  │     ControlAccessible 자동 생성 (toolkit factory)
  │
  └── ScreenReaderService (embedded in-process)
        ├── DirectAppRegistry (wraps bridge root → DirectNodeProxy)
        ├── KeyboardGestureProvider (KeyEvent → GestureInfo)
        ├── MacTtsEngine (AVSpeechSynthesizer)
        ├── StubFeedbackProvider / StubSettingsProvider
        ├── StubScreenReaderSwitch / StubDirectReadingService
        └── ReadingComposer → TtsCommandQueue → TTS speaks
```

### 생성된 파일 (5)

| File | Description |
|------|-------------|
| `tools/screen-reader/direct-node-proxy.h` | `Accessible*`를 wrapping하는 `NodeProxy` 구현. 42 메서드 직접 호출. `getReadingMaterial()`에서 CHECKABLE 상태 role로부터 추론 |
| `tools/screen-reader/direct-app-registry.h` | Bridge root accessible → `AppRegistry` 구현. `weak_ptr` proxy cache |
| `tools/screen-reader/mac-tts-engine.h` | `TtsEngine` macOS 구현 header (PIMPL) |
| `tools/screen-reader/mac-tts-engine.mm` | AVSpeechSynthesizer wrapping. `TtsImplData` + ObjC delegate. speak/stop/pause/resume/purge 지원 |
| `tools/screen-reader/screen-reader-demo.cpp` | Main binary — DALi 480×800 창 + 5개 실제 컨트롤 + 임베디드 ScreenReaderService + 키보드 제스처 매핑 |

### 수정된 파일 (2)

| File | Change |
|------|--------|
| `accessibility/internal/service/screen-reader/reading-composer.cpp` | `compose()` reading order를 state→name→role→description으로 변경 (Tizen production 순서 일치) |
| `build/tizen/CMakeLists.txt` | `BUILD_SCREEN_READER_DEMO` option + FIND_LIBRARY dali2-core/adaptor/toolkit + APPLE framework linking |

### 핵심 설계 결정

1. **In-Process (not D-Bus)**: macOS에 D-Bus daemon 불필요. `Accessible*` 직접 호출. `DirectNodeProxy`가 `NodeProxy` 인터페이스 구현.
2. **CHECKABLE 추론**: DALi `CheckBoxButton`이 CHECKED는 설정하지만 CHECKABLE은 미설정. `DirectNodeProxy::getReadingMaterial()`에서 role (CHECK_BOX, RADIO_BUTTON, TOGGLE_BUTTON)로부터 추론.
3. **직접 활성화**: DALi `DoAction("activate")`가 macOS에서 `ClickedSignal` 미발화. `ActivateCurrentNode()`에서 `getCurrentNode()` + 이름 매칭으로 직접 처리.
4. **Reading order**: Tizen production screen reader (`reading_composer.c`) 참조하여 state→name→role→description 순서로 수정. 120개 테스트 통과.
5. **ProgressBar 사용**: `Slider` 심볼이 macOS toolkit에서 미 export. `ProgressBar`로 대체.

### 빌드 및 실행

```bash
cd ~/tizen/accessibility-common/build/tizen/build

cmake .. -DENABLE_ATSPI=ON -DBUILD_SCREEN_READER_DEMO=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8

export DYLD_LIBRARY_PATH=$HOME/tizen/dali-env/lib
./accessibility-screen-reader-demo
```

### 키보드 단축키

| Key | Gesture | Action |
|-----|---------|--------|
| Right / n | FLICK_RIGHT | 다음 요소 이동 + TTS |
| Left / b | FLICK_LEFT | 이전 요소 이동 + TTS |
| Enter / d | DOUBLE_TAP | 활성화 (버튼 클릭, 체크박스 토글) |
| Space / p | TWO_FINGER_TAP | TTS 일시정지/재개 |
| r | THREE_FINGER_TAP | 처음부터 읽기 |
| Up / Down | — | ProgressBar 값 조정 (±10%) |
| t | — | Accessibility tree stdout 출력 |
| Esc / q | — | 종료 |

### 데모 동작 확인 사항

- DALi 480×800 창에 실제 버튼, 체크박스, 프로그레스바, 라벨 표시
- → 키로 navigate하면 TTS가 "Not checked, Autoplay, Check box" 등 읽어줌
- Enter로 체크박스 토글하면 상태 반영 + TTS 재발화
- Up/Down으로 프로그레스바 값 조정 + 상태 라벨 업데이트

## 테스트 결과 (전체 278 tests)

```bash
# ScreenReaderService 테스트 (reading order 변경 포함)
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
