# Handover: Phase 5 (Toolkit Integration) Complete

## What was done

### Phase 5a: accessibility-common preparation
- Verified library installs correctly (headers + shared lib)
- Added `ACCESSIBILITY_API` to `CollectionImpl` for symbol export
- Created `dali2-accessibility-common.pc` pkg-config file

### Phase 5b: dali-adaptor integration
- Replaced all atspi-interface headers (13 files) with forwarding headers (`using ::Accessibility::TypeName`)
- Replaced `accessibility.h`, `accessibility-bridge.h`, `accessibility-bitset.h` with forwarding headers
- Moved Actor-specific static methods to `ActorAccessible` (`Get`, `GetOwningPtr`, highlight management)
- Added `GetWindowAccessible(Window)` and `ConvertKeyEvent(KeyEvent)` helpers
- Wired `PlatformCallbacks` in `adaptor-impl.cpp` (addIdle, removeIdle, timers)
- Updated `window-impl.cpp` to use `Accessibility::Rect<int>` for `EmitBoundsChanged`
- Deleted all bridge code from `dali/internal/accessibility/bridge/` (~15,500 lines removed)
- Added `FIND_LIBRARY` for accessibility-common in CMakeLists.txt
- Added `dali2-accessibility-common` as a dependency in `dali2-adaptor.pc.in`

### Phase 5c: dali-toolkit adaptation
- Mechanical rename: `Accessible::Get(actor)` → `ActorAccessible::Get(actor)` (~60 occurrences, 12 files)
- Fixed `Dali::Rect<float>` → `Accessibility::Rect<float>` for `GetShowingGeometry`, `IsShowingGeometryOnScreen`, `GetRangeExtents`
- Fixed `Signal.Connect(this, &Method)` → `Signal.Connect([this]() { Method(); })` (4 files: text-editor, text-field, text-label, web-view)
- Fixed `bridge->GetAccessible(Actor)` → `ActorAccessible::Get(Actor)` in `control-data-impl.cpp`

### Avoiding unnecessary Accessible creation
- `IsAccessibleCreated()` now uses `bridge->GetAccessible(actorId)` (lookup-only, no creation)
- Added `RegisterDefaultLabel(Bridge*, Actor)` / `UnregisterDefaultLabel(Bridge*, Actor)` convenience functions in dali-adaptor that do lookup-only via `bridge->GetAccessible(actorId)`
- `EmitAccessibilityStateChanged` uses these convenience functions instead of `ActorAccessible::Get()`

### macOS crash fix
- `InitializeAccessibilityStatusClient()` and `ForceUp()` crashed on macOS because `DBusWrapper::Installed()` returns nullptr (no eldbus)
- Added null guards in `bridge-impl.cpp` (`InitializeAccessibilityStatusClient`) and `bridge-base.cpp` (`ForceUp`, `ForceDown`)
- Bridge gracefully falls back to inactive state instead of crashing

### Namespace fix (dali-demo)
- `::Accessibility` namespace leaks into global scope via forwarding headers
- Files with `using namespace Dali;` see both `Dali::Accessibility` and `::Accessibility` as `Accessibility`
- Fixed in dali-demo by qualifying as `Dali::Accessibility::`

## Commits

| Repo | Commit | Description |
|------|--------|-------------|
| accessibility-common | `9bbda74` | Export CollectionImpl symbol, install collection header |
| accessibility-common | `f496820` | Guard DBusWrapper null checks for macOS |
| dali-adaptor | `63a50aba6` | Phase 5b: full integration (67 files, -15,517 lines) |
| dali-adaptor | `31cdc2553` | RegisterDefaultLabel convenience functions |
| dali-toolkit | `0c38079e3` | Phase 5c: ActorAccessible::Get rename (12 files) |
| dali-toolkit | `d8f0e85d7` | Rect types, Signal.Connect, GetAccessible fixes |
| dali-toolkit | `9f949f2a0` | Avoid unnecessary Accessible creation |
| dali-demo | `c8044f25` | Qualify Accessibility namespace |

## Verification

Full build chain passes:
```
accessibility-common → dali-core → dali-adaptor → dali-toolkit → dali-demo
```
- `accessibility-test`: 31 passed, 0 failed
- `hello-world.example`: runs, renders, exits cleanly (exit code 0)
- DBusWrapper warnings on macOS are expected and harmless

## Known Issues

- `::Accessibility` namespace pollution: any file that `#include`s the forwarding headers AND does `using namespace Dali;` must use `Dali::Accessibility::` explicitly. This affects dali-demo and could affect user apps. Consider wrapping accessibility-common includes to avoid leaking `::Accessibility` into global scope.
- `DBusWrapper::Installed()` logs error on every call when no wrapper is installed (noisy on macOS). Could suppress with a flag after first warning.
- `ActorAccessible::Get(Actor)` creates Accessible on first call. Several call sites in dali-toolkit rely on this creation behavior. A lookup-only variant would be cleaner (see "Accessible ownership" discussion below).

## What's next

### Option A: E2E accessibility test with web inspector
Create or find a dali-demo example with buttons/labels/sliders, connect accessibility-common's web inspector to the live DALi tree, and verify the full pipeline: DALi Actor → ControlAccessible → Bridge → Web Inspector display. This proves Phase 5 works beyond just compiling.

### Option B: Rethink Accessible::Get and ownership
`ActorAccessible::Get(Actor)` is convenient but conflates lookup and creation. Accessible objects are currently owned by dali-adaptor (stored in Bridge's map by actor ID), but the Bridge and tree logic live in accessibility-common. Moving ownership into accessibility-common would clean up the architecture:
- `Bridge::GetAccessible(id)` for lookup
- `Bridge::AddAccessible(id, ptr)` for registration
- `ActorAccessible::Get(Actor)` remains as a dali-adaptor convenience that wraps lookup+create
- A `FindAccessible(Actor)` for lookup-only would prevent accidental creation

### Option C: From PLAN.md
- Phase 4e: macOS NSAccessibility backend (VoiceOver integration)
- Phase 4f: TIDL IPC backend
