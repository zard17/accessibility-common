#ifndef ACCESSIBILITY_INTERNAL_ATSPI_ACCESSIBILITY_COMMON_H
#define ACCESSIBILITY_INTERNAL_ATSPI_ACCESSIBILITY_COMMON_H

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
#include <iomanip>
#include <sstream>
#include <string>

// LOGGING
#include <accessibility/api/log.h>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/internal/bridge/dbus/dbus-locators.h>
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <accessibility/public-api/accessibility-common.h>

// DBus names

#define A11yDbusName "org.a11y.Bus"
#define A11yDbusStatusInterface "org.a11y.Status"
#define AtspiDbusNameRegistry "org.a11y.atspi.Registry"
#define DirectReadingDBusName "org.tizen.ScreenReader"
#define DirectReadingDBusInterface "org.tizen.DirectReading"

// DBus paths

#define A11yDbusPath "/org/a11y/bus"
#define AtspiDbusPathCache "/org/a11y/atspi/cache"
#define AtspiDbusPathDec "/org/a11y/atspi/registry/deviceeventcontroller"
#define AtspiDbusPathRegistry "/org/a11y/atspi/registry"
#define AtspiDbusPathRoot "/org/a11y/atspi/accessible/root"
#define AtspiPath "/org/a11y/atspi/accessible"
#define DirectReadingDBusPath "/org/tizen/DirectReading"

struct ObjectPath;

/**
 * @brief Enumeration used for quering Accessibility objects
 */
enum class MatchType : int32_t
{
  INVALID,
  ALL,
  ANY,
  NONE,
  EMPTY
};

/**
 * @brief Enumeration used for quering Accessibility objects
 * SortOrder::Canonical uses breadth-first search and sort objects in order of indexes in parent
 * SortOrder::ReverseCanonical uses SortOrder::Canonical and reverse collection
 * The rest of orders is not supported.
 */
enum class SortOrder : uint32_t
{
  INVALID,
  CANONICAL,
  FLOW,
  TAB,
  REVERSE_CANONICAL,
  REVERSE_FLOW,
  REVERSE_TAB,
  LAST_DEFINED
};

namespace DBus
{
/**
 * @brief The CurrentBridgePtr class is to save the current Accessibility Bridge.
 */
class CurrentBridgePtr
{
  static Accessibility::Bridge*& Get()
  {
    static thread_local Accessibility::Bridge* bridge = nullptr;
    return bridge;
  }
  Accessibility::Bridge* mPrev;
  CurrentBridgePtr(const CurrentBridgePtr&)            = delete;
  CurrentBridgePtr(CurrentBridgePtr&&)                 = delete;
  CurrentBridgePtr& operator=(const CurrentBridgePtr&) = delete;
  CurrentBridgePtr& operator=(CurrentBridgePtr&&)      = delete;

public:
  CurrentBridgePtr(Accessibility::Bridge* bridge)
  : mPrev(Get())
  {
    Get() = bridge;
  }

  ~CurrentBridgePtr()
  {
    Get() = mPrev;
  }

  static Accessibility::Bridge* GetCurrentBridge()
  {
    return Get();
  }
}; // CurrentBridgePtr

// Templates for setting and getting Accessible values
namespace detail
{
template<>
struct signature<Accessibility::Address> : signature_helper<signature<Accessibility::Address>>
{
  using subtype = std::pair<std::string, ObjectPath>;

  static constexpr auto name_v = concat("AtspiAccessiblePtr");
  static constexpr auto sig_v  = signature<subtype>::sig_v; // "(so)"

  /**
   * @brief Marshals value address as marshalled type into message
   */
  static void set(const DBusWrapper::MessageIterPtr& iter, const Accessibility::Address& address)
  {
    if(address)
    {
      signature<subtype>::set(iter, {address.GetBus(), ObjectPath{std::string{ATSPI_PREFIX_PATH} + address.GetPath()}});
    }
    else
    {
      signature<subtype>::set(iter, {address.GetBus(), ObjectPath{ATSPI_NULL_PATH}});
    }
  }

