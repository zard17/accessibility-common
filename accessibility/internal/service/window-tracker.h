#ifndef ACCESSIBILITY_INTERNAL_SERVICE_WINDOW_TRACKER_H
#define ACCESSIBILITY_INTERNAL_SERVICE_WINDOW_TRACKER_H

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
#include <memory>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/dbus/dbus.h>

namespace Accessibility
{
/**
 * @brief Tracks active and visible windows using the windowManager D-Bus interface.
 *
 * Uses org.enlightenment.wm.proc::GetVisibleWinInfo and GetFocusProc
 * to determine which application windows are visible and focused.
 */
class WindowTracker
{
public:
  struct WindowInfo
  {
    int32_t     pid{0};
    std::string busName;
    bool        focused{false};
  };

  using WindowChangedCallback = std::function<void(const WindowInfo&)>;

  /**
   * @brief Constructs a WindowTracker.
   *
   * @param[in] connection The D-Bus connection (session bus)
   */
  explicit WindowTracker(DBusWrapper::ConnectionPtr connection);

  ~WindowTracker();

  /**
   * @brief Gets the currently focused window info.
   */
  WindowInfo getFocusedWindow();

  /**
   * @brief Gets all visible window info.
   */
  std::vector<WindowInfo> getVisibleWindows();

  /**
   * @brief Starts tracking window changes.
   *
   * @param[in] callback Called when the focused window changes
   */
  void start(WindowChangedCallback callback);

  /**
   * @brief Stops tracking window changes.
   */
  void stop();

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_WINDOW_TRACKER_H
