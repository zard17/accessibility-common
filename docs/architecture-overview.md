# Tizen Accessibility Architecture Overview

Platform-agnostic accessibility framework extracted from DALi.

---

## 1. Vision

```mermaid
graph LR
    subgraph "Before"
        DA["DALi Adaptor<br/>(monolith)"] -->|"hard AT-SPI<br/>dependency"| ATSPI["AT-SPI / D-Bus"]
        ATSPI --> SR_C["Screen Reader<br/>(C, 68 atspi_* calls)"]
    end

    subgraph "After"
        AC["accessibility-common<br/>(standalone library)"] -->|"pluggable<br/>IPC"| B1["D-Bus (GDBus)"]
        AC -->|"pluggable<br/>IPC"| B2["TIDL"]
        AC -->|"pluggable<br/>IPC"| B3["In-Process"]
        SR_CPP["Screen Reader<br/>(C++, AccessibilityService)"] --> AC
        INS["Inspector"] --> AC
        AUR["Aurum"] --> AC
    end

    style DA fill:#f96
    style SR_C fill:#f96
    style AC fill:#6f6
    style SR_CPP fill:#6f6
```

**핵심 목표**: AT-SPI/D-Bus 하드코딩 제거, IPC pluggable, Screen reader를 C++ AccessibilityService로 재작성

---

## 2. Phase Overview

```mermaid
gantt
    title Accessibility Refactoring Phases
    dateFormat YYYY-MM
    axisFormat %Y-%m

    section Phase 1
    DALi에서 분리              :done, p1, 2025-06, 2025-12

    section Phase 2
    IPC 추상화 (양방향)          :done, p2, 2026-01, 2026-02
    eldbus→GDBus               :active, p25, 2026-02, 2026-04
    Tree Embedding 테스트        :p27, 2026-03, 2026-04
    TIDL Backend               :p26, 2026-04, 2026-06

    section Phase 3
    AccessibilityService Base   :p3, 2026-04, 2026-07

    section Phase 4
    Screen Reader (C++)         :p4a, 2026-07, 2026-10
    Inspector Rewrite           :p4b, 2026-10, 2026-12

    section Phase 5
    DALi Toolkit Integration    :p5, 2026-10, 2027-01
```

| Phase | Goal | Status |
|-------|------|--------|
| **1** | accessibility-common을 DALi에서 분리 | **DONE** |
| **2** | Bidirectional IPC 추상화 | **DONE** |
| **2.5** | eldbus → GDBus migration | **DONE** |
| **2.6** | TIDL IPC backend | **Stage A DONE** (scaffold + tidlc 코드 생성), Stage B/C는 Tizen 디바이스 필요 |
| **2.7** | Tree embedding 테스트 | **DONE** |
| **3** | AccessibilityService base class | TODO |
| **4** | Screen reader C++ rewrite | TODO |
| **5** | DALi toolkit integration | TODO |

---

## 3. End-to-End Architecture (Target State)

```mermaid
graph TB
    subgraph "UI Toolkit (DALi, Flutter, ...)"
        APP["Application"]
        CA["ControlAccessible<br/>(per-widget a11y data)"]
    end

    subgraph "accessibility-common"
        subgraph "Public API"
            ACC["Accessible<br/>(interface)"]
            BRIDGE["Bridge<br/>(interface)"]
            SVC["AccessibilityService<br/>(base class)"]
            NP["NodeProxy<br/>(interface)"]
        end

        subgraph "Bridge (app-side)"
            BI["BridgeImpl"]
            BB["BridgeBase"]
            BM["12 Bridge Modules"]
        end

        subgraph "IPC Abstraction"
            TF["TransportFactory"]
            IS["Server"]
            IC["5 Client Interfaces"]
            ID["InterfaceDescription"]
        end

        subgraph "Service (AT-side)"
            AR["AppRegistry"]
            GP["GestureProvider"]
            WT["WindowTracker"]
            ER["EventRouter"]
        end
    end

    subgraph "IPC Backends"
        DBUS["D-Bus (GDBus)"]
        TIDL["TIDL"]
        INPROC["In-Process"]
    end

    subgraph "Services (separate binaries)"
        SR["ScreenReaderService"]
        INS["InspectorService"]
        AUR["AurumService"]
    end

    subgraph "Platform"
        TTS["TTS Engine"]
        WM["Window Manager"]
        AMD["App Manager (amd)"]
    end

    APP --> CA
    CA --> ACC
    ACC --> BI
    BI --> BB
    BB --> BM
    BM --> IS
    BB --> TF

    TF --> DBUS
    TF --> TIDL
    TF --> INPROC
    IS --> ID

    SR --> SVC
    INS --> SVC
    AUR --> SVC
    SVC --> NP
    SVC --> AR
    SVC --> GP
    NP -->|"IPC"| BI

    AR --> AMD
    GP --> WM
    SR --> TTS

    style SVC fill:#bbf,stroke:#333
    style NP fill:#bbf,stroke:#333
    style TF fill:#6f6,stroke:#333
    style IS fill:#6f6,stroke:#333
    style IC fill:#6f6,stroke:#333
    style SR fill:#f9f,stroke:#333
```

