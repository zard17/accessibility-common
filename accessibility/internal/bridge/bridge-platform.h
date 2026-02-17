#ifndef ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_PLATFORM_H
#define ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_PLATFORM_H

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

// INTERNAL INCLUDES
#include <accessibility/public-api/accessibility-common.h>

namespace Accessibility
{
/**
 * @brief Platform-specific callback interface.
 *
 * This structure allows the accessibility-common library to call back into
 * the platform adaptor without directly depending on adaptor-impl.h.
 * The adaptor registers these callbacks at initialization time.
 */
struct ACCESSIBILITY_API PlatformCallbacks
{
  /**
   * @brief Adds an idle callback to the platform's event loop.
   *
   * @param[in] callback The callback to invoke on idle; return true to keep, false to remove
   * @return A handle identifying the idle callback (0 on failure)
   */
  std::function<uint32_t(std::function<bool()>)> addIdle;

  /**
   * @brief Removes an idle callback from the platform's event loop.
   *
   * @param[in] handle The handle returned by addIdle
   */
  std::function<void(uint32_t)> removeIdle;

  /**
   * @brief Gets the toolkit version string.
   *
   * @return The toolkit version (e.g. "2.3.0")
   */
  std::function<std::string()> getToolkitVersion;

  /**
   * @brief Gets the application package name.
   *
   * @return The application name
   */
  std::function<std::string()> getAppName;

  /**
   * @brief Checks if the adaptor is available.
   *
   * @return true if the adaptor is available
   */
  std::function<bool()> isAdaptorAvailable;

  /**
   * @brief Called when EnableAutoInit() is invoked and bridge is not yet initialized.
   *
   * The platform adaptor should perform the actual initialization
   * (e.g. obtaining the root layer, setting the application name, etc.)
   */
  std::function<void()> onEnableAutoInit;

  /**
   * @brief Creates a repeating timer that fires on the main thread.
   *
   * @param[in] intervalMs Interval in milliseconds
   * @param[in] callback Called on each tick; return true to continue, false to auto-stop
   * @return A handle identifying the timer (0 on failure)
   */
  std::function<uint32_t(uint32_t intervalMs, std::function<bool()> callback)> createTimer;

  /**
   * @brief Cancels a timer created by createTimer.
   *
   * @param[in] handle The timer handle returned by createTimer
   */
  std::function<void(uint32_t handle)> cancelTimer;

  /**
   * @brief Checks if a timer is currently ticking.
   *
   * @param[in] handle The timer handle returned by createTimer
   * @return true if the timer is currently running
   */
  std::function<bool(uint32_t handle)> isTimerRunning;
};

/**
 * @brief Sets the platform callbacks.
 *
 * @param[in] callbacks The callbacks to set
 */
ACCESSIBILITY_API void SetPlatformCallbacks(const PlatformCallbacks& callbacks);

/**
 * @brief Gets the platform callbacks.
 *
 * @return The current platform callbacks
 */
ACCESSIBILITY_API const PlatformCallbacks& GetPlatformCallbacks();

/**
 * @brief A repeating timer that fires callbacks on the main thread.
 *
 * Uses PlatformCallbacks to delegate to platform-native timers,
 * avoiding a direct dependency on platform-specific timers.
 */
class ACCESSIBILITY_API RepeatingTimer
{
public:
  RepeatingTimer() = default;
  ~RepeatingTimer();

  RepeatingTimer(const RepeatingTimer&) = delete;
  RepeatingTimer& operator=(const RepeatingTimer&) = delete;

  RepeatingTimer(RepeatingTimer&& rhs) noexcept;
  RepeatingTimer& operator=(RepeatingTimer&& rhs) noexcept;

  /**
   * @brief Starts the timer. Cancels any previously running timer first.
   *
   * @param[in] intervalMs Interval in milliseconds
   * @param[in] callback Called on each tick; return true to continue, false to auto-stop
   */
  void Start(uint32_t intervalMs, std::function<bool()> callback);

  /**
   * @brief Stops and invalidates the timer.
   */
  void Stop();

  /**
   * @brief Returns true if the timer is currently ticking.
   */
  bool IsRunning() const;

  /**
   * @brief Returns true if the timer handle is valid (started and not stopped).
   */
  explicit operator bool() const;

private:
  uint32_t mHandle{0};
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_PLATFORM_H
