#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_REGISTRY_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_REGISTRY_CLIENT_H

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
#include <string>
#include <tuple>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-result.h>

namespace Ipc
{
/**
 * @brief Abstract interface for the AT-SPI registry client.
 *
 * Replaces direct DBus::DBusClient usage for GetRegisteredEvents
 * and listening to EventListenerRegistered/Deregistered signals
 * on the AT-SPI registry bus.
 */
class RegistryClient
{
public:
  using RegisteredEventsType = std::vector<std::tuple<std::string, std::string>>;

  virtual ~RegistryClient() = default;

  /**
   * @brief Asynchronously retrieves the list of registered AT-SPI events.
   *
   * @param[in] callback Called with the list of (bus, event) pairs or error
   */
  virtual void getRegisteredEvents(std::function<void(ValueOrError<RegisteredEventsType>)> callback) = 0;

  /**
   * @brief Listens for new event listener registrations.
   *
   * @param[in] callback Called when an event listener is registered
   */
  virtual void listenEventListenerRegistered(std::function<void()> callback) = 0;

  /**
   * @brief Listens for event listener deregistrations.
   *
   * @param[in] callback Called when an event listener is deregistered
   */
  virtual void listenEventListenerDeregistered(std::function<void()> callback) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_REGISTRY_CLIENT_H