---

## 4. Phase 1: DALi에서 분리 (DONE)

### Before

```mermaid
graph LR
    subgraph "dali-adaptor (monolith)"
        BA["Bridge code<br/>~15,000 lines"]
        DBW["DBusWrapper<br/>+ eldbus backend"]
        ACOM["accessibility-common.h"]
    end

    subgraph "dali-toolkit"
        CA["ControlAccessible"]
    end

    CA -->|"tight coupling"| BA
    BA --> DBW
```

### After

```mermaid
graph LR
    subgraph "accessibility-common (standalone)"
        BA["Bridge code"]
        DBW["DBusWrapper"]
        API["Public API<br/>(Accessible, Bridge, types)"]
        IPC["IPC abstraction"]
    end

    subgraph "dali-adaptor (thin shim)"
        SHIM["DaliAccessible<br/>(adapter)"]
    end

    subgraph "dali-toolkit"
        CA["ControlAccessible"]
    end

    CA --> SHIM
    SHIM -->|"links to"| API
    BA --> IPC
    IPC --> DBW
```

### Design Decisions

| Decision | Chosen | Rejected | Rationale |
|----------|--------|----------|-----------|
| Library boundary | Standalone `.so` | Header-only / git submodule | Independent versioning, CI, reuse by non-DALi toolkits |
| Accessible ownership | Raw `Accessible*` in bridge, `shared_ptr` for features | Shared ownership everywhere | Matches DALi's actor lifecycle; bridge doesn't own widgets |
| Platform callbacks | `PlatformCallbacks` function pointers | Virtual base class | Zero-overhead, no inheritance required, toolkit sets at init |
| Build system | CMake with conditional ATSPI/PKG | Meson / autotools | Matches DALi ecosystem |

---

## 5. Phase 2: Bidirectional IPC Abstraction (DONE)

### Problem

```mermaid
graph LR
    BI["BridgeImpl"] -->|"4x DBus::DBusClient"| DBUS["D-Bus API"]
    BB["BridgeBase"] -->|"DBus::DBusClient mRegistry"| DBUS
    BM["12 Bridge Modules"] -->|"DBus::DBusInterfaceDescription"| DBUS

    style DBUS fill:#f96
```

Bridge code에 D-Bus type이 직접 사용됨 → 다른 IPC backend 추가 불가

### Solution

```mermaid
graph TB
    subgraph "Bridge (IPC-agnostic)"
        BI["BridgeImpl"]
        BB["BridgeBase"]
        BM["12 Modules"]
    end

    subgraph "Abstraction Layer"
        TF["TransportFactory"]
        SRV["Server"]
        SM["StatusMonitor"]
        KF["KeyEventForwarder"]
        DRC["DirectReadingClient"]
        RC["RegistryClient"]
        SC["SocketClient"]
    end

    subgraph "D-Bus Backend"
        DTF["DbusTransportFactory"]
        DSM["DbusStatusMonitor"]
        DKF["DbusKeyEventForwarder"]
        DDRC["DbusDirectReadingClient"]
        DRC2["DbusRegistryClient"]
        DSC["DbusSocketClient"]
    end

    BI --> TF
    BI --> SM
    BI --> KF
    BI --> DRC
    BB --> RC
    BB --> SRV
    BM -->|"createInterfaceDescription()"| SRV

    TF -.-> DTF
    SM -.-> DSM
    KF -.-> DKF
    DRC -.-> DDRC
    RC -.-> DRC2
    SC -.-> DSC

    style TF fill:#6f6
    style SRV fill:#6f6
    style SM fill:#6f6
    style KF fill:#6f6
    style DRC fill:#6f6
    style RC fill:#6f6
    style SC fill:#6f6
```

