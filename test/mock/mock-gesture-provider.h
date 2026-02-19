#ifndef ACCESSIBILITY_TEST_MOCK_GESTURE_PROVIDER_H
#define ACCESSIBILITY_TEST_MOCK_GESTURE_PROVIDER_H

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
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/gesture-provider.h>

/**
 * @brief Mock GestureProvider that allows tests to fire gestures programmatically.
 */
class MockGestureProvider : public Accessibility::GestureProvider
{
public:
  void onGestureReceived(std::function<void(const Accessibility::GestureInfo&)> callback) override
  {
    mCallbacks.push_back(std::move(callback));
  }

  /**
   * @brief Fires a gesture event to all registered callbacks.
   */
  void fireGesture(const Accessibility::GestureInfo& gesture)
  {
    for(auto& cb : mCallbacks)
    {
      cb(gesture);
    }
  }

private:
  std::vector<std::function<void(const Accessibility::GestureInfo&)>> mCallbacks;
};

#endif // ACCESSIBILITY_TEST_MOCK_GESTURE_PROVIDER_H
