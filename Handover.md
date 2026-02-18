# Handover: Phase 2.7 Complete — Tree Embedding Tests

## What was done

### Phase 2.7: Tree Embedding Tests (COMPLETE)

Added 9 unit tests for Socket Embed/Unembed/SetOffset functionality, exercising the server-side `ApplicationAccessible` through the D-Bus Socket interface via MockDBusWrapper.

#### New helpers in `test/test-app.cpp`
- `CreateSocketClient()` — DBusClient pointing to root path with Socket interface
- `CreateRootAccessibleClient()` — DBusClient pointing to root with Accessible interface

#### Test groups added

**Step 13: Server-side Socket Embed/Unembed (4 tests)**
- A1: Embed sets embedded state — GetIndexInParent returns 0
- A2: Embed returns application root address (path="root")
- A3: Unembed resets state — GetIndexInParent returns error
- A4: Unembed with wrong plug is no-op — still embedded

**Step 14: SetOffset + Extents verification (3 tests)**
- A5: SetOffset(100,200) shifts GetExtents by offset (button: 10→110, 20→220)
- A6: SetOffset is no-op when not embedded — extents unchanged
- A7: SetOffset resets after Unembed — extents back to original

**Step 15: Edge Cases (2 tests)**
- C1: Double-embed overwrites parent — Unembed(old) is no-op
- C2: Embed-Unembed-Embed cycle works correctly

#### Test count
- **56 passed, 0 failed** (31 existing + 25 new assertions across 9 test cases)

## Key observations

1. **MockSocketClient not needed**: Existing MockDBusWrapper fallback routing handles Socket interface calls. BridgeSocket::RegisterInterfaces() registers at "/" with fallback=true, so CreateSocketClient with the root path works.

2. **ForceUp embeds via mock**: During bridge ForceUp, BridgeImpl::EmbedSocket sends an outbound Embed call that the mock routes to the bridge's own Socket handler (due to fallback matching). This means `mIsEmbedded` is already true before tests start. Tests account for this by always calling Embed explicitly first.

3. **Address bus name convention**: ApplicationAccessible::GetAddress() returns `{"", "root"}` but the framework fills in the connection's bus name during serialization. Test checks path only, not bus.

## Files changed

**Modified files (1):**
- `test/test-app.cpp` — Added CreateSocketClient, CreateRootAccessibleClient helpers + 9 tests (Steps 13-15)

## What's next

Per updated plan priority:
1. **Phase 2.5**: eldbus → GDBus migration (dbus-gdbus.cpp implementation)
2. **Phase 2.6**: TIDL IPC backend plan
3. **Phase 3**: AccessibilityService base class (NodeProxy, AppRegistry, GestureProvider)
4. **Phase 4a**: Screen reader C++ rewrite

## Verification commands

```bash
cd ~/tizen/accessibility-common/build/tizen/build
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-test  # 56 passed, 0 failed
```
