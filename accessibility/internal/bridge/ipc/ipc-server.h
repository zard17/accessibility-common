#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_SERVER_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_SERVER_H

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
#include <string>
#include <variant>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/types.h>

namespace Ipc
{
class InterfaceDescription; // forward

/**
 * @brief Protocol-neutral signal payload type.
 *
 * Covers all variant types used in AT-SPI event signals:
 * int (StateChanged, WindowEvent, etc.), string (TextChanged),
 * Address (ActiveDescendantChanged), Rect<int> (BoundsChanged).
 */
using SignalVariant = std::variant<
  int,
  std::string,
  Accessibility::Address,
  Accessibility::Rect<int>
>;

/**
 * @brief Abstract server-side IPC interface.
 *
 * The bridge registers method handlers, properties, and signals via this interface.
 * Each IPC backend (D-Bus, TIDL) provides a concrete implementation.
 */
class Server
{
public:
  virtual ~Server() = default;

  /**
   * @brief Registers an accessibility interface at the given path.
   *
   * @param[in] pathName Object path to register the interface on
   * @param[in] desc Interface description (methods, properties, signals)
   * @param[in] fallback If true, this registration handles all sub-paths
   */
  virtual void addInterface(const std::string& pathName,
                            InterfaceDescription& desc,
                            bool fallback = false) = 0;

  /**
   * @brief Returns the bus/connection name for this server.
   */
  virtual std::string getBusName() const = 0;

  /**
   * @brief Returns the current IPC object path being handled.
   *
   * Callable from within method/property callbacks to determine
   * which object the request targets.
   */
  virtual std::string getCurrentObjectPath() const = 0;

  /**
   * @brief Emits an accessibility signal (AT-SPI event pattern).
   *
   * This is the protocol-neutral replacement for D-Bus emit2<>().
   * Each IPC backend maps the parameters to its native signal mechanism.
   *
   * @param[in] objectPath   The object emitting the signal
   * @param[in] interfaceName The interface the signal belongs to
   * @param[in] signalName   The signal name (e.g. "StateChanged")
   * @param[in] detail       Event detail string
   * @param[in] detail1      First integer detail
   * @param[in] detail2      Second integer detail
   * @param[in] data         Signal payload (variant)
   * @param[in] sender       Address of the sender
   */
  virtual void emitSignal(const std::string&             objectPath,
                          const std::string&             interfaceName,
                          const std::string&             signalName,
                          const std::string&             detail,
                          int                            detail1,
                          int                            detail2,
                          const SignalVariant&            data,
                          const Accessibility::Address&  sender) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_SERVER_H
