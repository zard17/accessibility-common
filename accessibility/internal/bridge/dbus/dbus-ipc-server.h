#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_IPC_SERVER_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_IPC_SERVER_H

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
#include <memory>
#include <string>
#include <tuple>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/internal/bridge/ipc/ipc-server.h>

namespace Ipc
{
/**
 * @brief D-Bus implementation of the IPC server interface.
 *
 * Wraps DBus::DBusServer and delegates all operations to it.
 */
class DbusIpcServer : public Server
{
public:
  /**
   * @brief Constructs a D-Bus IPC server from an existing connection.
   *
   * @param[in] conn D-Bus connection pointer
   */
  explicit DbusIpcServer(const DBusWrapper::ConnectionPtr& conn)
  : mDbusServer(conn),
    mConnection(conn)
  {
  }

  ~DbusIpcServer() override = default;

  DbusIpcServer(const DbusIpcServer&)            = delete;
  DbusIpcServer& operator=(const DbusIpcServer&) = delete;
  DbusIpcServer(DbusIpcServer&&)                  = default;
  DbusIpcServer& operator=(DbusIpcServer&&)       = default;

  // Ipc::Server interface

  void addInterface(const std::string& pathName,
                    InterfaceDescription& desc,
                    bool fallback = false) override
  {
    auto& dbusDesc = static_cast<DBus::DBusInterfaceDescription&>(desc);
    mDbusServer.addInterface(pathName, dbusDesc, fallback);
  }

  std::string getBusName() const override
  {
    return mDbusServer.getBusName();
  }

  std::string getCurrentObjectPath() const override
  {
    return DBus::DBusServer::getCurrentObjectPath();
  }

  void emitSignal(const std::string&             objectPath,
                  const std::string&             interfaceName,
                  const std::string&             signalName,
                  const std::string&             detail,
                  int                            detail1,
                  int                            detail2,
                  const SignalVariant&            data,
                  const Accessibility::Address&  sender) override
  {
    std::visit([&](const auto& value)
    {
      using T = std::decay_t<decltype(value)>;
      if constexpr(std::is_same_v<T, int>)
      {
        mDbusServer.emit2<std::string, int, int, DBus::EldbusVariant<int>, Accessibility::Address>(
          objectPath, interfaceName, signalName, detail, detail1, detail2, {value}, sender);
      }
      else if constexpr(std::is_same_v<T, std::string>)
      {
        mDbusServer.emit2<std::string, int, int, DBus::EldbusVariant<std::string>, Accessibility::Address>(
          objectPath, interfaceName, signalName, detail, detail1, detail2, {value}, sender);
      }
      else if constexpr(std::is_same_v<T, Accessibility::Address>)
      {
        mDbusServer.emit2<std::string, int, int, DBus::EldbusVariant<Accessibility::Address>, Accessibility::Address>(
          objectPath, interfaceName, signalName, detail, detail1, detail2, {value}, sender);
      }
      else if constexpr(std::is_same_v<T, Accessibility::Rect<int>>)
      {
        DBus::EldbusVariant<std::tuple<int32_t, int32_t, int32_t, int32_t>> tmp{
          std::tuple<int32_t, int32_t, int32_t, int32_t>{value.x, value.y, value.width, value.height}};
        mDbusServer.emit2<std::string, int, int, DBus::EldbusVariant<std::tuple<int32_t, int32_t, int32_t, int32_t>>, Accessibility::Address>(
          objectPath, interfaceName, signalName, detail, detail1, detail2, tmp, sender);
      }
    }, data);
  }

  std::unique_ptr<InterfaceDescription> createInterfaceDescription(const std::string& interfaceName) override
  {
    return std::make_unique<DBus::DBusInterfaceDescription>(interfaceName);
  }

  // D-Bus-specific accessors

  /**
   * @brief Returns a reference to the underlying DBus::DBusServer.
   *
   * Used by bridge modules that need D-Bus-specific operations
   * like signal emission (emit2).
   */
  DBus::DBusServer& getDbusServer()
  {
    return mDbusServer;
  }

  const DBus::DBusServer& getDbusServer() const
  {
    return mDbusServer;
  }

  /**
   * @brief Returns the underlying D-Bus connection.
   */
  const DBusWrapper::ConnectionPtr& getConnection() const
  {
    return mConnection;
  }

private:
  DBus::DBusServer       mDbusServer;
  DBusWrapper::ConnectionPtr mConnection;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_IPC_SERVER_H
