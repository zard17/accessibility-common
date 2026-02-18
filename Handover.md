# Handover: Phase 7 — Bridge Lifecycle Decoupling from IPC Transport

## What was done

### Bridge lifecycle decoupling (accessibility-common)
Decoupled Bridge's `IsUp()` from D-Bus connection so accessibility works on platforms without D-Bus (macOS, Windows).

**Problem**: On macOS, `IsUp()` returned false because no D-Bus connection existed. This caused `GrabHighlight()` to be rejected, preventing highlight rendering.

**Solution** (3 files changed in accessibility-common):

#### `bridge-impl.cpp`
- `Initialize()`: When `DBusWrapper::Installed()` returns null, sets `mIsEnabled = true` and `mIsApplicationRunning = true`, then calls `SwitchBridge()` to bring the bridge up locally
- `ForceUp()`: Wrapped all IPC-dependent code (interface registration, registry/reading clients, bus name, embed socket) in `if(mIpcServer)` guard. `mEnabledSignal.Emit()` stays outside — fires regardless of IPC
- `ForceDown()`: Wrapped `UnembedSocket`/`ReleaseBusName` in `if(mIpcServer)` guard
- `EmitKeyEvent/Pause/Resume/StopReading/Say`: Added `|| !mIpcServer` to early-return guards
- `EmbedSocket/UnembedSocket/SetSocketOffset`: Added `if(!mIpcServer) return` guards
- `SetPreferredBusName`: Changed `if(IsUp())` to `if(IsUp() && mIpcServer)`

#### `bridge-base.cpp`
- `ForceUp()`: When `DBusWrapper::Installed()` returns null, returns `JUST_STARTED` instead of `FAILED`. Bridge is up for local accessibility without IPC.

#### `bridge-object.cpp`
- Added `if(!mIpcServer) return;` guard before all 10 `mIpcServer->emitSignal()` call sites (direct calls and inside coalescable message lambdas)

### Web inspector highlight integration (dali-demo)
- `accessibility-inspector-example.cpp`: Added `TriggerEventFactory`-based cross-thread dispatch so web inspector navigation triggers `GrabHighlight()` on the DALi main thread
- `Dali::Timer` doesn't tick on macOS — `TriggerEvent` is the correct mechanism for cross-thread dispatch

## Key findings

- `Dali::Timer` does NOT fire on macOS (created successfully, started, but tick signal never triggers)
- `TriggerEventFactory::CreateTriggerEvent()` works on macOS for cross-thread main-thread dispatch (uses `trigger-event-mac.h` internally)
- `SwitchBridge()` requires `mIsApplicationRunning = true` — without it, `ForceDown()` is called which clears `mApplication->mChildren`
- "ACCESSIBILITY ERROR: No DBusWrapper installed" is logged from `dbus-stub.cpp:204` every time `DBusWrapper::Installed()` is called — pre-existing, harmless on macOS
- `::Accessibility` namespace ambiguity: use `::Accessibility::IsUp()` (not `Dali::Accessibility::IsUp()`) in files with `using namespace Dali;`

## Verification

- `accessibility-test`: 31 passed, 0 failed
- Full stack rebuilt: accessibility-common → dali-core → dali-adaptor → dali-toolkit → dali-demo
- `accessibility-inspector.example`: correct DALi tree in web inspector, highlight renders on navigation

## What's next

- Phase 4e: macOS NSAccessibility backend (VoiceOver integration)
- Phase 4f: TIDL IPC backend
- Suppress noisy "No DBusWrapper installed" warnings (log once, then silence)
- Fix `Dali::Timer` on macOS (or document that TriggerEvent should be used instead)
