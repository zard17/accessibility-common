#ifndef ACCESSIBILITY_API_GESTURE_PROVIDER_H
#define ACCESSIBILITY_API_GESTURE_PROVIDER_H

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
#include <functional>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>

namespace Accessibility
{
/**
 * @brief Abstract interface for receiving gesture events from the platform.
 *
 * Reuses the existing GestureInfo struct from accessibility.h.
 */
class GestureProvider
{
public:
  virtual ~GestureProvider() = default;

  /**
   * @brief Registers a callback for when a gesture is received.
   */
  virtual void onGestureReceived(std::function<void(const GestureInfo&)> callback) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_GESTURE_PROVIDER_H