### Design Decisions

| Decision | Chosen | Rejected | Rationale |
|----------|--------|----------|-----------|
| Client abstraction | 5 domain-specific interfaces | Generic `Ipc::Client` with type-erased methods | Type safety, clear contracts, easy to mock |
| Factory pattern | Abstract Factory (`TransportFactory`) | Service Locator / DI container | Single entry point, same connection shared |
| InterfaceDescription creation | `Server::createInterfaceDescription()` | Direct construction | Bridge modules don't `#include` D-Bus headers |
| Helper template dispatch | `static_cast` to concrete type inside helpers | Full type erasure in `InterfaceDescription` | Pragmatic — avoids complex template machinery |
| Factory injection | Constructor (`BridgeImpl()`) | External via `CreateBridge()` | `mTransportFactory` is `protected` |

### Pros / Cons

| Pros | Cons |
|------|------|
| New IPC backends without bridge code changes | 12 new files |
| D-Bus types confined to `dbus/` directory | `static_cast` in helpers ties to one backend at a time |
| Each component independently mockable | InterfaceDescription registration not fully type-erased |
| Zero runtime overhead (virtual dispatch << IPC latency) | No runtime backend switching |
| Incremental migration, tests green throughout | — |

---

## 6. Phase 2.5: eldbus → GDBus (TODO)

### Problem

`dbus-tizen.cpp` uses EFL's `eldbus` → EFL dependency on all platforms

### Solution

```mermaid
graph TB
    DBW["DBusWrapper<br/>(~50 virtual methods)"]

    DBW --> ELDBUS["DBusWrapper_eldbus<br/>(dbus-tizen.cpp)<br/>eldbus_* API"]
    DBW --> GDBUS["DBusWrapper_gdbus<br/>(dbus-gdbus.cpp, NEW)<br/>g_dbus_* API"]
    DBW --> STUB["DBusWrapper_stub<br/>(dbus-stub.cpp)<br/>no-op"]

    style GDBUS fill:#bbf
    style ELDBUS fill:#ddd
```

| Aspect | eldbus | GDBus |
|--------|--------|-------|
| Dependency | EFL (ecore, eldbus) | GLib (gio-2.0) |
| Platforms | Tizen only | Linux, macOS (Homebrew), Windows (MSYS2) |
| Message model | `Eldbus_Message_Iter` (sequential) | `GVariant` (tree-structured) |
| Thread safety | Single-threaded (ecore) | Thread-safe by default |
| Maintenance | Tizen-specific | Active GNOME project |

### Pros / Cons

| Pros | Cons |
|------|------|
| EFL dependency removed | ~2000 lines to reimplement |
| Works on macOS/Ubuntu/Android | GLib dependency added |
| `GVariant` is more type-safe | Different iteration model for containers |
| Active community maintenance | — |

---

## 7. Phase 2.6: TIDL Backend (Stage A DONE)

### Problem

D-Bus requires a daemon → overhead, no native Tizen security integration

### Solution

```mermaid
graph LR
    subgraph "D-Bus path"
        APP_D["App"] -->|"D-Bus"| DAEMON["dbus-daemon"] -->|"D-Bus"| AT_D["Screen Reader"]
    end

    subgraph "TIDL path"
        APP_T["App"] -->|"TIDL<br/>(direct IPC)"| AT_T["Screen Reader"]
    end

    style DAEMON fill:#f96
    style APP_T fill:#6f6
    style AT_T fill:#6f6
```

| Aspect | D-Bus | TIDL |
|--------|-------|------|
| Routing | Via dbus-daemon (central) | Direct P2P |
| Security | D-Bus policy files | Cynara (Tizen native) |
| Code generation | None (runtime registration) | `tidlc` codegen (compile-time) |
| Portability | Linux/macOS | Tizen only |
| Latency | Higher (daemon hop) | Lower (direct) |

### Implementation Status

#### Stage A: Scaffold (DONE, macOS)

- `tidl/` 디렉터리에 10개 파일 생성 (interface description, server, factory, 5 client stubs)
- `accessibility-service.tidl`: 모든 AT-SPI 메서드를 TIDL 인터페이스로 정의 (tidlc 검증 완료)
- `TidlInterfaceDescription`: `DBusInterfaceDescription`과 동일한 `addMethod<T>` / `addProperty<T>` / `addSignal<ARGS...>` 템플릿 API
- `bridge-base.h`: `#ifdef ENABLE_TIDL_BACKEND`로 concrete type cast 분기
- CMake `ENABLE_TIDL` 옵션 + `rpc-port` pkg-config 통합
- D-Bus 백엔드 56/56 테스트 통과 확인 (regression 없음)

