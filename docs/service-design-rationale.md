# AccessibilityService 설계 근거: AT-SPI 대체 분석

> Parent document: [architecture-overview.md](architecture-overview.md)

---

## 1. AccessibilityService가 대체하는 것

기존 Tizen screen reader는 AT-SPI 인프라에 전적으로 의존:

```
[Before]
DALi App
  └─ ControlAccessible
       └─ BridgeImpl (dali-adaptor, D-Bus hardcoded)
            └─ D-Bus (eldbus)
                 └─ at-spi2-registryd (system daemon)
                      └─ Screen Reader (C binary, 68 atspi_* calls)
                           └─ libatspi (client library)

[After]
DALi App
  └─ ControlAccessible
       └─ BridgeImpl (accessibility-common, IPC abstracted)
            └─ IPC (D-Bus / TIDL / In-Process)
                 └─ AccessibilityService (C++ base class)
                      ├─ AppRegistry      ← registryd 역할
                      ├─ NodeProxy        ← libatspi 역할
                      └─ GestureProvider  ← WM gesture dispatch 역할
```

### 구성요소별 매핑

| AT-SPI 구성요소 | 역할 | accessibility-common 대체 |
|---|---|---|
| `at-spi2-registryd` | app 등록, event routing, desktop node | `AppRegistry` interface (`AtSpiAppRegistry`, `DirectAppRegistry`) |
| `libatspi` | 68개 `atspi_*` C API calls | `NodeProxy` (42 typed C++ methods) |
| AT-SPI D-Bus signals | event dispatch to screen reader | `AccessibilityService::dispatchEvent()` → `onAccessibilityEvent()` |
| WM gesture forwarding | gesture → screen reader | `GestureProvider` interface |
| `at-spi2-core` (library) | D-Bus serialization for AT-SPI | 자체 bridge serialization (accessibility-common) |
| `at-spi2-atk` | ATK→AT-SPI bridge | 해당 없음 (DALi는 ATK 미사용) |

핵심: AT-SPI는 **프로토콜 + 데몬 + 클라이언트 라이브러리**가 D-Bus에 묶여있었음. `AccessibilityService`는 이 세 역할을 abstract interface로 분리하여 IPC backend를 교체 가능하게 만듦.

---

## 2. Pros

### IPC pluggable

기존: `libatspi` → D-Bus only. 변경 불가.

현재: `NodeProxy` 구현만 교체하면 D-Bus, TIDL, In-Process 모두 동작. 서비스 코드 변경 없음.

### Daemon 의존성 제거 (In-Process 모드)

기존: `at-spi2-registryd` 데몬이 반드시 실행 중이어야 함.

현재: `DirectNodeProxy` + `DirectAppRegistry`로 데몬 없이 동작 가능.

### Type safety

기존: `atspi_accessible_get_name()` → `char*` 반환, caller가 `g_free()`. 런타임까지 타입 오류 모름.

현재: `NodeProxy::getName()` → `std::string`, `getRole()` → `Role` enum. 컴파일 타임 검증.

### Batch optimization

기존: `atspi_accessible_get_name()`, `_get_role()`, `_get_states()` — 각각 별도 D-Bus round-trip.

현재: `getReadingMaterial()` 한 번에 24개 필드 fetch.

### Unit testable

기존: `libatspi` mock 불가. 테스트하려면 실제 D-Bus daemon + app 필요.

현재: `MockNodeProxy`, `MockAppRegistry`로 120개 screen reader 테스트가 IPC 없이 실행.

### Profile 분리 (Mobile/TV)

기존: 하나의 binary에서 조건문 분기.

현재: `ScreenReaderService`(mobile) / `TvScreenReaderService`(TV)가 같은 base class를 상속, event handling만 다름.

### Cross-platform

기존: Linux D-Bus 전용 (`eldbus` → EFL 의존).

현재: macOS에서 demo 동작. CI에서 stub backend로 테스트 가능.

### at-spi2-core fork 유지 비용 제거

기존: at-spi2-core를 fork하여 custom interface 추가. Upstream 변경 cherry-pick, 충돌 관리 부담.

현재: 자체 소유 코드에 자유롭게 확장. Upstream 추적 불필요.

---

## 3. Cons

### 비표준

AT-SPI는 freedesktop.org 표준. `AccessibilityService`/`NodeProxy`는 Tizen 커스텀. 다른 Linux desktop screen reader (Orca 등)의 채택 가능성 낮음. 단, Tizen은 이미 AT-SPI를 자체 확장하여 사용 중이었으므로 실질적 차이는 크지 않음.

### Migration cost

기존 C screen reader (약 3,600 lines, 8개 파일)를 C++ `AccessibilityService` subclass로 포팅 필요. 핵심 로직은 `ScreenReaderService`로 재구현 완료, 그러나 기존 C 코드의 edge case가 포팅 과정에서 누락될 위험.

### ATK 앱 공존 시 registryd 필요

Tizen 웹앱/WebView가 ATK를 통해 accessibility 구현. ATK 앱이 등록할 `at-spi2-registryd`가 필요. 모든 앱이 accessibility-common으로 마이그레이션될 때까지 registryd 유지 필수. (상세: 아래 "ATK 앱 공존" 섹션)

---

## 4. ATK 앱 공존

### 현황

