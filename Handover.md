# Handover: Phase 3 — AccessibilityService Base Class

## 완료된 Phase 상태

| Phase | Status |
|-------|--------|
| 2.5 (GDBus) | **DONE** — `dbus-gdbus.cpp`, unit 48개 + integration 42개 테스트 통과 |
| 2.6 (TIDL) | **Stage A DONE** — scaffold + tidlc 코드 생성 검증. Stage B/C는 Tizen 디바이스 필요 |
| 2.7 (Tree embedding) | **DONE** — Socket/Plug embed/unembed/setOffset 테스트 완료 |
| **3 (AccessibilityService)** | **DONE** — AT-side 프레임워크, 55개 서비스 테스트 + 56개 기존 테스트 통과 |

## Phase 3: 완료된 작업

### 아키텍처

```
Concrete Services (Phase 4)
  ScreenReaderService / InspectorService / AurumService
         |  extends
  AccessibilityService (base class)
    start(), stop(), navigateNext(), navigatePrev(), highlightNode()
    virtual: onAccessibilityEvent(), onWindowChanged(), onGesture(), onKeyEvent()
         |  uses               |  receives from        |  receives from
    NodeProxy (~42 methods)   AppRegistry             GestureProvider
    (abstract interface)      (abstract interface)
         |                         |
    ┌────┴────┐            ┌───────┴───────┐
    │         │            │               │
AtSpiNode  TidlNode   AtSpiApp        TidlApp
Proxy      Proxy       Registry        Registry
(D-Bus)    (rpc_port)  (D-Bus bus)     (Tizen aul)
```

### 생성된 파일 (27개)

#### Public API (5)

| File | Description |
|------|-------------|
| `accessibility/api/accessibility-event.h` | AccessibilityEvent struct (10 event types) |
| `accessibility/api/node-proxy.h` | NodeProxy 인터페이스 (42 methods) + ReadingMaterial/NodeInfo/RemoteRelation/DefaultLabelInfo structs |
| `accessibility/api/app-registry.h` | AppRegistry 인터페이스 (getDesktop, getActiveWindow, callbacks) |
| `accessibility/api/gesture-provider.h` | GestureProvider 인터페이스 |
| `accessibility/api/accessibility-service.h` | AccessibilityService base class (PIMPL) |

#### D-Bus 구현 (8)

| File | Description |
|------|-------------|
| `accessibility/internal/service/atspi-node-proxy.h` | D-Bus NodeProxy header |
| `accessibility/internal/service/atspi-node-proxy.cpp` | D-Bus NodeProxy — 42개 메서드 전부 구현 (DBus::DBusClient 사용) |
| `accessibility/internal/service/atspi-app-registry.h` | D-Bus AppRegistry header |
| `accessibility/internal/service/atspi-app-registry.cpp` | Desktop/ActiveWindow 조회, NodeProxy factory |
| `accessibility/internal/service/atspi-event-router.h` | D-Bus 이벤트 라우터 header |
| `accessibility/internal/service/atspi-event-router.cpp` | Signal → AccessibilityEvent 매핑 (scaffold) |
| `accessibility/internal/service/window-tracker.h` | Window tracking header |
| `accessibility/internal/service/window-tracker.cpp` | GetFocusProc/GetVisibleWinInfo (scaffold) |

#### TIDL scaffold (3)

| File | Description |
|------|-------------|
| `accessibility/internal/service/tidl/tidl-node-proxy.h` | 모든 42 메서드가 default 반환 |
| `accessibility/internal/service/tidl/tidl-app-registry.h` | 빈 앱 리스트 반환 |
| `accessibility/internal/service/tidl/tidl-event-router.h` | No-op 이벤트 구독 |

#### Composite + Base + Stubs (5)