#### Stage B/C: Tizen 디바이스 환경에서 수행해야 할 작업

macOS에서 `tidlc` 바이너리(v2.3.3)로 C++ stub/proxy 코드 생성은 가능하지만, 생성된 코드가 Tizen 플랫폼 API(`rpc_port_*`, `bundle_*`)를 ~2,000회 호출하므로 실제 IPC 테스트는 Tizen 런타임에서만 가능하다.

**macOS vs Tizen 비교:**

| | GDBus (Phase 2.5) | TIDL (Phase 2.6) |
|---|---|---|
| macOS 데몬 | `brew install dbus` → 동작 | rpc-port 데몬 없음 |
| 라이브러리 | `libgio-2.0` macOS 빌드 있음 | `librpc-port.so` Tizen 전용 |
| macOS IPC 테스트 | full round-trip 검증 완료 | 불가능 |
| 런타임 의존성 | dbus-daemon | cynara + aul + app framework 전체 |

**Tizen 디바이스 환경에서의 작업 목록:**

1. **tidlc 코드 생성 및 빌드 검증**
   ```bash
   tidlc -s -l C++ -i accessibility-service.tidl -o generated/accessibility-bridge-stub
   tidlc -p -l C++ -i accessibility-service.tidl -o generated/accessibility-bridge-proxy
   ```
   생성된 `ServiceBase` 클래스는 모든 TIDL 메서드에 대해 pure virtual 메서드를 제공.

2. **TidlIpcServer dispatch 구현**: 생성된 `ServiceBase`를 상속하여 `TidlInterfaceDescription`에 저장된 핸들러로 dispatch. `ServiceBase::GetName(objectPath)` → `mMethods["GetName"]` callback 호출.

3. **5개 Client wrapper 구현**: 생성된 proxy 클래스를 감싸서 `Ipc::StatusMonitor`, `Ipc::KeyEventForwarder` 등의 인터페이스 구현. Delegate callback → `listenXxx()` callback 연결.

4. **통합 테스트**: `ENABLE_TIDL=ON`으로 빌드 후 Tizen 스크린 리더와 연동 테스트.

### Pros / Cons

| Pros | Cons |
|------|------|
| No daemon → faster boot, less memory | Tizen only |
| Compile-time type checking (codegen) | Build complexity (tidlc) |
| Native Tizen security (Cynara) | Requires full-stack migration (both app and AT) |
| — | No AT-SPI compatibility with existing tools |
| — | macOS에서 end-to-end 테스트 불가 (GDBus와 달리) |

---

## 8. Phase 3: AccessibilityService Base Class (TODO)

### Problem

Screen reader (C), Inspector, Aurum each implement their own AT-SPI communication from scratch.

### Solution: Android-inspired AccessibilityService pattern

```mermaid
graph TB
    subgraph "AccessibilityService (base class)"
        SVC["AccessibilityService"]
        SVC -->|"owns"| WT["WindowTracker"]
        SVC -->|"uses"| NP["NodeProxy"]
        SVC -->|"receives"| AR["AppRegistry"]
        SVC -->|"receives"| GP["GestureProvider"]

        SVC -->|"navigateNext()"| NP
        SVC -->|"highlightNode()"| NP
        NP -->|"IPC proxy"| APP["App Bridge"]
    end

    subgraph "Concrete Services"
        SR["ScreenReaderService<br/>extends AccessibilityService"]
        INS["InspectorService<br/>extends AccessibilityService"]
        AUR["AurumService<br/>extends AccessibilityService"]
    end

    SR --> SVC
    INS --> SVC
    AUR --> SVC

    SR -->|"onGesture()"| DISPATCH["Gesture → Navigate → TTS"]
    SR -->|"onAccessibilityEvent()"| HANDLE["Event → TTS/Sound"]

    style SVC fill:#bbf
    style NP fill:#bbf
```

### Key Interfaces

