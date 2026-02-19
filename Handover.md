# Handover: TIDL IPC Backend (Phase 2.6)

## 완료된 Phase 상태

| Phase | Status |
|-------|--------|
| 2.5 (GDBus) | **DONE** — `dbus-gdbus.cpp`, unit 48개 + integration 42개 테스트 통과 |
| 2.6 (TIDL) | **Stage A DONE** — scaffold + tidlc 코드 생성 검증. Stage B/C는 Tizen 디바이스 필요 |
| 2.7 (Tree embedding) | **DONE** — Socket/Plug embed/unembed/setOffset 테스트 완료 |

## Phase 2.6: 완료된 작업 (Stage A)

### 생성된 파일 (`accessibility/internal/bridge/tidl/`)

| File | Description |
|------|-------------|
| `accessibility-service.tidl` | TIDL 인터페이스 정의 (tidlc v2.3.3 검증 완료) |
| `tidl-interface-description.h` | `addMethod<T>` / `addProperty<T>` / `addSignal<ARGS...>` type-erased 저장 |
| `tidl-ipc-server.h/cpp` | `TidlIpcServer : Ipc::Server` (scaffold: 인터페이스 등록만, dispatch 미구현) |
| `tidl-transport-factory.h` | `TidlTransportFactory : Ipc::TransportFactory` |
| `tidl-status-monitor.h` | scaffold: `isEnabled=true` 반환 |
| `tidl-key-event-forwarder.h` | scaffold: `consumed=false` 반환 |
| `tidl-direct-reading-client.h` | scaffold: error 반환 |
| `tidl-registry-client.h` | scaffold: 빈 이벤트 리스트 반환 |
| `tidl-socket-client.h` | scaffold: error 반환 |

### 수정된 파일

| File | Change |
|------|--------|
| `bridge/file.list` | `accessibility_common_tidl_src_files` 추가 |
| `build/tizen/CMakeLists.txt` | `ENABLE_TIDL` 옵션, `rpc-port` pkg-config, TIDL 소스 선택, 테스트 바이너리 지원 |
| `bridge/bridge-impl.cpp` | `#ifdef ENABLE_TIDL_BACKEND` → `TidlTransportFactory` / `DbusTransportFactory` 분기 |
| `bridge/bridge-base.h` | `AddFunctionToInterface` 등 4개 헬퍼에서 `TidlInterfaceDescription` cast 분기 |

### 테스트 결과

- D-Bus 백엔드 (ENABLE_TIDL=OFF): **56/56 passed** — regression 없음
- TIDL 공유 라이브러리 (ENABLE_TIDL=ON): 컴파일/링크 성공
- TIDL 테스트 바이너리: 컴파일 성공, 브리지 라이프사이클 동작 (14개 인터페이스 등록 확인)

### tidlc 코드 생성 검증

macOS에서 `tidlc` v2.3.3 (`download.tizen.org`에서 다운로드) 실행 확인:

```bash
tidlc -s -l C++ -i accessibility-service.tidl -o generated/stub    # stub (server)
tidlc -p -l C++ -i accessibility-service.tidl -o generated/proxy   # proxy (client)
```

생성 결과: stub header 20KB + impl 54KB, proxy header 15KB + impl 159KB. `ServiceBase` 클래스에 모든 TIDL 메서드가 pure virtual로 생성됨.

## macOS 테스트 한계 (Limitation)

생성된 코드가 Tizen 플랫폼 API를 직접 호출:

| 의존성 | stub .cc | proxy .cc | 비고 |
|--------|----------|-----------|------|
| `rpc_port_*` 호출 | 477회 | 1,553회 | P2P 소켓, 직렬화 |
| 필요 헤더 | `bundle.h`, `rpc-port.h`, `rpc-port-parcel.h`, `dlog.h` | 동일 | Tizen 전용 |

