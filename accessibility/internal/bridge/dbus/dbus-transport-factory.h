#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_TRANSPORT_FACTORY_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_TRANSPORT_FACTORY_H

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

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>
#include <accessibility/api/log.h>
#include <accessibility/internal/bridge/dbus/dbus-direct-reading-client.h>
#include <accessibility/internal/bridge/dbus/dbus-ipc-server.h>
#include <accessibility/internal/bridge/dbus/dbus-key-event-forwarder.h>
#include <accessibility/internal/bridge/dbus/dbus-locators.h>
#include <accessibility/internal/bridge/dbus/dbus-registry-client.h>
#include <accessibility/internal/bridge/dbus/dbus-socket-client.h>
#include <accessibility/internal/bridge/dbus/dbus-status-monitor.h>
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/internal/bridge/ipc/ipc-transport-factory.h>

// DBus names/paths (from accessibility-common.h)
#ifndef A11yDbusName
#define A11yDbusName "org.a11y.Bus"
#endif
#ifndef A11yDbusStatusInterface
#define A11yDbusStatusInterface "org.a11y.Status"
#endif
#ifndef A11yDbusPath
#define A11yDbusPath "/org/a11y/bus"
#endif
#ifndef AtspiDbusNameRegistry
#define AtspiDbusNameRegistry "org.a11y.atspi.Registry"
#endif
#ifndef AtspiDbusPathRegistry
#define AtspiDbusPathRegistry "/org/a11y/atspi/registry"
#endif
#ifndef AtspiDbusPathDec
#define AtspiDbusPathDec "/org/a11y/atspi/registry/deviceeventcontroller"
#endif
#ifndef DirectReadingDBusName
#define DirectReadingDBusName "org.tizen.ScreenReader"
#endif
#ifndef DirectReadingDBusInterface
#define DirectReadingDBusInterface "org.tizen.DirectReading"
#endif
#ifndef DirectReadingDBusPath
#define DirectReadingDBusPath "/org/tizen/DirectReading"
#endif

namespace Ipc
{
/**
 * @brief D-Bus implementation of TransportFactory.
 *
 * Creates D-Bus-based IPC components. Uses DBusWrapper for the
 * underlying connection management.
 */
class DbusTransportFactory : public TransportFactory
{
public:
  DbusTransportFactory() = default;
  ~DbusTransportFactory() override = default;

  bool isAvailable() const override
  {
    return DBusWrapper::Installed() != nullptr;
  }

  ValueOrError<ConnectionResult> connect() override
  {
    auto proxy = DBus::DBusClient{
      dbusLocators::atspi::BUS,
      dbusLocators::atspi::OBJ_PATH,
      dbusLocators::atspi::BUS_INTERFACE,
      DBus::ConnectionType::SESSION};

    auto addr = proxy.method<std::string()>(dbusLocators::atspi::GET_ADDRESS).call();

    if(!addr)
    {
      return Ipc::Error{addr.getError().message};
    }

    auto connectionPtr = DBusWrapper::Installed()->eldbus_address_connection_get_impl(std::get<0>(addr));
    auto busName       = DBus::getConnectionName(connectionPtr);
    auto server        = std::make_unique<DbusIpcServer>(connectionPtr);

    ConnectionResult result;
    result.server  = std::move(server);
    result.busName = std::move(busName);
    return result;
  }

  std::unique_ptr<AccessibilityStatusMonitor> createStatusMonitor() override
  {
    return std::make_unique<DbusStatusMonitor>(
      A11yDbusName, A11yDbusPath, A11yDbusStatusInterface);
  }

  std::unique_ptr<KeyEventForwarder> createKeyEventForwarder(Server& server) override
  {
    auto& dbusServer = static_cast<DbusIpcServer&>(server);
    return std::make_unique<DbusKeyEventForwarder>(
      AtspiDbusNameRegistry,
      AtspiDbusPathDec,
      Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::DEVICE_EVENT_CONTROLLER),
      dbusServer.getConnection());
  }

  std::unique_ptr<DirectReadingClient> createDirectReadingClient(Server& server) override
  {
    auto& dbusServer = static_cast<DbusIpcServer&>(server);
    return std::make_unique<DbusDirectReadingClient>(
      DirectReadingDBusName,
      DirectReadingDBusPath,
      DirectReadingDBusInterface,
      dbusServer.getConnection());
  }

  std::unique_ptr<RegistryClient> createRegistryClient(Server& server) override
  {
    auto& dbusServer = static_cast<DbusIpcServer&>(server);
    return std::make_unique<DbusRegistryClient>(
      AtspiDbusNameRegistry,
      AtspiDbusPathRegistry,
      Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::REGISTRY),
      dbusServer.getConnection());
  }

  std::unique_ptr<SocketClient> createSocketClient(const Accessibility::Address& address,
                                                   Server&                       server) override
  {
    auto& dbusServer = static_cast<DbusIpcServer&>(server);
    return std::make_unique<DbusSocketClient>(
      address.GetBus(),
      std::string{ATSPI_PREFIX_PATH} + address.GetPath(),
      Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::SOCKET),
      dbusServer.getConnection());
  }

  void requestBusName(Server& server, const std::string& name) override
  {
    if(name.empty())
    {
      return;
    }
    auto& dbusServer = static_cast<DbusIpcServer&>(server);
    DBus::requestBusName(dbusServer.getConnection(), name);
  }

  void releaseBusName(Server& server, const std::string& name) override
  {
    if(name.empty())
    {
      return;
    }
    auto& dbusServer = static_cast<DbusIpcServer&>(server);
    DBus::releaseBusName(dbusServer.getConnection(), name);
  }
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_TRANSPORT_FACTORY_H