```mermaid
classDiagram
    class AccessibilityService {
        +start()
        +stop()
        +getActiveWindow() NodeProxy
        +navigateNext() NodeProxy
        +navigatePrev() NodeProxy
        +highlightNode(node) bool
        #onAccessibilityEvent(event)*
        #onWindowChanged(window)*
        #onGesture(gesture)*
        #onKeyEvent(key) bool
    }

    class NodeProxy {
        <<interface>>
        +getName() string
        +getRole() Role
        +getStates() States
        +getNeighbor(root, direction) NodeProxy
        +getReadingMaterial() ReadingMaterial
        +getExtents(coordType) Rect
        +grabHighlight() bool
        +doActionByName(name) bool
        ...40+ methods
    }

    class AppRegistry {
        <<interface>>
        +getDesktop() NodeProxy
        +getActiveWindow() NodeProxy
        +onAppRegistered(callback)
        +onAppDeregistered(callback)
    }

    class GestureProvider {
        <<interface>>
        +onGestureReceived(callback)
    }

    class TtsEngine {
        <<interface>>
        +speak(text, interrupt) int
        +stop()
        +pause()
        +resume()
    }

    AccessibilityService --> NodeProxy
    AccessibilityService --> AppRegistry
    AccessibilityService --> GestureProvider

    class ScreenReaderService {
        -mTts: TtsEngine
        -mComposer: ReadingComposer
        +onGesture(gesture)
        +onAccessibilityEvent(event)
    }

    ScreenReaderService --|> AccessibilityService
    ScreenReaderService --> TtsEngine
```

### Architecture: Proxy vs Cache

```mermaid
graph LR
    subgraph "Strategy A: Proxy (chosen)"
        SR_A["ScreenReader"] -->|"getNeighbor()"| NP_A["NodeProxy"]
        NP_A -->|"IPC call"| APP_A["App Bridge"]
        APP_A -->|"result"| NP_A
    end

    subgraph "Strategy B: Tree Cache (rejected)"
        SR_B["ScreenReader"] -->|"getNeighbor()"| CACHE["Local Tree Cache"]
        CACHE -->|"periodic sync"| APP_B["App Bridge"]
    end

    style NP_A fill:#6f6
    style CACHE fill:#f96
```

| Aspect | Proxy (A) | Cache (B) |
|--------|-----------|-----------|
| Complexity | Low | High (sync, invalidation) |
| Freshness | Always current | May be stale |
| Latency | 1 IPC per call | Near-zero after sync |
| Memory | O(1) per node | O(n) entire tree |
| Chosen? | **Yes** | No (add later if needed) |

**완화책**: `getReadingMaterial()` batch call이 name+role+states+value+parent를 한번에 fetch → round-trip 최소화

Tree embedding (WebView 등) 시나리오에서의 Proxy vs Cache 상세 비교 분석은 [tree-embedding-analysis.md](tree-embedding-analysis.md) 참조. 결론: Strategy A의 Socket/Plug protocol 복잡도는 설정 시점에 한정되지만, Strategy B의 cache sync 비용은 runtime 내내 발생하므로 Proxy가 기본값으로 적합.

### Design Decisions

| Decision | Chosen | Rejected | Rationale |
|----------|--------|----------|-----------|
| Navigation strategy | Proxy (Strategy A) | Local tree cache (B) | Simpler, always fresh, optimize later if needed |
| NodeProxy granularity | Single interface (~40 methods) | Split per AT-SPI interface | Screen reader needs all of them; splitting adds complexity |
| Base class vs composition | Base class with virtual hooks | Strategy/Observer pattern | Android pattern proven at scale; subclass = one service |
| Window tracking | Built into base class | External plugin | Every service needs it; common logic |
| IPC direction | AT queries App (pull) | App pushes to AT (push) | Matches AT-SPI model; AT controls navigation |

### Pros / Cons

| Pros | Cons |
|------|------|
| Screen reader/Inspector/Aurum share base class | NodeProxy methods are all IPC → latency |
| IPC backend swap transparent to service code | WindowTracker must be reimplemented from C |
| New AT = one subclass | AT-SPI protocol compatibility must be exact |
| Testable with mocks (MockNodeProxy, etc.) | Event loop integration varies by platform |

---

## 9. Phase 4: Service Implementations (TODO)

### 4a: Screen Reader (REQUIRED)

