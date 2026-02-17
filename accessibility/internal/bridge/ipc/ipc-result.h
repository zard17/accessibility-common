#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_RESULT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_RESULT_H

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
#include <cassert>
#include <string>
#include <tuple>

/**
 * @brief Protocol-neutral IPC result types.
 *
 * These types represent the outcome of an IPC operation: either a set of
 * values or an error message. They are independent of any particular IPC
 * backend (D-Bus, TIDL, etc.).
 */
namespace Ipc
{
/**
 * @brief Enumeration indicating IPC error type
 */
enum class ErrorType
{
  DEFAULT,      ///< default
  INVALID_REPLY ///< reply message has error
};

/**
 * @brief Represents an error from an IPC operation.
 */
struct Error
{
  std::string message;
  ErrorType   errorType{ErrorType::DEFAULT};

  Error() = default;
  Error(std::string msg, ErrorType errorType = ErrorType::DEFAULT)
  : message(std::move(msg)),
    errorType(errorType)
  {
    assert(!message.empty());
  }
};

/**
 * @brief Marker type for successful void operations.
 */
struct Success
{
};

/**
 * @brief Value representing data from an IPC operation, or an error message.
 *
 * Object of this class either holds a series of values (of types ARGS...)
 * or an error message. This object will be true in boolean context if it has data,
 * and false if an error occurred.
 * It's valid to create ValueOrError object with empty argument list or void:
 * \code{.cpp}
 * ValueOrError<> v1;
 * ValueOrError<void> v2;
 * \endcode
 * Both mean the same - ValueOrError containing no real data and being a marker,
 * whether operation succeeded or failed and containing possible error message.
 */
template<typename... ARGS>
class ValueOrError
{
public:
  /**
   * @brief Empty constructor. Valid only if all ARGS types are default constructible.
   */
  ValueOrError() = default;

  /**
   * @brief Value constructor.
   *
   * This will be initialized as success with passed in values.
   */
  ValueOrError(ARGS... t)
  : value(std::move(t)...)
  {
  }

  /**
   * @brief Alternative Value constructor.
   *
   * This will be initialized as success with passed in values.
   */
  ValueOrError(std::tuple<ARGS...> t)
  : value(std::move(t))
  {
  }

  /**
   * @brief Error constructor. This will be initialized as failure with given error message.
   */
  ValueOrError(Error e)
  : error(std::move(e))
  {
    assert(!error.message.empty());
  }

  /**
   * @brief bool operator.
   *
   * Returns true if operation was successful (getValues member is callable), or false
   * when operation failed (getError is callable).
   */
  explicit operator bool() const
  {
    return error.message.empty();
  }

  /**
   * @brief Returns error message object.
   */
  const Error& getError() const
  {
    return error;
  }

  /**
   * @brief Returns modifiable tuple of held data.
   */
  std::tuple<ARGS...>& getValues()
  {
    assert(*this);
    return value;
  }

  /**
   * @brief Returns const tuple of held data.
   */
  const std::tuple<ARGS...>& getValues() const
  {
    assert(*this);
    return value;
  }

protected:
  std::tuple<ARGS...> value;
  Error               error;
};

/**
 * @brief Specialization for empty value list.
 */
template<>
class ValueOrError<>
{
public:
  ValueOrError() = default;
  ValueOrError(std::tuple<> t)
  {
  }
  ValueOrError(Error e)
  : error(std::move(e))
  {
    assert(!error.message.empty());
  }

  explicit operator bool() const
  {
    return error.message.empty();
  }
  const Error& getError() const
  {
    return error;
  }
  std::tuple<>& getValues()
  {
    assert(*this);
    static std::tuple<> t;
    return t;
  }
  std::tuple<> getValues() const
  {
    assert(*this);
    return {};
  }

protected:
  Error error;
};

/**
 * @brief Specialization for void (equivalent to empty).
 */
template<>
class ValueOrError<void>
{
public:
  ValueOrError() = default;
  ValueOrError(Success)
  {
  }
  ValueOrError(Error e)
  : error(std::move(e))
  {
    assert(!error.message.empty());
  }

  explicit operator bool() const
  {
    return error.message.empty();
  }
  const Error& getError() const
  {
    return error;
  }
  std::tuple<>& getValues()
  {
    assert(*this);
    static std::tuple<> t;
    return t;
  }
  std::tuple<> getValues() const
  {
    assert(*this);
    return {};
  }

protected:
  Error error;
};

} // namespace Ipc

namespace std
{
template<size_t INDEX, typename... ARGS>
inline auto get(Ipc::ValueOrError<ARGS...>& v) -> decltype(std::get<INDEX>(v.getValues()))&
{
  return std::get<INDEX>(v.getValues());
}

template<size_t INDEX, typename... ARGS>
inline auto get(const Ipc::ValueOrError<ARGS...>& v) -> decltype(std::get<INDEX>(v.getValues()))
{
  return std::get<INDEX>(v.getValues());
}
} // namespace std

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_RESULT_H
