#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_STATUS_MONITOR_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_STATUS_MONITOR_H

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
#include <accessibility/internal/bridge/ipc/ipc-status-monitor.h>

namespace Ipc
{
/**
 * @brief D-Bus implementation of AccessibilityStatusMonitor.
 *
 * Wraps a DBus::DBusClient connected to the accessibility status service.
 */
class DbusStatusMonitor : public AccessibilityStatusMonitor
{
public:
  /**
   * @brief Constructs a status monitor connected via SESSION bus.
   *
   * @param[in] busName D-Bus bus name (e.g. "org.a11y.Bus")
   * @param[in] path Object path (e.g. "/org/a11y/bus")
   * @param[in] interface Interface name (e.g. "org.a11y.Status")
   */
  DbusStatusMonitor(std::string busName, std::string path, std::string interface)
  : mClient(std::move(busName), std::move(path), std::move(interface), DBus::ConnectionType::SESSION)
  {
  }

  ~DbusStatusMonitor() override = default;

  bool isConnected() const override
  {
    return bool(mClient);
  }

  void readIsEnabled(std::function<void(ValueOrError<bool>)> callback) override
  {
    mClient.property<bool>("IsEnabled").asyncGet(std::move(callback));
  }

  void listenIsEnabled(std::function<void(bool)> callback) override
  {
    mClient.addPropertyChangedEvent<bool>("IsEnabled", std::move(callback));
  }

  void readScreenReaderEnabled(std::function<void(ValueOrError<bool>)> callback) override
  {
    mClient.property<bool>("ScreenReaderEnabled").asyncGet(std::move(callback));
  }

  void listenScreenReaderEnabled(std::function<void(bool)> callback) override
  {
    mClient.addPropertyChangedEvent<bool>("ScreenReaderEnabled", std::move(callback));
  }

private:
  DBus::DBusClient mClient;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_DBUS_STATUS_MONITOR_H