```mermaid
graph TB
    subgraph "ScreenReaderService"
        SR["ScreenReaderService<br/>extends AccessibilityService"]

        SR --> OG["onGesture()"]
        SR --> OE["onAccessibilityEvent()"]
        SR --> OK["onKeyEvent()"]

        OG --> NAV["Navigate<br/>(base class)"]
        OG --> ACT["Activate<br/>(doActionByName)"]
        OE --> RC["ReadingComposer<br/>(ReadingMaterial → text)"]
        OE --> SN["SmartNotification<br/>(scroll/chain sounds)"]
        RC --> TTS["TtsEngine<br/>(platform TTS)"]
    end

    subgraph "C → C++ Mapping"
        direction LR
        C1["navigator.c"] -.-> OG
        C2["app_tracker.c"] -.-> OE
        C3["reading_composer.c"] -.-> RC
        C4["smart_notification.c"] -.-> SN
        C5["screen_reader_tts.c"] -.-> TTS
        C6["flat_navi.c"] -.-> NAV
        C7["window_tracker.c"] -.-> WT["WindowTracker<br/>(in base class)"]
    end
```

| C Source | Lines | C++ Replacement | Pure Logic? |
|----------|-------|-----------------|-------------|
| `navigator.c` | ~800 | `ScreenReaderService::onGesture()` | No (IPC) |
| `flat_navi.c` | ~400 | `AccessibilityService::navigateNext/Prev()` | No (IPC) |
| `reading_composer.c` | ~600 | `ReadingComposer` class | **Yes** |
| `smart_notification.c` | ~200 | `SmartNotification` class | **Yes** |
| `screen_reader_tts.c` | ~300 | `TtsEngine` (platform impl) | No (platform) |
| `window_tracker.c` | ~500 | `WindowTracker` (base class) | Partially |
| `keyboard_tracker.c` | ~200 | `onKeyEvent()` hook | No (platform) |
| `app_tracker.c` | ~600 | `onAccessibilityEvent()` | No (IPC) |

구현 순서: ReadingComposer → SmartNotification → TtsEngine → GestureProvider → ScreenReaderService → main

### 4b: Inspector (OPTIONAL)

기존 `DirectQueryEngine` + `WebInspectorServer` 위에 재구축. 현재 동작하므로 우선순위 낮음.

### 4c: Aurum (OPTIONAL)

기존 aurum이 AT-SPI 직접 사용. AccessibilityService API로 전환하면 screen reader와 동일 경로 → 테스트 일관성 향상.

### 4d: macOS NSAccessibility Backend (OPTIONAL)

Native macOS accessibility backend using `NSAccessibilityElement`. VoiceOver와 직접 통합.

| Component | Description |
|-----------|-------------|
| `DaliAccessibleNode` | `NSAccessibilityElement` subclass wrapping `Accessible` objects |
| VoiceOver 통합 | `accessibilityChildren`, `accessibilityHitTest`, `accessibilityFocusedUIElement` |
| Event notification | `NSAccessibilityPostNotification` for state/focus/value changes |
| Rendering surface | macOS `NSWindow` + OpenGL/Metal view 필요 |

D-Bus/AT-SPI가 아닌 macOS native accessibility protocol을 사용하므로, IPC 추상화 레이어와는 별도의 backend path.

---

## Phase 5: Toolkit Integration (Planned)

accessibility-common을 DALi의 dependency로 다시 통합. 현재 dali-adaptor 내의 accessibility 코드를 대체.

| Goal | Description |
|------|-------------|
| dali-adaptor dependency | `dali-adaptor`가 accessibility-common에 의존 (코드 포함 대신) |
| ControlAccessible | `dali-toolkit`의 `ControlAccessible`이 `Accessible` interface 구현 |
| Platform callbacks | DALi adaptor lifecycle에서 `PlatformCallbacks` 연결 |
| Zero behavior change | 기존 accessibility consumer 동작 변경 없음 |

---

## 10. Full Stack Data Flow

App → Bridge → IPC → Screen Reader 간의 signal, method call, navigation sequence diagram은 [data-flow.md](data-flow.md) 참조.

핵심 흐름:
- **Event**: App `SetName()` → Bridge `emitSignal()` → Screen Reader `onAccessibilityEvent()` → `getReadingMaterial()` → TTS
- **Navigation**: Gesture → `navigateNext()` → `getNeighbor()` IPC → `grabHighlight()` → TTS
- **Init**: `SetPlatformCallbacks()` → `GetCurrentBridge()` → `Initialize()` → `ApplicationResumed()` → `ForceUp()`

---

## 11. Repository Structure (Target)

