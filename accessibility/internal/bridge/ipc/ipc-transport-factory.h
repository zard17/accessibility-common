#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_TRANSPORT_FACTORY_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_TRANSPORT_FACTORY_H

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
#include <accessibility/api/accessibility.h>
#include <accessibility/internal/bridge/ipc/ipc-direct-reading-client.h>
#include <accessibility/internal/bridge/ipc/ipc-key-event-forwarder.h>
#include <accessibility/internal/bridge/ipc/ipc-registry-client.h>
#include <accessibility/internal/bridge/ipc/ipc-result.h>
#include <accessibility/internal/bridge/ipc/ipc-server.h>
#include <accessibility/internal/bridge/ipc/ipc-socket-client.h>
#include <accessibility/internal/bridge/ipc/ipc-status-monitor.h>

namespace Ipc
{
/**
 * @brief Result of a successful transport connection.
 */
struct ConnectionResult
{
  std::unique_ptr<Server> server;  ///< IPC server instance
  std::string             busName; ///< Bus/connection name
};

/**
 * @brief Abstract factory for creating IPC transport components.
 *
 * Each IPC backend (D-Bus, TIDL, in-process) provides a concrete
 * implementation that creates the appropriate server and client instances.
 */
class TransportFactory
{
public:
  virtual ~TransportFactory() = default;

  /**
   * @brief Returns true if the transport layer is available.
   *
   * For D-Bus, this checks if DBusWrapper is installed.
   */
  virtual bool isAvailable() const = 0;

  /**
   * @brief Establishes the IPC connection and creates a server.
   *
   * For D-Bus, this calls GetAddress on the a11y bus, opens the
   * connection, and wraps it in a DbusIpcServer.
   *
   * @return ConnectionResult on success, or error
   */
  virtual ValueOrError<ConnectionResult> connect() = 0;

  /**
   * @brief Creates an accessibility status monitor client.
   *
   * Used to read and listen for IsEnabled/ScreenReaderEnabled properties.
   *
   * @return Status monitor instance, or nullptr if transport unavailable
   */
  virtual std::unique_ptr<AccessibilityStatusMonitor> createStatusMonitor() = 0;

  /**
   * @brief Creates a key event forwarder client.
   *
   * Used to forward key events to the AT registry.
   *
   * @param[in] server The IPC server (for connection sharing)
   * @return Key event forwarder instance
   */
  virtual std::unique_ptr<KeyEventForwarder> createKeyEventForwarder(Server& server) = 0;

  /**
   * @brief Creates a direct reading (TTS) client.
   *
   * @param[in] server The IPC server (for connection sharing)
   * @return Direct reading client instance
   */
  virtual std::unique_ptr<DirectReadingClient> createDirectReadingClient(Server& server) = 0;

  /**
   * @brief Creates a registry client.
   *
   * Used to query registered events and listen for listener changes.
   *
   * @param[in] server The IPC server (for connection sharing)
   * @return Registry client instance
   */
  virtual std::unique_ptr<RegistryClient> createRegistryClient(Server& server) = 0;

  /**
   * @brief Creates a socket client for the given remote address.
   *
   * Used for Embed/Unembed/SetOffset operations.
   *
   * @param[in] address Remote socket address
   * @param[in] server The IPC server (for connection sharing)
   * @return Socket client instance
   */
  virtual std::unique_ptr<SocketClient> createSocketClient(const Accessibility::Address& address,
                                                           Server&                       server) = 0;

  /**
   * @brief Requests ownership of a bus name.
   *
   * @param[in] server The IPC server
   * @param[in] name Bus name to request
   */
  virtual void requestBusName(Server& server, const std::string& name) = 0;

  /**
   * @brief Releases ownership of a bus name.
   *
   * @param[in] server The IPC server
   * @param[in] name Bus name to release
   */
  virtual void releaseBusName(Server& server, const std::string& name) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_TRANSPORT_FACTORY_H
