#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_KEY_EVENT_FORWARDER_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_KEY_EVENT_FORWARDER_H

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
#include <string>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-key-event-forwarder.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of KeyEventForwarder.
 *
 * Uses TIDL proxy to forward key events to the AT registry via
 * rpc_port direct P2P connection instead of D-Bus.
 *
 * Scaffold: Returns not-consumed for all key events.
 */
class TidlKeyEventForwarder : public KeyEventForwarder
{
public:
  /**
   * @brief Constructs a TIDL key event forwarder.
   *
   * @param[in] server Reference to the TidlIpcServer for connection sharing
   */
  TidlKeyEventForwarder() = default;

  ~TidlKeyEventForwarder() override = default;

  void notifyListenersSync(uint32_t                                keyType,
                           int32_t                                 keyCode,
                           int32_t                                 timeStamp,
                           const std::string&                      keyName,
                           bool                                    isText,
                           std::function<void(ValueOrError<bool>)> callback) override
  {
    // Scaffold: report key as not consumed
    // Real implementation: mProxy->NotifyListenersSync(keyType, keyCode, ...)
    if(callback)
    {
      callback(ValueOrError<bool>(false));
    }
  }
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_KEY_EVENT_FORWARDER_H
