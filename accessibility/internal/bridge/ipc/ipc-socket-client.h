#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_SOCKET_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_SOCKET_CLIENT_H

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
#include <cstdint>
#include <functional>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/internal/bridge/ipc/ipc-result.h>

namespace Ipc
{
/**
 * @brief Abstract interface for AT-SPI socket operations.
 *
 * Replaces direct DBus::DBusClient creation for Embed, Unembed,
 * and SetOffset operations on remote accessible sockets.
 */
class SocketClient
{
public:
  virtual ~SocketClient() = default;

  /**
   * @brief Embeds a plug into the socket (synchronous).
   *
   * @param[in] plug Address of the plug to embed
   * @return The parent address, or error
   */
  virtual ValueOrError<Accessibility::Address> embed(Accessibility::Address plug) = 0;

  /**
   * @brief Unembeds a plug from the socket (asynchronous).
   *
   * @param[in] plug Address of the plug to unembed
   * @param[in] callback Called on completion
   */
  virtual void unembed(Accessibility::Address                   plug,
                       std::function<void(ValueOrError<void>)>  callback) = 0;

  /**
   * @brief Sets the coordinate offset for the socket (asynchronous).
   *
   * @param[in] x X offset
   * @param[in] y Y offset
   * @param[in] callback Called on completion
   */
  virtual void setOffset(std::int32_t                            x,
                         std::int32_t                            y,
                         std::function<void(ValueOrError<void>)> callback) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_SOCKET_CLIENT_H