| File | Description |
|------|-------------|
| `accessibility/internal/service/composite-app-registry.h` | D-Bus + TIDL 통합 AppRegistry header |
| `accessibility/internal/service/composite-app-registry.cpp` | 두 registry 합치기 구현 |
| `accessibility/internal/service/accessibility-service-impl.cpp` | AccessibilityService PIMPL 구현 |
| `accessibility/internal/service/stub/stub-gesture-provider.h` | macOS no-op stub |
| `accessibility/internal/service/stub/stub-app-registry.h` | macOS stub (single mock window) |

#### Build (1)

| File | Description |
|------|-------------|
| `accessibility/internal/service/file.list` | CMake 소스 리스트 |

#### Test (4)

| File | Description |
|------|-------------|
| `test/mock/mock-node-proxy.h` | TestAccessible 기반 MockNodeProxy (DFS neighbor navigation 포함) |
| `test/mock/mock-app-registry.h` | Demo tree 생성 + MockNodeProxy factory |
| `test/mock/mock-gesture-provider.h` | 프로그래밍 방식 gesture 발화 |
| `test/test-service.cpp` | 55개 유닛 테스트 |

### 수정된 파일 (2)

| File | Change |
|------|--------|
| `accessibility/internal/bridge/file.list` | `INCLUDE( .../service/file.list )` 추가 |
| `build/tizen/CMakeLists.txt` | Service sources 추가, `BUILD_SERVICE_TESTS` 옵션 + `accessibility-service-test` 타겟 |

### 테스트 결과

```bash
cd ~/tizen/accessibility-common/build/tizen/build

# 기존 테스트 (regression 없음)
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-test
# === Results: 56 passed, 0 failed ===

# 서비스 테스트
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DBUILD_SERVICE_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-service-test
# === Results: 55 passed, 0 failed ===
```

### 테스트 상세 (55 tests)

| Category | Count | Tests |
|----------|-------|-------|
| MockNodeProxy | 18 | getName, getRole, getStates×3, getExtents, getChildCount×2, getChildAtIndex×2, getParent×2, getReadingMaterial×3, getNodeInfo×2 |
| Neighbor navigation | 10 | Forward 7단계 (Menu→Title→Play→Volume→NowPlaying→Prev→Next), 래핑, Backward 2단계 |
| Service lifecycle | 6 | getActiveWindow before/after start, start, stop, getCurrentNode after stop |
| Navigation | 6 | navigateNext first, named node, 7 nodes, 2+ elements, navigatePrev, getCurrentNode |
| Event routing | 7 | dispatch, type preserved, detail preserved, window event×2, multiple events, after stop |
| Gesture handling | 4 | dispatch, type preserved, point preserved, multiple gestures |
| Highlight | 2 | highlightNode, highlightNode(nullptr) |
| App registration | 3 | registered callback, address, deregistered callback |

## 핵심 설계 결정

1. **Proxy (not Cache)**: 각 NodeProxy 메서드는 IPC 호출. getReadingMaterial()이 24개 필드를 한 번에 가져옴
2. **Transport 추상화**: AccessibilityService는 NodeProxy/AppRegistry 인터페이스만 사용 → D-Bus/TIDL 무관
3. **MockNodeProxy의 DFS neighbor**: BridgeAccessible의 CalculateNeighbor와 동일한 로직을 HIGHLIGHTABLE 기준으로 구현
4. **CompositeAppRegistry**: D-Bus와 TIDL 앱을 하나의 리스트로 통합. Phase 2.6B 이후 TIDL 부분 활성화

## 다음 Phase

### Phase 4: Concrete Services
- `ScreenReaderService` — extends AccessibilityService, TTS integration
- `InspectorService` — extends AccessibilityService, web UI
- `AurumService` — extends AccessibilityService, automated testing

### Phase 2.6 Stage B 이후 작업
- `TidlNodeProxy` — generated proxy 코드로 42개 메서드 채움
- `TidlAppRegistry` — Tizen `aul` API로 앱 발견
- `TidlEventRouter` — TIDL delegate callbacks로 이벤트 수신
