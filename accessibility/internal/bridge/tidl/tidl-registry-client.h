#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_REGISTRY_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_REGISTRY_CLIENT_H

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

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-registry-client.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of RegistryClient.
 *
 * Uses TIDL proxy to communicate with the AT-SPI registry via
 * rpc_port direct P2P connection instead of D-Bus.
 *
 * Scaffold: Returns empty event list and stores listener callbacks.
 */
class TidlRegistryClient : public RegistryClient
{
public:
  TidlRegistryClient() = default;

  ~TidlRegistryClient() override = default;

  void getRegisteredEvents(std::function<void(ValueOrError<RegisteredEventsType>)> callback) override
  {
    // Scaffold: return empty list (no real TIDL proxy yet)
    // Real implementation: mProxy->GetRegisteredEvents()
    if(callback)
    {
      callback(RegisteredEventsType{});
    }
  }

  void listenEventListenerRegistered(std::function<void()> callback) override
  {
    // Scaffold: store callback
    // Real implementation: mProxy->setOnEventListenerRegistered(callback)
    mRegisteredCallback = std::move(callback);
  }

  void listenEventListenerDeregistered(std::function<void()> callback) override
  {
    // Scaffold: store callback
    // Real implementation: mProxy->setOnEventListenerDeregistered(callback)
    mDeregisteredCallback = std::move(callback);
  }

private:
  std::function<void()> mRegisteredCallback;
  std::function<void()> mDeregisteredCallback;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_REGISTRY_CLIENT_H