  /**
   * @brief Marshals value from marshalled type into variable address
   */
  static bool get(const DBusWrapper::MessageIterPtr& iter, Accessibility::Address& address)
  {
    subtype tmp;
    if(!signature<subtype>::get(iter, tmp))
    {
      return false;
    }

    if(tmp.second.value == ATSPI_NULL_PATH)
    {
      address = {};
      return true;
    }
    if(tmp.second.value.substr(0, strlen(ATSPI_PREFIX_PATH)) != ATSPI_PREFIX_PATH)
    {
      return false;
    }

    address = {std::move(tmp.first), tmp.second.value.substr(strlen(ATSPI_PREFIX_PATH))};
    return true;
  }
};

template<typename T>
struct SignatureAccessibleImpl : signature_helper<SignatureAccessibleImpl<T>>
{
  using subtype = Accessibility::Address;

  static constexpr auto name_v = signature<subtype>::name_v;
  static constexpr auto sig_v  = signature<subtype>::sig_v;

  /**
   * @brief Marshals value address as marshalled type into message
   */
  static void set(const DBusWrapper::MessageIterPtr& iter, T* accessible)
  {
    signature<subtype>::set(iter, accessible ? accessible->GetAddress() : subtype{});
  }

  /**
   * @brief Marshals value from marshalled type into variable path
   */
  static bool get(const DBusWrapper::MessageIterPtr& iter, T*& path)
  {
    subtype address;

    signature<subtype>::get(iter, address);

    auto currentBridge = CurrentBridgePtr::GetCurrentBridge();
    if(!currentBridge || currentBridge->GetBusName() != address.GetBus())
    {
      return false;
    }

    path = currentBridge->FindByPath(address.GetPath());
    return path != nullptr;
  }
};

template<>
struct signature<Accessibility::Accessible*> : public SignatureAccessibleImpl<Accessibility::Accessible>
{
};

template<>
struct signature<Accessibility::States> : signature_helper<signature<Accessibility::States>>
{
  using subtype = std::array<uint32_t, 2>;

  static constexpr auto name_v = signature<subtype>::name_v;
  static constexpr auto sig_v  = signature<subtype>::sig_v;

  /**
   * @brief Marshals value state as marshalled type into message
   */
  static void set(const DBusWrapper::MessageIterPtr& iter, const Accessibility::States& states)
  {
    signature<subtype>::set(iter, states.GetRawData());
  }

  /**
   * @brief Marshals value from marshalled type into variable state
   */
  static bool get(const DBusWrapper::MessageIterPtr& iter, Accessibility::States& state)
  {
    subtype tmp;
    if(!signature<subtype>::get(iter, tmp))
    {
      return false;
    }
    state = Accessibility::States{tmp};
    return true;
  }
};
} // namespace detail
} // namespace DBus

struct _Logger
{
  const char*        mFile;
  int                mLine;
  std::ostringstream mTmp;

  _Logger(const char* file, int line)
  : mFile(file),
    mLine(line)
  {
  }

  ~_Logger()
  {
    Accessibility::LogMessage(Accessibility::LogLevel::INFO, "%s:%d: %s", mFile, mLine, mTmp.str().c_str());
  }

  template<typename T>
  _Logger& operator<<(T&& t)
  {
    mTmp << std::forward<T>(t);
    return *this;
  }
};

struct _LoggerEmpty
{
  template<typename T>
  _LoggerEmpty& operator<<(T&& t)
  {
    return *this;
  }
};

struct _LoggerScope
{
  const char* mFile;
  int         mLine;

  _LoggerScope(const char* file, int line)
  : mFile(file),
    mLine(line)
  {
    Accessibility::LogMessage(Accessibility::LogLevel::INFO, "%s:%d: +", mFile, mLine);
  }

  ~_LoggerScope()
  {
    Accessibility::LogMessage(Accessibility::LogLevel::INFO, "%s:%d: -", mFile, mLine);
  }
};

#define LOG() _Logger(__FILE__, __LINE__)
#define SCOPE() _LoggerScope _l##__LINE__(__FILE__, __LINE__)

#endif // ACCESSIBILITY_INTERNAL_ATSPI_ACCESSIBILITY_COMMON_H
