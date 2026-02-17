#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_IPC_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_IPC_CLIENT_H

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
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/internal/bridge/ipc/ipc-client.h>

namespace Ipc
{
/**
 * @brief D-Bus implementation of the IPC client interface.
 *
 * Wraps DBus::DBusClient and delegates all operations to it.
 */
class DbusIpcClient : public Client
{
public:
  /**
   * @brief Default constructor, creates non-connected client.
   */
  DbusIpcClient() = default;

  /**
   * @brief Constructs a D-Bus IPC client using a connection type.
   *
   * @param[in] busName Name of the bus to connect to
   * @param[in] pathName Object path
   * @param[in] interfaceName Interface name
   * @param[in] tp Connection type (SYSTEM or SESSION)
   */
  DbusIpcClient(std::string busName, std::string pathName, std::string interfaceName, DBus::ConnectionType tp)
  : mDbusClient(std::move(busName), std::move(pathName), std::move(interfaceName), tp)
  {
  }

  /**
   * @brief Constructs a D-Bus IPC client using an existing connection.
   *
   * @param[in] busName Name of the bus to connect to
   * @param[in] pathName Object path
   * @param[in] interfaceName Interface name
   * @param[in] conn Existing D-Bus connection
   */
  DbusIpcClient(std::string busName, std::string pathName, std::string interfaceName, const DBusWrapper::ConnectionPtr& conn = {})
  : mDbusClient(std::move(busName), std::move(pathName), std::move(interfaceName), conn)
  {
  }

  ~DbusIpcClient() override = default;

  DbusIpcClient(const DbusIpcClient&)            = delete;
  DbusIpcClient& operator=(const DbusIpcClient&) = delete;
  DbusIpcClient(DbusIpcClient&&)                  = default;
  DbusIpcClient& operator=(DbusIpcClient&&)       = default;

  // Ipc::Client interface

  bool isConnected() const override
  {
    return bool(mDbusClient);
  }

  // D-Bus-specific accessors

  /**
   * @brief Returns a reference to the underlying DBus::DBusClient.
   *
   * Used by bridge modules that need D-Bus-specific operations
   * like method calls, property access, and signal listening.
   */
  DBus::DBusClient& getDbusClient()
  {
    return mDbusClient;
  }

  const DBus::DBusClient& getDbusClient() const
  {
    return mDbusClient;
  }

private:
  DBus::DBusClient mDbusClient;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_IPC_CLIENT_H
