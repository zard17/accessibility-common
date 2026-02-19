#ifndef ACCESSIBILITY_API_ACCESSIBILITY_EVENT_H
#define ACCESSIBILITY_API_ACCESSIBILITY_EVENT_H

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
#include <cstdint>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>

namespace Accessibility
{
/**
 * @brief Structure describing an accessibility event received from an application.
 */
struct AccessibilityEvent
{
  /**
   * @brief Type of accessibility event.
   */
  enum class Type
  {
    STATE_CHANGED,
    PROPERTY_CHANGED,
    BOUNDS_CHANGED,
    ACTIVE_DESCENDANT_CHANGED,
    TEXT_CARET_MOVED,
    TEXT_CHANGED,
    MOVED_OUT,
    SCROLL_STARTED,
    SCROLL_FINISHED,
    WINDOW_CHANGED
  };

  Type        type{};
  Address     source;
  std::string detail;
  int         detail1{0};
  int         detail2{0};
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_ACCESSIBILITY_EVENT_H