```
accessibility-common/
├── accessibility/
│   ├── api/                          # Public API
│   │   ├── accessible.h              # Accessible interface
│   │   ├── accessibility-bridge.h    # Bridge interface
│   │   ├── component.h               # Component (extents, layer)
│   │   ├── types.h                   # Role, State, enums
│   │   ├── accessibility-service.h   # ← Phase 3: base class
│   │   ├── node-proxy.h             # ← Phase 3: AT-side node interface
│   │   ├── app-registry.h           # ← Phase 3: app discovery
│   │   └── ...
│   │
│   ├── internal/
│   │   ├── bridge/                   # App-side bridge
│   │   │   ├── ipc/                  # IPC abstraction (Phase 2)
│   │   │   │   ├── ipc-server.h
│   │   │   │   ├── ipc-transport-factory.h
│   │   │   │   ├── ipc-status-monitor.h
│   │   │   │   ├── ipc-key-event-forwarder.h
│   │   │   │   ├── ipc-direct-reading-client.h
│   │   │   │   ├── ipc-registry-client.h
│   │   │   │   └── ipc-socket-client.h
│   │   │   │
│   │   │   ├── dbus/                 # D-Bus backend
│   │   │   │   ├── dbus.h            # Core DBus abstraction
│   │   │   │   ├── dbus-tizen.cpp    # eldbus (current)
│   │   │   │   ├── dbus-gdbus.cpp   # ← Phase 2.5: GDBus
│   │   │   │   ├── dbus-stub.cpp     # stub (macOS/CI)
│   │   │   │   ├── dbus-transport-factory.h
│   │   │   │   └── dbus-*-client.h   # 5 D-Bus client impls
│   │   │   │
│   │   │   ├── tidl/                # ← Phase 2.6: TIDL backend
│   │   │   │   ├── tidl-transport-factory.h
│   │   │   │   └── tidl-*-client.h
│   │   │   │
│   │   │   ├── bridge-base.h/.cpp
│   │   │   ├── bridge-impl.cpp
│   │   │   └── bridge-*.cpp          # 12 bridge modules
│   │   │
│   │   └── service/                  # ← Phase 3: AT-side service
│   │       ├── atspi-node-proxy.cpp
│   │       ├── atspi-app-registry.cpp
│   │       ├── atspi-event-router.cpp
│   │       └── window-tracker.cpp
│   │
│   └── service/                      # ← Phase 4: service implementations
│       └── screen-reader/
│           ├── screen-reader-service.h/.cpp
│           ├── reading-composer.h/.cpp
│           └── smart-notification.h/.cpp
│
├── tools/
│   ├── inspector/                    # CLI/Web inspector (exists)
│   └── screen-reader/               # ← Phase 4: screen reader binary
│       └── main.cpp
│
├── test/                             # Tests (31 existing + growing)
├── build/tizen/                      # CMake build
└── docs/
    ├── architecture-overview.md      # This document (concise)
    ├── phase2-ipc-abstraction.md     # Phase 2 detailed design
    ├── tree-embedding-analysis.md    # Proxy vs Cache for embedding
    ├── data-flow.md                  # Sequence diagrams
    ├── inspector-architecture.md     # CLI/Web inspector details
    └── mock-and-test.md              # Mock, build system, test infra
```

---

## 12. Key Abstractions Across Phases

```mermaid
graph TB
    subgraph "Phase 1: Separation"
        A1["Accessible (interface)"]
        A2["Bridge (interface)"]
        A3["PlatformCallbacks"]
    end

    subgraph "Phase 2: IPC Abstraction"
        B1["TransportFactory"]
        B2["Server + InterfaceDescription"]
        B3["5 Client Interfaces"]
    end

    subgraph "Phase 2.5/2.6: Backends"
        C1["GDBus Backend"]
        C2["TIDL Backend"]
    end

    subgraph "Phase 3: Service Framework"
        D1["AccessibilityService"]
        D2["NodeProxy"]
        D3["AppRegistry + GestureProvider"]
    end

    subgraph "Phase 4: Implementations"
        E1["ScreenReaderService"]
        E2["ReadingComposer"]
        E3["TtsEngine"]
    end

    A1 --> B2
    A2 --> B1
    B1 --> C1
    B1 --> C2
    B3 --> D2
    D1 --> E1
    D2 --> E1
    D3 --> E1
    E1 --> E2
    E1 --> E3

    style A1 fill:#ddd
    style A2 fill:#ddd
    style A3 fill:#ddd
    style B1 fill:#6f6
    style B2 fill:#6f6
    style B3 fill:#6f6
    style C1 fill:#bbf
    style C2 fill:#bbf
    style D1 fill:#fbf
    style D2 fill:#fbf
    style D3 fill:#fbf
    style E1 fill:#ff9
    style E2 fill:#ff9
    style E3 fill:#ff9
```

