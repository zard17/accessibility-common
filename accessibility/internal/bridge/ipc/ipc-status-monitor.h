#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_STATUS_MONITOR_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_STATUS_MONITOR_H

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
#include <accessibility/internal/bridge/ipc/ipc-result.h>

namespace Ipc
{
/**
 * @brief Abstract interface for monitoring accessibility status properties.
 *
 * Replaces direct DBus::DBusClient usage for reading and listening to
 * IsEnabled and ScreenReaderEnabled properties on the accessibility status bus.
 */
class AccessibilityStatusMonitor
{
public:
  virtual ~AccessibilityStatusMonitor() = default;

  /**
   * @brief Returns true if the monitor is connected to the status service.
   */
  virtual bool isConnected() const = 0;

  /**
   * @brief Boolean conversion operator.
   */
  explicit operator bool() const
  {
    return isConnected();
  }

  /**
   * @brief Asynchronously reads the current IsEnabled property value.
   *
   * @param[in] callback Called with the result (bool value or error)
   */
  virtual void readIsEnabled(std::function<void(ValueOrError<bool>)> callback) = 0;

  /**
   * @brief Listens for changes to the IsEnabled property.
   *
   * @param[in] callback Called whenever the property value changes
   */
  virtual void listenIsEnabled(std::function<void(bool)> callback) = 0;

  /**
   * @brief Asynchronously reads the current ScreenReaderEnabled property value.
   *
   * @param[in] callback Called with the result (bool value or error)
   */
  virtual void readScreenReaderEnabled(std::function<void(ValueOrError<bool>)> callback) = 0;

  /**
   * @brief Listens for changes to the ScreenReaderEnabled property.
   *
   * @param[in] callback Called whenever the property value changes
   */
  virtual void listenScreenReaderEnabled(std::function<void(bool)> callback) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_STATUS_MONITOR_H
