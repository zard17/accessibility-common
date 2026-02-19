#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_IPC_SERVER_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_IPC_SERVER_H

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
#include <map>
#include <memory>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-server.h>
#include <accessibility/internal/bridge/tidl/tidl-interface-description.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of the IPC server interface.
 *
 * Wraps a TIDL-generated stub for direct P2P communication via rpc_port.
 * In the scaffold stage, this is a stub implementation that stores interface
 * descriptions and tracks the current object path but does not perform
 * actual IPC. The real implementation will use tidlc-generated code.
 *
 * Design:
 *   - addInterface() stores TidlInterfaceDescription objects in a map
 *     keyed by (path, interfaceName). Unlike D-Bus (which registers with
 *     the daemon), TIDL stores them locally and dispatches incoming calls.
 *   - getCurrentObjectPath() returns the objectPath parameter from the
 *     current TIDL method invocation (stored during dispatch).
 *   - emitSignal() will call registered TIDL delegate callbacks.
 *   - createInterfaceDescription() returns a TidlInterfaceDescription.
 */
class TidlIpcServer : public Server
{
public:
  /**
   * @brief Constructs a TIDL IPC server.
   *
   * @param[in] appId Application ID used as the TIDL service identifier
   */
  explicit TidlIpcServer(std::string appId);

  ~TidlIpcServer() override;

  TidlIpcServer(const TidlIpcServer&)            = delete;
  TidlIpcServer& operator=(const TidlIpcServer&) = delete;
  TidlIpcServer(TidlIpcServer&&)                  = default;
  TidlIpcServer& operator=(TidlIpcServer&&)       = default;

  // Ipc::Server interface

  void addInterface(const std::string& pathName,
                    InterfaceDescription& desc,
                    bool fallback = false) override;

  std::string getBusName() const override;

  std::string getCurrentObjectPath() const override;

  void emitSignal(const std::string&             objectPath,
                  const std::string&             interfaceName,
                  const std::string&             signalName,
                  const std::string&             detail,
                  int                            detail1,
                  int                            detail2,
                  const SignalVariant&            data,
                  const Accessibility::Address&  sender) override;

  std::unique_ptr<InterfaceDescription> createInterfaceDescription(const std::string& interfaceName) override;

private:
  /**
   * @brief Key for interface lookup: (objectPath, interfaceName).
   */
  struct InterfaceKey
  {
    std::string path;
    std::string interfaceName;

    bool operator<(const InterfaceKey& other) const
    {
      if(path != other.path)
      {
        return path < other.path;
      }
      return interfaceName < other.interfaceName;
    }
  };

  std::string mAppId;            ///< Application/service identifier
  std::string mCurrentObjectPath; ///< Object path during dispatch

  /// Stored interface descriptions for dispatch
  std::map<InterfaceKey, std::unique_ptr<Tidl::TidlInterfaceDescription>> mInterfaces;

  /// Fallback interfaces (registered with fallback=true)
  std::vector<std::unique_ptr<Tidl::TidlInterfaceDescription>> mFallbackInterfaces;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_IPC_SERVER_H
