#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_KEY_EVENT_FORWARDER_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_KEY_EVENT_FORWARDER_H

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

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/internal/bridge/ipc/ipc-key-event-forwarder.h>

namespace Ipc
{
/**
 * @brief D-Bus implementation of KeyEventForwarder.
 *
 * Wraps a DBus::DBusClient connected to the device event controller.
 */
class DbusKeyEventForwarder : public KeyEventForwarder
{
public:
  /**
   * @brief Constructs a key event forwarder.
   *
   * @param[in] busName D-Bus bus name
   * @param[in] path Object path
   * @param[in] interface Interface name
   * @param[in] conn Existing D-Bus connection
   */
  DbusKeyEventForwarder(std::string busName, std::string path, std::string interface,
                        const DBusWrapper::ConnectionPtr& conn)
  : mClient(std::move(busName), std::move(path), std::move(interface), conn)
  {
  }

  ~DbusKeyEventForwarder() override = default;

  void notifyListenersSync(uint32_t                                keyType,
                           int32_t                                 keyCode,
                           int32_t                                 timeStamp,
                           const std::string&                      keyName,
                           bool                                    isText,
                           std::function<void(ValueOrError<bool>)> callback) override
  {
    using ArgumentTypes = std::tuple<uint32_t, int32_t, int32_t, int32_t, int32_t, std::string, bool>;
    ArgumentTypes arguments(keyType, 0, keyCode, 0, timeStamp, keyName, isText);
    mClient.method<bool(ArgumentTypes)>("NotifyListenersSync").asyncCall(std::move(callback), arguments);
  }

private:
  DBus::DBusClient mClient;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_KEY_EVENT_FORWARDER_H
