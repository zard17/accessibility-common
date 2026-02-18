#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_SOCKET_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_SOCKET_CLIENT_H

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
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/internal/bridge/ipc/ipc-socket-client.h>

namespace Ipc
{
/**
 * @brief D-Bus implementation of SocketClient.
 *
 * Wraps a DBus::DBusClient for socket Embed/Unembed/SetOffset operations.
 */
class DbusSocketClient : public SocketClient
{
public:
  /**
   * @brief Constructs a socket client.
   *
   * @param[in] busName D-Bus bus name (from socket address)
   * @param[in] path Object path (ATSPI_PREFIX_PATH + socket path)
   * @param[in] interface Interface name
   * @param[in] conn Existing D-Bus connection
   */
  DbusSocketClient(std::string busName, std::string path, std::string interface,
                   const DBusWrapper::ConnectionPtr& conn)
  : mClient(std::move(busName), std::move(path), std::move(interface), conn)
  {
  }

  ~DbusSocketClient() override = default;

  ValueOrError<Accessibility::Address> embed(Accessibility::Address plug) override
  {
    return mClient.method<Accessibility::Address(Accessibility::Address)>("Embed").call(plug);
  }

  void unembed(Accessibility::Address                  plug,
               std::function<void(ValueOrError<void>)> callback) override
  {
    mClient.method<void(Accessibility::Address)>("Unembed").asyncCall(std::move(callback), plug);
  }

  void setOffset(std::int32_t                            x,
                 std::int32_t                            y,
                 std::function<void(ValueOrError<void>)> callback) override
  {
    mClient.method<void(std::int32_t, std::int32_t)>("SetOffset").asyncCall(std::move(callback), x, y);
  }

private:
  DBus::DBusClient mClient;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_SOCKET_CLIENT_H
