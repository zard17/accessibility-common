#ifndef ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_OBJECT_H
#define ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_OBJECT_H

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

// EXTERNAL INCLUDES
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>
#include <accessibility/api/types.h>
#include <accessibility/internal/bridge/bridge-base.h>

/**
 * @brief The BridgeObject class is to correspond with Accessibility::Bridge.
 */
class BridgeObject : public virtual BridgeBase
{
protected:
  /**
   * @brief Constructor.
   */
  BridgeObject();

  /**
   * @brief Registers Bridge functions to dbus interfaces.
   */
  void RegisterInterfaces();

  /**
   * @copydoc Accessibility::Bridge::EmitActiveDescendantChanged()
   */
  void EmitActiveDescendantChanged(Accessibility::Accessible* obj, Accessibility::Accessible* child) override;

  /**
   * @copydoc Accessibility::Bridge::EmitCursorMoved()
   */
  void EmitCursorMoved(Accessibility::Accessible* obj, unsigned int cursorPosition) override;

  /**
   * @copydoc Accessibility::Bridge::EmitTextChanged()
   */
  void EmitTextChanged(Accessibility::Accessible* obj, Accessibility::TextChangedState state, unsigned int position, unsigned int length, const std::string& content) override;

  /**
   * @copydoc Accessibility::Bridge::EmitStateChanged()
   */
  void EmitStateChanged(std::shared_ptr<Accessibility::Accessible> obj, Accessibility::State state, int newValue, int reserved) override;

  /**
   * @copydoc Accessibility::Bridge::Emit()
   */
  void Emit(Accessibility::Accessible* obj, Accessibility::WindowEvent event, unsigned int detail = 0) override;

  /**
   * @copydoc Accessibility::Bridge::Emit()
   */
  void Emit(std::shared_ptr<Accessibility::Accessible> obj, Accessibility::ObjectPropertyChangeEvent event) override;

  /**
   * @copydoc Accessibility::Bridge::EmitBoundsChanged()
   */
  void EmitBoundsChanged(std::shared_ptr<Accessibility::Accessible> obj, Accessibility::Rect<int> rect) override;

  /**
   * @copydoc Accessibility::Bridge::EmitPostRender()
   */
  void EmitPostRender(std::shared_ptr<Accessibility::Accessible> obj) override;

  /**
   * @copydoc Accessibility::Bridge::EmitMovedOutOfScreen()
   */
  void EmitMovedOutOfScreen(Accessibility::Accessible* obj, Accessibility::ScreenRelativeMoveType type) override;

  /**
   * @copydoc Accessibility::Bridge::EmitScrollStarted()
   */
  void EmitScrollStarted(Accessibility::Accessible* obj) override;

  /**
   * @copydoc Accessibility::Bridge::EmitScrollFinished()
   */
  void EmitScrollFinished(Accessibility::Accessible* obj) override;

};

#endif // ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_OBJECT_H
