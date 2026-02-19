#ifndef ACCESSIBILITY_API_APP_REGISTRY_H
#define ACCESSIBILITY_API_APP_REGISTRY_H

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
#include <functional>
#include <memory>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/node-proxy.h>

namespace Accessibility
{
/**
 * @brief Callback type for application registration/deregistration events.
 */
using AppCallback = std::function<void(const Address&)>;

/**
 * @brief Abstract interface for discovering accessible applications.
 *
 * Concrete implementations: AtSpiAppRegistry (D-Bus), TidlAppRegistry (Tizen aul).
 * CompositeAppRegistry merges multiple registries.
 */
class AppRegistry
{
public:
  virtual ~AppRegistry() = default;

  /**
   * @brief Gets the desktop node (root of all application trees).
   */
  virtual std::shared_ptr<NodeProxy> getDesktop() = 0;

  /**
   * @brief Gets the currently active (focused) window node.
   */
  virtual std::shared_ptr<NodeProxy> getActiveWindow() = 0;

  /**
   * @brief Registers a callback for when a new app becomes available.
   */
  virtual void onAppRegistered(AppCallback callback) = 0;

  /**
   * @brief Registers a callback for when an app is removed.
   */
  virtual void onAppDeregistered(AppCallback callback) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_APP_REGISTRY_H