---

## 13. Decision Log (Cross-Phase)

| # | Phase | Decision | Chosen | Rationale |
|---|-------|----------|--------|-----------|
| 1 | 1 | Library structure | Standalone `.so` | Independent of DALi lifecycle |
| 2 | 1 | Accessible ownership | Raw pointers + shared_ptr features | Matches toolkit actor lifecycle |
| 3 | 1 | Platform decoupling | `PlatformCallbacks` function pointers | No base class dependency on toolkit |
| 4 | 2 | Client abstraction style | 5 domain-specific interfaces | Type safety, mockability |
| 5 | 2 | Backend creation | Abstract Factory (`TransportFactory`) | Single entry point for backend |
| 6 | 2 | Interface registration | `static_cast` in helpers | Pragmatic; avoids type erasure |
| 7 | 2.5 | D-Bus library | GDBus (GLib) over eldbus (EFL) | Broader platform support |
| 8 | 2.6 | Alternative IPC | TIDL | D-Bus daemon 제거, Tizen native IPC |
| 9 | 3 | Navigation model | Proxy (not tree cache) | Simpler, always fresh |
| 10 | 3 | Service pattern | Android AccessibilityService | Proven at scale |
| 11 | 3 | NodeProxy scope | Single interface (~40 methods) | All methods needed by screen reader |
| 12 | 3 | Registry daemon | Not needed (use existing amd) | Avoid new daemon |
| 13 | 4 | Screen reader language | C++ (not C) | Type safety, share base class |
| 14 | 4 | TTS composition | Separate `ReadingComposer` class | Pure logic, testable |

---

## 14. Risk Matrix

| Risk | Phase | Impact | Likelihood | Mitigation |
|------|-------|--------|------------|------------|
| GDBus `GVariant` mismatch with nested containers | 2.5 | High | Medium | Extensive unit tests per type |
| TIDL lacks D-Bus introspection / property signals | 2.6 | Medium | High | Implement equivalent in TIDL interface |
| NodeProxy IPC latency hurts screen reader performance | 3 | High | Low | `getReadingMaterial()` batch call |
| AtSpiNodeProxy doesn't match bridge protocol exactly | 3 | High | Medium | Test against real AT-SPI apps |
| Event loop integration differs per platform | 3-4 | Medium | Medium | `PlatformCallbacks` extension |
| C → C++ screen reader logic port introduces bugs | 4 | Medium | Medium | Port pure logic first (ReadingComposer) |

---

## 15. Verification Strategy

| Phase | Test Method | Expected |
|-------|------------|----------|
| 1 | `accessibility-test` | 31 passed |
| 2 | `accessibility-test` (unchanged) | 31 passed |
| 2.5 | + GDBus integration test on session bus | 31 + N passed |
| 2.7 | + Tree embedding unit tests | 31 + 10 passed |
| 3 | + AccessibilityService unit tests (mock providers) | 31 + N passed |
| 4a | Screen reader binary vs AT-SPI apps | End-to-end TTS |
| 5 | Full stack rebuild + existing AT-SPI consumers | Zero behavior change |
| Full stack | accessibility-common → dali-adaptor → dali-toolkit → dali-demo | GUI app with a11y |

Test coverage 상세 및 build 방법은 [mock-and-test.md](mock-and-test.md) 참조.

---

## Appendix (detailed docs)

| Topic | File |
|-------|------|
| Mock architecture, build system, test infrastructure | [mock-and-test.md](mock-and-test.md) |
| Inspector architecture (CLI, Web, REST API) | [inspector-architecture.md](inspector-architecture.md) |
| Phase 2 IPC abstraction detailed design | [phase2-ipc-abstraction.md](phase2-ipc-abstraction.md) |
| Tree embedding: Proxy vs Cache deep analysis | [tree-embedding-analysis.md](tree-embedding-analysis.md) |
| Full stack data flow (sequence diagrams) | [data-flow.md](data-flow.md) |
