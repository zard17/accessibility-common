#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_INTERFACE_DESCRIPTION_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_INTERFACE_DESCRIPTION_H

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

namespace Ipc
{
/**
 * @brief Abstract interface description for registering methods, properties, and signals.
 *
 * Bridge modules populate an InterfaceDescription with their handlers, then
 * pass it to Server::addInterface(). Each IPC backend provides a concrete
 * subclass that knows how to serialize the registered handlers into its
 * native format.
 *
 * The addMethod<T>, addProperty<T>, and addSignal<ARGS...> template methods
 * are provided by the concrete backend class (e.g. DBus::DBusInterfaceDescription)
 * since they depend on backend-specific serialization.
 */
class InterfaceDescription
{
public:
  virtual ~InterfaceDescription() = default;

  /**
   * @brief Returns the interface name.
   */
  const std::string& getInterfaceName() const
  {
    return mInterfaceName;
  }

protected:
  explicit InterfaceDescription(std::string interfaceName)
  : mInterfaceName(std::move(interfaceName))
  {
  }

  std::string mInterfaceName;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_INTERFACE_DESCRIPTION_H
