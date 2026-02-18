#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_KEY_EVENT_FORWARDER_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_KEY_EVENT_FORWARDER_H

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
#include <functional>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-result.h>

namespace Ipc
{
/**
 * @brief Abstract interface for forwarding key events to the AT registry.
 *
 * Replaces direct DBus::DBusClient usage for calling NotifyListenersSync
 * on the device event controller.
 */
class KeyEventForwarder
{
public:
  virtual ~KeyEventForwarder() = default;

  /**
   * @brief Asynchronously forwards a key event to registered listeners.
   *
   * @param[in] keyType Key type (0 = down, 1 = up)
   * @param[in] keyCode Key code
   * @param[in] timeStamp Event timestamp
   * @param[in] keyName Key name string
   * @param[in] isText Whether the event represents text input
   * @param[in] callback Called with the result (bool consumed or error)
   */
  virtual void notifyListenersSync(uint32_t                                keyType,
                                   int32_t                                 keyCode,
                                   int32_t                                 timeStamp,
                                   const std::string&                      keyName,
                                   bool                                    isText,
                                   std::function<void(ValueOrError<bool>)> callback) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_KEY_EVENT_FORWARDER_H