**GDBus와의 차이**: GDBus는 `brew install dbus`로 macOS에서 full IPC round-trip 테스트가 가능했으나, TIDL의 `rpc_port`는 Tizen 앱 프레임워크(cynara, aul) 위에서만 동작. 헤더를 제공해도 컴파일만 되고, 실제 P2P 연결·데이터 직렬화·보안 검증은 Tizen 런타임 없이 불가능.

## Tizen 디바이스 환경에서 해야 할 작업 (Stage B/C)

### Stage B: 생성 코드 통합 (Tizen SDK 필요)

1. **tidlc 코드 생성**
   ```bash
   tidlc -s -l C++ -i accessibility-service.tidl -o generated/accessibility-bridge-stub
   tidlc -p -l C++ -i accessibility-service.tidl -o generated/accessibility-bridge-proxy
   ```

2. **TidlIpcServer dispatch 구현**
   - 생성된 `ServiceBase`를 상속하는 `AccessibilityBridgeService` 클래스 작성
   - 각 virtual 메서드(예: `GetName(objectPath)`)에서:
     1. `objectPath`로 `FindCurrentObject()` 호출하여 대상 `Accessible*` 해석
     2. `TidlInterfaceDescription`에 저장된 핸들러에서 해당 메서드 콜백 lookup
     3. 콜백 실행 후 결과를 TIDL 반환 타입(bundle 등)으로 변환
   - `mCurrentObjectPath`를 설정하여 `getCurrentObjectPath()` 동작

3. **5개 Client wrapper 구현**
   - 생성된 proxy 클래스(`proxy::AccessibilityBridge`)를 감싸서 Ipc 인터페이스 구현:
     - `TidlStatusMonitor`: `proxy.GetIsEnabled()` / `OnIsEnabledChanged` delegate
     - `TidlKeyEventForwarder`: `proxy.NotifyListenersSync()`
     - `TidlDirectReadingClient`: `proxy.ReadCommand()` / `OnReadingStateChanged` delegate
     - `TidlRegistryClient`: `proxy.GetRegisteredEvents()` / `OnEventListenerRegistered` delegate
     - `TidlSocketClient`: `proxy.Embed()` / `proxy.Unembed()`

4. **CMake 업데이트**
   - `file.list`에 생성된 `.cc` 파일 추가
   - `PKG_CHECK_MODULES(RPC_PORT rpc-port)` 활성화 후 실제 링크

### Stage C: 통합 테스트 (Tizen 디바이스/에뮬레이터)

1. **빌드 및 기본 테스트**
   ```bash
   cmake .. -DENABLE_ATSPI=ON -DENABLE_TIDL=ON -DENABLE_PKG_CONFIGURE=ON
   make -j8
   ./accessibility-test  # TIDL transport로 전체 테스트 통과 확인
   ```

2. **스크린 리더 연동**: 실제 Tizen 스크린 리더가 TIDL proxy로 접속하여 `GetName`, `GetRole`, `GetNeighbor` 등 호출 확인

3. **성능 비교**: D-Bus vs TIDL latency 측정 (daemon hop 제거 효과)

4. **Cynara 보안**: 앱 간 접근 권한 검증

## 다음 Phase

### Phase 3: AccessibilityService base class
- `NodeProxy` interface (~40 methods)
- `AppRegistry` (desktop, active window, app lifecycle)
- `GestureProvider` (gesture → navigation mapping)
- `AccessibilityService` base class with virtual hooks

## Verification commands

```bash
cd ~/tizen/accessibility-common/build/tizen/build

# D-Bus backend (all tests pass)
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-test  # 56 passed

# TIDL backend scaffold (compiles, bridge lifecycle works)
cmake .. -DENABLE_ATSPI=ON -DENABLE_TIDL=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8

# tidlc code generation (macOS — download from download.tizen.org)
tidlc -s -l C++ -i accessibility-service.tidl -o generated/stub
tidlc -p -l C++ -i accessibility-service.tidl -o generated/proxy
```