Tizen의 웹앱/WebView는 ATK(Accessibility Toolkit)를 통해 accessibility를 구현. ATK 앱은 `at-spi2-atk` bridge를 통해 AT-SPI D-Bus interface를 노출. 당장 마이그레이션이 어려우므로 공존 필요.

### 공존 아키텍처

```
┌─────────────────────────────────────────────────┐
│                    D-Bus bus                     │
│                                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐ │
│  │ DALi App   │  │ Web App    │  │ Web App    │ │
│  │ (acc-common│  │ (ATK +     │  │ (ATK +     │ │
│  │  bridge)   │  │ at-spi2-atk│  │ at-spi2-atk│ │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘ │
│        │               │               │        │
│        ▼               ▼               ▼        │
│   org.a11y.atspi.* (동일 D-Bus protocol)        │
│                                                  │
│           at-spi2-registryd (app discovery)      │
│                      │                           │
│                      ▼                           │
│           AtSpiAppRegistry                       │
│           AtSpiNodeProxy ◄── ScreenReaderService │
└──────────────────────────────────────────────────┘
```

- accessibility-common bridge와 ATK bridge 모두 **같은 AT-SPI D-Bus protocol** 노출
- `AtSpiNodeProxy`는 상대가 어떤 bridge든 동일하게 query 가능
- `AtSpiAppRegistry`는 registryd에서 모든 앱(DALi + ATK) 발견

### 주의 사항

| 항목 | 설명 |
|------|------|
| Protocol 정합성 | `GetReadingMaterial`은 accessibility-common 전용 확장. ATK bridge에는 없으므로 ATK 앱에는 기본 AT-SPI methods (`GetName`, `GetRole`, `GetState`) fallback 필요 |
| registryd 유지 | ATK 앱이 등록할 곳 필요. 웹앱 ATK 의존하는 한 registryd 제거 불가 |
| 테스트 | `AtSpiNodeProxy` → ATK 앱 간 실제 D-Bus 통신 검증 필요 |

### registryd 제거 시점

모든 앱이 accessibility-common 또는 TIDL로 마이그레이션된 후. 웹앱의 ATK 마이그레이션은 별도 프로젝트.

---

## 5. Multi-AT 동시 사용

### D-Bus 경로: 문제 없음

D-Bus는 multicast bus. 여러 AT가 같은 bus에 연결하여 같은 app에 독립적으로 query/subscribe 가능. `registryd`는 app 발견 역할이며, IPC 중개가 아님. 각 AT가 자기 `AppRegistry` 인스턴스를 가지므로 충돌 없음.

### TIDL 경로: 미해결 과제

TIDL은 P2P direct IPC. 중앙 bus 없음.

| 문제 | 설명 |
|------|------|
| App 발견 | registryd 없이 AT가 실행 중인 app을 어떻게 아는가? |
| 다중 연결 | 하나의 app TIDL server에 여러 AT client 동시 연결 필요 |
| Event fanout | app signal을 연결된 모든 AT에 broadcast 필요 |

가능한 접근:
- `amd` (Tizen app manager)를 app discovery에 활용
- TIDL server의 multi-client accept 지원 (`rpc_port` + `ServiceBase::OnConnected`)
- D-Bus+TIDL 공존: app discovery는 D-Bus(registryd), data query만 TIDL — `CompositeAppRegistry`가 이 패턴용

Phase 2.6 Stage B/C (Tizen 디바이스)에서 해결 필요.

---

## 6. TV In-Process vs IPC: 미결정

TV screen reader의 배포 형태 — 앱 내장(in-process) vs 별도 프로세스(IPC) — 는 성능 비교 후 결정.

### 비교

| | In-Process | IPC (D-Bus/TIDL) |
|---|---|---|
| Latency | ~0 (함수 호출) | 수 ms (D-Bus round-trip) |
| 배포 | 앱에 내장, daemon 관리 불필요 | 별도 screen reader daemon |
| Crash isolation | 앱과 함께 crash | screen reader 독립 생존 |
| Multi-app | 현재 앱만 접근 가능 | 모든 앱 접근 가능 |
| 구현 | `DirectNodeProxy` + `DirectAppRegistry` | `AtSpiNodeProxy` + `AtSpiAppRegistry` |

### TV 특성

- Focus는 한 번에 하나의 control에만 → screen reader가 현재 앱만 읽으면 됨
- 리모컨 방향키 → focus 이동 → TTS 발화: latency가 직접 체감됨
- Crash 시 앱 재시작으로 복구 가능하지만, 앱 안정성에 의존하게 되는 risk 있음

### 결정 기준

1. **성능 측정**: 실제 TV 환경에서 IPC (D-Bus 또는 TIDL) round-trip latency 측정
2. **Crash 빈도**: screen reader 코드의 crash 가능성 평가 (순수 로직이므로 낮을 것으로 예상)
3. **앱 구조**: TV 앱이 screen reader를 내장할 수 있는 구조인지 (library linking 가능 여부)

In-Process가 유리한 경우: IPC latency가 TTS 응답성에 영향을 줄 정도이고, crash risk가 낮을 때.
IPC가 유리한 경우: crash isolation이 중요하거나, 여러 앱을 오가는 시나리오가 있을 때.

**현재 상태**: Demo 단계에서는 In-Process (`TvScreenReaderService` + `DirectNodeProxy`)로 구현. Production 결정은 Tizen TV 환경에서의 성능 비교 후.
