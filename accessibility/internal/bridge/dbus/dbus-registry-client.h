#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_REGISTRY_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_REGISTRY_CLIENT_H

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
#include <accessibility/internal/bridge/ipc/ipc-registry-client.h>

namespace Ipc
{
/**
 * @brief D-Bus implementation of RegistryClient.
 *
 * Wraps a DBus::DBusClient connected to the AT-SPI registry.
 */
class DbusRegistryClient : public RegistryClient
{
public:
  /**
   * @brief Constructs a registry client.
   *
   * @param[in] busName D-Bus bus name
   * @param[in] path Object path
   * @param[in] interface Interface name
   * @param[in] conn Existing D-Bus connection
   */
  DbusRegistryClient(std::string busName, std::string path, std::string interface,
                     const DBusWrapper::ConnectionPtr& conn)
  : mClient(std::move(busName), std::move(path), std::move(interface), conn)
  {
  }

  ~DbusRegistryClient() override = default;

  void getRegisteredEvents(std::function<void(ValueOrError<RegisteredEventsType>)> callback) override
  {
    mClient.method<DBus::ValueOrError<RegisteredEventsType>()>("GetRegisteredEvents")
      .asyncCall(std::move(callback));
  }

  void listenEventListenerRegistered(std::function<void()> callback) override
  {
    mClient.addSignal<void(void)>("EventListenerRegistered", std::move(callback));
  }

  void listenEventListenerDeregistered(std::function<void()> callback) override
  {
    mClient.addSignal<void(void)>("EventListenerDeregistered", std::move(callback));
  }

private:
  DBus::DBusClient mClient;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_REGISTRY_CLIENT_H
