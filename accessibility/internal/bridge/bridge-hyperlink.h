#ifndef ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_HYPERLINK_H
#define ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_HYPERLINK_H

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
#include <accessibility/api/hyperlink.h>
#include <accessibility/internal/bridge/bridge-base.h>

class BridgeHyperlink : public virtual BridgeBase
{
protected:
  BridgeHyperlink() = default;

  /**
   * @brief Registers Hyperlink functions to dbus interfaces.
   */
  void RegisterInterfaces();

  /**
   * @brief Returns the Hyperlink object of the currently executed DBus method call.
   *
   * @return The Hyperlink object
   */
  std::shared_ptr<Accessibility::Hyperlink> FindSelf() const;

public:
  /**
   * @copydoc Accessibility::Hyperlink::GetEndIndex()
   */
  DBus::ValueOrError<int32_t> GetEndIndex();

  /**
   * @copydoc Accessibility::Hyperlink::GetStartIndex()
   */
  DBus::ValueOrError<int32_t> GetStartIndex();

  /**
   * @copydoc Accessibility::Hyperlink::GetAnchorCount()
   */
  DBus::ValueOrError<int32_t> GetAnchorCount();

  /**
   * @copydoc Accessibility::Hyperlink::GetAnchorAccessible()
   */
  DBus::ValueOrError<Accessibility::Accessible*> GetAnchorAccessible(int32_t anchorIndex);

  /**
   * @copydoc Accessibility::Hyperlink::GetAnchorUri()
   */
  DBus::ValueOrError<std::string> GetAnchorUri(int32_t anchorIndex);

  /**
   * @copydoc Accessibility::Hyperlink::IsValid()
   */
  DBus::ValueOrError<bool> IsValid();
};

#endif // ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_HYPERLINK_H
