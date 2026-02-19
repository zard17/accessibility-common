#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_SOCKET_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_SOCKET_CLIENT_H

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
#include <accessibility/internal/bridge/ipc/ipc-socket-client.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of SocketClient.
 *
 * Uses TIDL proxy for Embed/Unembed/SetOffset operations via
 * rpc_port direct P2P connection instead of D-Bus.
 *
 * Scaffold: Returns errors for all operations.
 */
class TidlSocketClient : public SocketClient
{
public:
  /**
   * @brief Constructs a TIDL socket client.
   *
   * @param[in] address Remote socket address
   */
  explicit TidlSocketClient(Accessibility::Address address)
  : mAddress(std::move(address))
  {
  }

  ~TidlSocketClient() override = default;

  ValueOrError<Accessibility::Address> embed(Accessibility::Address plug) override
  {
    // Scaffold: return error (no real TIDL proxy yet)
    // Real implementation: mProxy->Embed(objectPath, plug_bundle)
    return Error{"TIDL socket embed not yet implemented"};
  }

  void unembed(Accessibility::Address                  plug,
               std::function<void(ValueOrError<void>)> callback) override
  {
    // Scaffold: return error
    // Real implementation: mProxy->Unembed(objectPath, plug_bundle)
    if(callback)
    {
      callback(Error{"TIDL socket unembed not yet implemented"});
    }
  }

  void setOffset(std::int32_t                            x,
                 std::int32_t                            y,
                 std::function<void(ValueOrError<void>)> callback) override
  {
    // Scaffold: return error
    // Real implementation: mProxy->SetOffset(objectPath, x, y)
    if(callback)
    {
      callback(Error{"TIDL socket setOffset not yet implemented"});
    }
  }

private:
  Accessibility::Address mAddress;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_SOCKET_CLIENT_H
