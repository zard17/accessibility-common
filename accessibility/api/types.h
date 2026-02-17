#ifndef ACCESSIBILITY_TYPES_H
#define ACCESSIBILITY_TYPES_H

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
#include <string>
#include <vector>

namespace Accessibility
{
/**
 * @brief Generic rectangle type.
 */
template<typename T>
struct Rect
{
  T x{};
  T y{};
  T width{};
  T height{};

  Rect() = default;

  Rect(T x, T y, T width, T height)
  : x(x),
    y(y),
    width(width),
    height(height)
  {
  }

  bool operator==(const Rect& rhs) const
  {
    return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
  }

  bool operator!=(const Rect& rhs) const
  {
    return !(*this == rhs);
  }

  /**
   * @brief Checks if this rectangle intersects with another.
   */
  bool Intersects(const Rect& other) const
  {
    return !(x + width <= other.x || other.x + other.width <= x ||
             y + height <= other.y || other.y + other.height <= y);
  }
};

/**
 * @brief Generic key event type.
 */
struct KeyEvent
{
  enum class State
  {
    DOWN,
    UP
  };

  std::string keyName;
  std::string keyString;
  int32_t     keyCode{0};
  State       state{State::DOWN};
  uint32_t    time{0};
};

/**
 * @brief Simple signal class backed by a vector of std::function slots.
 */
template<typename... Args>
class Signal
{
public:
  using SlotType = std::function<void(Args...)>;

  /**
   * @brief Connects a slot to this signal.
   *
   * @param[in] slot The slot to connect
   */
  void Connect(SlotType slot)
  {
    mSlots.emplace_back(std::move(slot));
  }

  /**
   * @brief Emits the signal, calling all connected slots.
   *
   * @param[in] args Arguments forwarded to slots
   */
  void Emit(Args... args)
  {
    for(auto& slot : mSlots)
    {
      if(slot)
      {
        slot(args...);
      }
    }
  }

private:
  std::vector<SlotType> mSlots;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_TYPES_H
