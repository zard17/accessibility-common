#ifndef ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_SELECTION_H
#define ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_SELECTION_H

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

// INTERNAL INCLUDES
#include <accessibility/api/selection.h>
#include <accessibility/internal/bridge/bridge-base.h>

/**
 * @brief The BridgeSelection class is to correspond with Accessibility::Selection.
 */
class BridgeSelection : public virtual BridgeBase
{
protected:
  BridgeSelection() = default;

  /**
   * @brief Registers Selection functions to dbus interfaces.
   */
  void RegisterInterfaces();

  /**
   * @brief Returns the Selection object of the currently executed DBus method call.
   *
   * @return The Selection object
   */
  std::shared_ptr<Accessibility::Selection> FindSelf() const;

public:
  /**
   * @copydoc Accessibility::Selection::GetSelectedChildrenCount()
   */
  DBus::ValueOrError<int32_t> GetSelectedChildrenCount();

  /**
   * @copydoc Accessibility::Selection::GetSelectedChild()
   */
  DBus::ValueOrError<Accessibility::Accessible*> GetSelectedChild(int32_t selectedChildIndex);

  /**
   * @copydoc Accessibility::Selection::SelectChild()
   */
  DBus::ValueOrError<bool> SelectChild(int32_t childIndex);

  /**
   * @copydoc Accessibility::Selection::DeselectSelectedChild()
   */
  DBus::ValueOrError<bool> DeselectSelectedChild(int32_t selectedChildIndex);

  /**
   * @copydoc Accessibility::Selection::IsChildSelected()
   */
  DBus::ValueOrError<bool> IsChildSelected(int32_t childIndex);

  /**
   * @copydoc Accessibility::Selection::SelectAll()
   */
  DBus::ValueOrError<bool> SelectAll();

  /**
   * @copydoc Accessibility::Selection::ClearSelection()
   */
  DBus::ValueOrError<bool> ClearSelection();

  /**
   * @copydoc Accessibility::Selection::DeselectChild()
   */
  DBus::ValueOrError<bool> DeselectChild(int32_t childIndex);
};

#endif // ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_SELECTION_H
