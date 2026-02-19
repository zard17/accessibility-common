#ifndef ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_APP_REGISTRY_H
#define ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_APP_REGISTRY_H

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
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/app-registry.h>
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/internal/service/atspi-node-proxy.h>

namespace Accessibility
{
/**
 * @brief D-Bus implementation of AppRegistry.
 *
 * Connects to org.a11y.atspi.Registry on the accessibility bus.
 * Uses GetDesktop() and listens for AddAccessible/RemoveAccessible signals.
 */
class AtSpiAppRegistry : public AppRegistry
{
public:
  /**
   * @brief Constructs an AtSpiAppRegistry.
   *
   * @param[in] connection The D-Bus connection to the accessibility bus
   */
  explicit AtSpiAppRegistry(DBusWrapper::ConnectionPtr connection);

  std::shared_ptr<NodeProxy> getDesktop() override;
  std::shared_ptr<NodeProxy> getActiveWindow() override;
  void onAppRegistered(AppCallback callback) override;
  void onAppDeregistered(AppCallback callback) override;

  /**
   * @brief Creates a NodeProxy for the given address using D-Bus transport.
   */
  std::shared_ptr<NodeProxy> createNodeProxy(const Address& address);

private:
  DBusWrapper::ConnectionPtr           mConnection;
  std::vector<AppCallback>             mRegisteredCallbacks;
  std::vector<AppCallback>             mDeregisteredCallbacks;
  std::shared_ptr<NodeProxy>           mDesktop;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_APP_REGISTRY_H
