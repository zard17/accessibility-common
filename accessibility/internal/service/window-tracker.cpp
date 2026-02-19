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

// CLASS HEADER
#include <accessibility/internal/service/window-tracker.h>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/dbus/dbus-locators.h>

namespace Accessibility
{
struct WindowTracker::Impl
{
  DBusWrapper::ConnectionPtr connection;
  WindowChangedCallback      callback;
  bool                       running{false};
};

WindowTracker::WindowTracker(DBusWrapper::ConnectionPtr connection)
: mImpl(std::make_unique<Impl>())
{
  mImpl->connection = std::move(connection);
}

WindowTracker::~WindowTracker()
{
  stop();
}

WindowTracker::WindowInfo WindowTracker::getFocusedWindow()
{
  WindowInfo info{};

  auto client = DBus::DBusClient{
    dbusLocators::windowManager::BUS,
    dbusLocators::windowManager::OBJ_PATH,
    dbusLocators::windowManager::INTERFACE,
    mImpl->connection};

  auto result = client.method<DBus::ValueOrError<int32_t>()>(dbusLocators::windowManager::GET_FOCUS_PROC).call();
  if(result)
  {
    info.pid     = std::get<0>(result.getValues());
    info.focused = true;
  }

  return info;
}

std::vector<WindowTracker::WindowInfo> WindowTracker::getVisibleWindows()
{
  // In full implementation, this would call GetVisibleWinInfo and parse the response.
  // For now, return the focused window only.
  std::vector<WindowInfo> windows;
  auto focused = getFocusedWindow();
  if(focused.pid != 0)
  {
    windows.push_back(focused);
  }
  return windows;
}

void WindowTracker::start(WindowChangedCallback callback)
{
  mImpl->callback = std::move(callback);
  mImpl->running  = true;
  // In full implementation, subscribe to window manager signals
}

void WindowTracker::stop()
{
  mImpl->running  = false;
  mImpl->callback = nullptr;
}

} // namespace Accessibility
