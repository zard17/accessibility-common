# Bridge Decoupling Requirements

## Problem Statement

현재 Bridge의 `IsUp()` (accessibility enabled 여부)이 D-Bus 연결에 의존하고 있다.

```
IsUp() 판정 조건 (현재):
1. Bridge::IsEnabled() → mIsEnabled (D-Bus AT-SPI 상태 서비스에서 읽어옴)
2. Bridge::IsUp() → bool(mData) (ForceUp()에서 설정)
```

**ForceUp() 호출 경로:**
```
Initialize()
  → InitializeAccessibilityStatusClient()      // DBusWrapper 없으면 실패
    → ReadIsEnabledProperty()                   // D-Bus property 읽기
      → mIsEnabled = true
        → SwitchBridge()
          → ForceUp()                           // mData 설정
```

**ForceUp() 내부 (BridgeBase):**
```
BridgeBase::ForceUp()
  → Bridge::ForceUp()       // mData 설정 (OK)
  → DBusWrapper::Installed() 확인   // nullptr이면 FAILED 반환
  → D-Bus 연결, IPC 서버 생성       // D-Bus 전용
```

D-Bus가 없는 플랫폼 (macOS, Windows 등)에서는 전체 체인이 실패하여
`IsUp()` = false → `GrabHighlight()` 거부 → 하이라이트 안 그려짐.

---

## 핵심 원칙

1. **D-Bus는 hard requirement가 아니다.** D-Bus, TIDL, 또는 다른 IPC가 없어도 accessibility는 동작해야 한다.
2. **"Bridge up" = "accessibility enabled"** — IPC transport 연결 여부와 무관하다.
3. **하이라이트 렌더링은 DALi 앱의 책임이다.** 스크린리더나 a11y 서비스가 그리는 게 아니다.
4. **A11y 서비스는 DALi에 grab highlight 이벤트를 보내는 역할이다.** DALi가 받아서 렌더링한다.

---

## 요구사항

### R1. Bridge lifecycle을 IPC transport에서 분리

**R1.1** `ForceUp()`은 IPC transport 없이도 성공해야 한다.
- `mData`가 설정되고, `IsUp()` = true 반환
- IPC가 있으면 IPC 서버 생성 + 인터페이스 등록 (추가 작업)
- IPC가 없으면 로컬 accessibility만 활성화 (하이라이트, 네비게이션 등)

**R1.2** `Initialize()`는 IPC transport가 없을 때 즉시 accessibility를 활성화해야 한다.
- D-Bus 상태 서비스 접속 실패 시, D-Bus가 아예 없는 건지 vs. 일시적 실패인지 구분
- D-Bus wrapper가 설치 안 된 경우: `ForceUp()` 직접 호출
- D-Bus wrapper가 설치됐지만 접속 실패: 기존 retry 로직 유지

**R1.3** `IsEnabled()`는 IPC 상태와 독립적으로 설정 가능해야 한다.
- 현재: D-Bus property에서만 `mIsEnabled` 설정
- 필요: IPC가 없으면 기본값 `true`로 설정 (로컬에서 accessibility 사용 가능)

### R2. Highlight는 IPC 없이 동작

**R2.1** `GrabHighlight()` / `ClearHighlight()`는 `IsUp()` = true이면 동작해야 한다.
- 현재 코드 변경 불필요 — R1이 해결되면 자동으로 동작
- `ControlAccessible::GrabHighlight()` (dali-toolkit)
- `WindowAccessible::GrabHighlight()` (dali-adaptor의 actor-accessible.cpp)

**R2.2** 하이라이트 상태는 `Bridge::Data`에 저장된다.
- `mData->mCurrentlyHighlightedAccessible` — 현재도 이 구조
- IPC transport와 무관하게 동작 (mData만 있으면 됨)

### R3. IPC 연결은 선택적 확장

**R3.1** IPC가 있을 때만 추가되는 기능:
- `RegisterInterfaces()` — AT-SPI 인터페이스 등록 (D-Bus method/property binding)
- `mIpcServer` 생성 — IPC 서버
- `mRegistryClient` / `mDirectReadingClient` — 외부 서비스 연결
- `RequestBusName()` / `EmbedSocket()` — AT-SPI 등록

**R3.2** IPC가 없을 때도 동작하는 기능:
- 하이라이트 (GrabHighlight / ClearHighlight)
- 네비게이션 (GetNeighbor)
- Accessible 트리 관리 (AddAccessible / RemoveAccessible / GetChildren)
- 상태/이벤트 (Emit* 함수들) — IPC가 없으면 로컬 이벤트만

### R4. 플랫폼별 동작

| 플랫폼 | D-Bus | IsUp() | Highlight | IPC 이벤트 |
|--------|-------|--------|-----------|-----------|
| Tizen  | 있음  | AT-SPI 서비스 상태에 따름 | 동작 | D-Bus로 전송 |
| macOS  | 없음  | 항상 true (로컬) | 동작 | 로컬만 (향후 NSAccessibility 가능) |
| Windows| 없음  | 항상 true (로컬) | 동작 | 로컬만 (향후 UIA 가능) |

---

## 변경 범위

### accessibility-common

| 파일 | 변경 내용 |
|------|----------|
| `bridge-base.cpp` | `ForceUp()`: D-Bus 없으면 실패 대신 JUST_STARTED 반환 |
| `bridge-impl.cpp` | `Initialize()`: D-Bus 없으면 직접 `ForceUp()` 호출 |
| `bridge-impl.cpp` | `ForceUp()`: IPC 관련 코드를 `mIpcServer` null 체크로 보호 |
| `bridge-impl.cpp` | `ForceDown()`: D-Bus 관련 cleanup을 null 체크로 보호 |

### dali-adaptor / dali-toolkit

변경 없음 — R1이 해결되면 기존 `GrabHighlight()` / `ClearHighlight()` 코드가 그대로 동작.

---

## 영향도 분석

### 안전한 변경
- `Bridge::ForceUp()` (mData 설정) — 변경 없음
- `ControlAccessible::GrabHighlight()` — 변경 없음 (IsUp() 체크만 있음)
- `ActorAccessible::GrabHighlight()` — 변경 없음

### 주의 필요
- `BridgeBase::ForceUp()`: D-Bus 없이 JUST_STARTED 반환하면, 이후 `mIpcServer`가 nullptr
  - `RegisterInterfaces()`에서 `mIpcServer->addInterface()` 호출 → null deref 가능
  - `emitSignal()` 호출 시 `mIpcServer` null deref 가능
  - 모든 `mIpcServer` 사용처에 null guard 필요
- `BridgeImpl::ForceDown()`: `UnembedSocket()`, `ReleaseBusName()` — D-Bus 없으면 skip 필요
- `Emit*` 함수들 (bridge-object.cpp): `mIpcServer->emitSignal()` — null guard 필요

### 기존 동작 보존
- Tizen에서의 동작은 변경 없음 (D-Bus가 있으므로 기존 경로 그대로)
- `mIsEnabled`이 AT-SPI 서비스에 의해 제어되는 기존 동작 유지
