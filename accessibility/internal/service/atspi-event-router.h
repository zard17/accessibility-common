#ifndef ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_EVENT_ROUTER_H
#define ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_EVENT_ROUTER_H

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
#include <functional>
#include <memory>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility-event.h>
#include <accessibility/internal/bridge/dbus/dbus.h>

namespace Accessibility
{
class AccessibilityService;

/**
 * @brief Subscribes to AT-SPI D-Bus event signals and routes them to an AccessibilityService.
 *
 * Listens for Event.Object and Event.Window signals on the accessibility bus,
 * translates them into AccessibilityEvent, and dispatches to the service.
 *
 * Signal mapping:
 *   Event.Object::StateChanged           -> STATE_CHANGED
 *   Event.Object::PropertyChange         -> PROPERTY_CHANGED
 *   Event.Object::BoundsChanged          -> BOUNDS_CHANGED
 *   Event.Object::ActiveDescendantChanged -> ACTIVE_DESCENDANT_CHANGED
 *   Event.Object::TextCaretMoved         -> TEXT_CARET_MOVED
 *   Event.Object::TextChanged            -> TEXT_CHANGED
 *   Event.Object::MoveOuted              -> MOVED_OUT
 *   Event.Object::ScrollStarted          -> SCROLL_STARTED
 *   Event.Object::ScrollFinished         -> SCROLL_FINISHED
 *   Event.Window::Activate/Deactivate/Create/Destroy -> WINDOW_CHANGED
 */
class AtSpiEventRouter
{
public:
  using EventCallback = std::function<void(const AccessibilityEvent&)>;

  /**
   * @brief Constructs an AtSpiEventRouter.
   *
   * @param[in] connection The D-Bus connection to the accessibility bus
   */
  explicit AtSpiEventRouter(DBusWrapper::ConnectionPtr connection);

  ~AtSpiEventRouter();

  /**
   * @brief Starts listening for AT-SPI signals.
   *
   * @param[in] callback The function to call when an event is received
   */
  void start(EventCallback callback);

  /**
   * @brief Stops listening for AT-SPI signals.
   */
  void stop();

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_EVENT_ROUTER_H
