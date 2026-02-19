#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_TRANSPORT_FACTORY_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_TRANSPORT_FACTORY_H

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

// INTERNAL INCLUDES
#include <accessibility/api/log.h>
#include <accessibility/internal/bridge/ipc/ipc-transport-factory.h>
#include <accessibility/internal/bridge/tidl/tidl-direct-reading-client.h>
#include <accessibility/internal/bridge/tidl/tidl-ipc-server.h>
#include <accessibility/internal/bridge/tidl/tidl-key-event-forwarder.h>
#include <accessibility/internal/bridge/tidl/tidl-registry-client.h>
#include <accessibility/internal/bridge/tidl/tidl-socket-client.h>
#include <accessibility/internal/bridge/tidl/tidl-status-monitor.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of TransportFactory.
 *
 * Creates TIDL-based IPC components that use rpc_port for direct P2P
 * communication instead of a D-Bus daemon. Only available on Tizen.
 *
 * In the scaffold stage, this creates stub implementations that compile
 * but do not perform actual TIDL IPC. The real implementation will use
 * tidlc-generated stub/proxy code.
 */
class TidlTransportFactory : public TransportFactory
{
public:
  TidlTransportFactory() = default;
  ~TidlTransportFactory() override = default;

  bool isAvailable() const override
  {
    // TIDL is always available on Tizen (rpc_port is a platform service).
    // In scaffold mode, return true so the bridge lifecycle proceeds.
    return true;
  }

  ValueOrError<ConnectionResult> connect() override
  {
    // Get the application ID (TIDL uses app ID instead of D-Bus bus name)
    std::string appId = getAppId();

    auto server = std::make_unique<TidlIpcServer>(appId);

    ConnectionResult result;
    result.server  = std::move(server);
    result.busName = std::move(appId);
    return result;
  }

  std::unique_ptr<AccessibilityStatusMonitor> createStatusMonitor() override
  {
    return std::make_unique<TidlStatusMonitor>(
      "org.tizen.accessibility", "accessibility_status");
  }

  std::unique_ptr<KeyEventForwarder> createKeyEventForwarder(Server& server) override
  {
    return std::make_unique<TidlKeyEventForwarder>();
  }

  std::unique_ptr<DirectReadingClient> createDirectReadingClient(Server& server) override
  {
    return std::make_unique<TidlDirectReadingClient>();
  }

  std::unique_ptr<RegistryClient> createRegistryClient(Server& server) override
  {
    return std::make_unique<TidlRegistryClient>();
  }

  std::unique_ptr<SocketClient> createSocketClient(const Accessibility::Address& address,
                                                   Server&                       server) override
  {
    return std::make_unique<TidlSocketClient>(address);
  }

  void requestBusName(Server& server, const std::string& name) override
  {
    // TIDL doesn't have bus name ownership like D-Bus.
    // The app ID is the implicit service identifier.
    if(!name.empty())
    {
      ACCESSIBILITY_LOG_DEBUG_INFO("TidlTransportFactory::requestBusName(%s) - no-op for TIDL\n", name.c_str());
    }
  }

  void releaseBusName(Server& server, const std::string& name) override
  {
    // TIDL doesn't have bus name ownership.
    if(!name.empty())
    {
      ACCESSIBILITY_LOG_DEBUG_INFO("TidlTransportFactory::releaseBusName(%s) - no-op for TIDL\n", name.c_str());
    }
  }

private:
  /**
   * @brief Returns the application ID for TIDL service identification.
   *
   * On Tizen, this would use app_get_id() from the application framework.
   * In scaffold mode, returns a placeholder.
   */
  static std::string getAppId()
  {
    // Real implementation: use Tizen app_get_id() API
    // Scaffold: return placeholder
    return "org.tizen.accessibility.bridge";
  }
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_TRANSPORT_FACTORY_H
