#ifndef ACCESSIBILITY_DUMMY_ATSPI_H
#define ACCESSIBILITY_DUMMY_ATSPI_H

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/api/accessibility.h>

namespace Accessibility
{
struct DummyBridge : Accessibility::Bridge
{
  static std::shared_ptr<DummyBridge> GetInstance()
  {
    static auto instance = std::make_shared<DummyBridge>();

    return instance;
  }

  DummyBridge()  = default;
  ~DummyBridge() = default;

  const std::string& GetBusName() const override
  {
    static const std::string name = "";
    return name;
  }

  void AddTopLevelWindow(Accessibility::Accessible* object) override
  {
  }

  void RemoveTopLevelWindow(Accessibility::Accessible* object) override
  {
  }

  void RegisterDefaultLabel(Accessibility::Accessible* accessible) override
  {
  }

  void UnregisterDefaultLabel(Accessibility::Accessible* accessible) override
  {
  }

  Accessibility::Accessible* GetDefaultLabel(Accessibility::Accessible* root) override
  {
    return nullptr;
  }

  void SetApplicationName(std::string name) override
  {
  }

  void SetToolkitName(std::string_view toolkitName) override
  {
  }

  Accessibility::Accessible* GetApplication() const override
  {
    return nullptr;
  }

  Accessibility::Accessible* FindByPath(const std::string& path) const override
  {
    return nullptr;
  }

  void WindowCreated(Accessible* windowRoot) override
  {
  }

  void WindowShown(Accessible* windowRoot) override
  {
  }

  void WindowHidden(Accessible* windowRoot) override
  {
  }

  void WindowFocused(Accessible* windowRoot) override
  {
  }

  void WindowUnfocused(Accessible* windowRoot) override
  {
  }

  void WindowMinimized(Accessible* windowRoot) override
  {
  }

  void WindowRestored(Accessible* windowRoot, WindowRestoreType detail) override
  {
  }

  void WindowMaximized(Accessible* windowRoot) override
  {
  }

  void ApplicationPaused() override
  {
  }

  void ApplicationResumed() override
  {
  }

  void Initialize() override
  {
  }

  void Terminate() override
  {
  }

  ForceUpResult ForceUp() override
  {
    return ForceUpResult::JUST_STARTED;
  }

  void ForceDown() override
  {
  }

  void EmitCursorMoved(Accessibility::Accessible* obj, unsigned int cursorPosition) override
  {
  }

  void EmitActiveDescendantChanged(Accessibility::Accessible* obj, Accessibility::Accessible* child) override
  {
  }

  void EmitTextChanged(Accessibility::Accessible* obj, Accessibility::TextChangedState state, unsigned int position, unsigned int length, const std::string& content) override
  {
  }

  void EmitMovedOutOfScreen(Accessibility::Accessible* obj, ScreenRelativeMoveType type) override
  {
  }

  void EmitScrollStarted(Accessibility::Accessible* obj) override
  {
  }

  void EmitScrollFinished(Accessibility::Accessible* obj) override
  {
  }

  void EmitStateChanged(std::shared_ptr<Accessibility::Accessible> obj, Accessibility::State state, int newValue, int reserved) override
  {
  }

  void Emit(Accessibility::Accessible* obj, Accessibility::WindowEvent event, unsigned int detail) override
  {
  }

  void Emit(std::shared_ptr<Accessibility::Accessible> obj, Accessibility::ObjectPropertyChangeEvent event) override
  {
  }

  void EmitBoundsChanged(std::shared_ptr<Accessibility::Accessible> obj, Rect<int> rect) override
  {
  }

  void EmitPostRender(std::shared_ptr<Accessibility::Accessible> obj) override
  {
  }

  bool EmitKeyEvent(Accessibility::KeyEvent keyEvent, std::function<void(Accessibility::KeyEvent, bool)> callback) override
  {
    return false;
  }

  void Say(const std::string& text, bool discardable, std::function<void(std::string)> callback) override
  {
  }

  void Pause() override
  {
  }

  void Resume() override
  {
  }

  void StopReading(bool alsoNonDiscardable) override
  {
  }

  void SuppressScreenReader(bool suppress) override
  {
  }

  bool GetScreenReaderEnabled() override
  {
    return false;
  }

  bool IsEnabled() override
  {
    return false;
  }

  Address EmbedSocket(const Address& plug, const Address& socket) override
  {
    return {};
  }

  void UnembedSocket(const Address& plug, const Address& socket) override
  {
  }

  void SetSocketOffset(ProxyAccessible* socket, std::int32_t x, std::int32_t y) override
  {
  }

  void SetExtentsOffset(std::int32_t x, std::int32_t y) override
  {
  }

  void SetPreferredBusName(std::string_view preferredBusName) override
  {
  }

  bool AddAccessible(uint32_t actorId, std::shared_ptr<Accessible> accessible) override
  {
    return false;
  }

  void RemoveAccessible(uint32_t actorId) override
  {
  }

  std::shared_ptr<Accessible> GetAccessible(uint32_t objectId) const override
  {
    return nullptr;
  }

  std::shared_ptr<Accessible> GetAccessible(const std::string& path) const override
  {
    return nullptr;
  }

  bool ShouldIncludeHidden() const override
  {
    return false;
  }
};

} // namespace Accessibility

#endif // ACCESSIBILITY_DUMMY_ATSPI_H
