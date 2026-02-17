#ifndef ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_ACTION_H
#define ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_ACTION_H

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
#include <tuple>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/action.h>
#include <accessibility/internal/bridge/bridge-base.h>

/**
 * @brief The BridgeAction class is to correspond with Accessibility::Action.
 */
class BridgeAction : public virtual BridgeBase
{
protected:
  BridgeAction() = default;

  /**
   * @brief Registers Action functions to dbus interfaces.
   */
  void RegisterInterfaces();

  /**
   * @brief Returns the Action object of the currently executed DBus method call.
   *
   * @return The Action object
   */
  std::shared_ptr<Accessibility::Action> FindSelf() const;

public:
  /**
   * @copydoc Accessibility::Action::GetActionName()
   */
  DBus::ValueOrError<std::string> GetActionName(int32_t index);

  /**
   * @copydoc Accessibility::Action::GetLocalizedActionName()
   */
  DBus::ValueOrError<std::string> GetLocalizedActionName(int32_t index);

  /**
   * @copydoc Accessibility::Action::GetActionDescription()
   */
  DBus::ValueOrError<std::string> GetActionDescription(int32_t index);

  /**
   * @copydoc Accessibility::Action::GetActionKeyBinding()
   */
  DBus::ValueOrError<std::string> GetActionKeyBinding(int32_t index);

  /**
   * @copydoc Accessibility::Action::GetActionCount()
   */
  DBus::ValueOrError<int32_t> GetActionCount();

  /**
   * @copydoc Accessibility::Action::DoAction()
   */
  DBus::ValueOrError<bool> DoAction(int32_t index);

  /**
   * @copydoc Accessibility::Action::DoAction()
   */
  DBus::ValueOrError<bool> DoActionName(std::string name);
};

#endif // ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_ACTION_H
