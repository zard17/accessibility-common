#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_INTERFACE_DESCRIPTION_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_INTERFACE_DESCRIPTION_H

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
#include <any>
#include <functional>
#include <map>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-interface-description.h>
#include <accessibility/internal/bridge/ipc/ipc-result.h>

namespace Ipc
{
namespace Tidl
{
/**
 * @brief Signal identifier for TIDL signals.
 *
 * Mirrors DBusWrapper::SignalId to maintain API compatibility with
 * bridge modules that store signal IDs.
 */
struct SignalId
{
  int id{0};

  SignalId() = default;
  explicit SignalId(int signalId)
  : id(signalId)
  {
  }
};

namespace detail
{
/**
 * @brief Traits for extracting function signature components.
 *
 * Used to decompose addMethod<RET(ARGS...)> into return/argument types.
 */
template<typename T>
struct tidl_interface_traits;

template<typename RET, typename... ARGS>
struct tidl_interface_traits<RET(ARGS...)>
{
  using ReturnType = RET;
  using SyncCB     = std::function<RET(ARGS...)>;
};

} // namespace detail

/**
 * @brief TIDL implementation of InterfaceDescription.
 *
 * Provides the same addMethod<T>, addProperty<T>, and addSignal<ARGS...>
 * template API as DBus::DBusInterfaceDescription. Instead of creating
 * D-Bus-specific method/property/signal descriptors, this class stores
 * type-erased callbacks in maps that the TidlIpcServer dispatches to.
 *
 * Bridge modules call addMethod<ValueOrError<RET...>(ARGS...)>(name, callback)
 * through the AddFunctionToInterface helper, which static_casts the abstract
 * InterfaceDescription& to the concrete backend type. For TIDL, the cast
 * target is TidlInterfaceDescription.
 */
class TidlInterfaceDescription : public InterfaceDescription
{
public:
  /**
   * @brief Type-erased method handler.
   *
   * Stores the callback as std::any. The TidlIpcServer dispatch logic
   * knows the expected signature from the TIDL stub and can std::any_cast
   * to the correct std::function type.
   */
  struct MethodHandler
  {
    std::string name;
    std::any    callback; ///< std::function<RET(ARGS...)> stored as any
  };

  /**
   * @brief Type-erased property handler.
   */
  struct PropertyHandler
  {
    std::string name;
    std::any    getter; ///< std::function<ValueOrError<T>()> stored as any
    std::any    setter; ///< std::function<ValueOrError<void>(T)> stored as any
  };

  /**
   * @brief Signal registration entry.
   */
  struct SignalEntry
  {
    std::string name;
    SignalId    id;
  };

  /**
   * @brief Creates an interface description with the given name.
   *
   * @param[in] interfaceName AT-SPI interface name
   */
  explicit TidlInterfaceDescription(std::string interfaceName)
  : InterfaceDescription(std::move(interfaceName))
  {
  }

  ~TidlInterfaceDescription() override = default;

  /**
   * @brief Adds a synchronous method handler.
   *
   * Template type T defines the function signature: RET(ARGS...).
   * This mirrors DBus::DBusInterfaceDescription::addMethod<T>().
   *
   * @param[in] memberName Method name
   * @param[in] callback Method implementation
   */
  template<typename T>
  void addMethod(const std::string& memberName, typename detail::tidl_interface_traits<T>::SyncCB callback)
  {
    MethodHandler handler;
    handler.name     = memberName;
    handler.callback = std::move(callback);
    mMethods[memberName] = std::move(handler);
  }

  /**
   * @brief Adds a property with optional getter and setter.
   *
   * This mirrors DBus::DBusInterfaceDescription::addProperty<T>().
   *
   * @param[in] memberName Property name
   * @param[in] getter Getter callback (may be empty)
   * @param[in] setter Setter callback (may be empty)
   */
  template<typename T>
  void addProperty(const std::string& memberName, std::function<ValueOrError<T>()> getter, std::function<ValueOrError<void>(T)> setter)
  {
    PropertyHandler handler;
    handler.name = memberName;
    if(getter)
    {
      handler.getter = std::move(getter);
    }
    if(setter)
    {
      handler.setter = std::move(setter);
    }
    mProperties[memberName] = std::move(handler);
  }

  /**
   * @brief Registers a signal definition.
   *
   * This mirrors DBus::DBusInterfaceDescription::addSignal<ARGS...>().
   *
   * @param[in] memberName Signal name
   * @return Signal identifier for later emission
   */
  template<typename... ARGS>
  SignalId addSignal(const std::string& memberName)
  {
    static int nextSignalId = 1;
    SignalId   signalId{nextSignalId++};

    SignalEntry entry;
    entry.name = memberName;
    entry.id   = signalId;
    mSignals.push_back(std::move(entry));

    return signalId;
  }

  /**
   * @brief Returns the registered method handlers.
   */
  const std::map<std::string, MethodHandler>& getMethods() const
  {
    return mMethods;
  }

  /**
   * @brief Returns the registered property handlers.
   */
  const std::map<std::string, PropertyHandler>& getProperties() const
  {
    return mProperties;
  }

  /**
   * @brief Returns the registered signal entries.
   */
  const std::vector<SignalEntry>& getSignals() const
  {
    return mSignals;
  }

private:
  std::map<std::string, MethodHandler>   mMethods;
  std::map<std::string, PropertyHandler> mProperties;
  std::vector<SignalEntry>               mSignals;
};

} // namespace Tidl
} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_INTERFACE_DESCRIPTION_H
