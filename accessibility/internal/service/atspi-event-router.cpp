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
#include <accessibility/internal/service/atspi-event-router.h>

// EXTERNAL INCLUDES
#include <string>
#include <unordered_map>

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>

namespace Accessibility
{
struct AtSpiEventRouter::Impl
{
  DBusWrapper::ConnectionPtr connection;
  EventCallback              callback;
  bool                       running{false};

  // Maps D-Bus signal names to AccessibilityEvent::Type
  static const std::unordered_map<std::string, AccessibilityEvent::Type>& getObjectSignalMap()
  {
    static const std::unordered_map<std::string, AccessibilityEvent::Type> map{
      {"StateChanged", AccessibilityEvent::Type::STATE_CHANGED},
      {"PropertyChange", AccessibilityEvent::Type::PROPERTY_CHANGED},
      {"BoundsChanged", AccessibilityEvent::Type::BOUNDS_CHANGED},
      {"ActiveDescendantChanged", AccessibilityEvent::Type::ACTIVE_DESCENDANT_CHANGED},
      {"TextCaretMoved", AccessibilityEvent::Type::TEXT_CARET_MOVED},
      {"TextChanged", AccessibilityEvent::Type::TEXT_CHANGED},
      {"MoveOuted", AccessibilityEvent::Type::MOVED_OUT},
      {"ScrollStarted", AccessibilityEvent::Type::SCROLL_STARTED},
      {"ScrollFinished", AccessibilityEvent::Type::SCROLL_FINISHED},
    };
    return map;
  }

  static const std::unordered_map<std::string, AccessibilityEvent::Type>& getWindowSignalMap()
  {
    static const std::unordered_map<std::string, AccessibilityEvent::Type> map{
      {"Activate", AccessibilityEvent::Type::WINDOW_CHANGED},
      {"Deactivate", AccessibilityEvent::Type::WINDOW_CHANGED},
      {"Create", AccessibilityEvent::Type::WINDOW_CHANGED},
      {"Destroy", AccessibilityEvent::Type::WINDOW_CHANGED},
    };
    return map;
  }
};

AtSpiEventRouter::AtSpiEventRouter(DBusWrapper::ConnectionPtr connection)
: mImpl(std::make_unique<Impl>())
{
  mImpl->connection = std::move(connection);
}

AtSpiEventRouter::~AtSpiEventRouter()
{
  stop();
}

void AtSpiEventRouter::start(EventCallback callback)
{
  mImpl->callback = std::move(callback);
  mImpl->running  = true;

  // In a full implementation, this would subscribe to D-Bus signals using:
  //   DBusWrapper::Installed()->eldbus_proxy_signal_handler_add_impl(...)
  // For each signal in Event.Object and Event.Window interfaces.
  //
  // The signal handler would parse the D-Bus message, create an AccessibilityEvent,
  // and call mImpl->callback(event).
  //
  // This is the subscription scaffold — actual D-Bus match rules need
  // the a11y bus connection to be fully established.
}

void AtSpiEventRouter::stop()
{
  mImpl->running  = false;
  mImpl->callback = nullptr;
}

} // namespace Accessibility
