#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_WM_GESTURE_PROVIDER_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_WM_GESTURE_PROVIDER_H

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

// INTERNAL INCLUDES
#include <accessibility/api/gesture-provider.h>

namespace Accessibility
{
/**
 * @brief Window-manager gesture provider scaffold for Tizen.
 *
 * Implements the GestureProvider interface by subscribing to D-Bus signals
 * from the Tizen window manager's gesture navigation service
 * (org.tizen.GestureNavigation). Converts WM gesture signals into
 * GestureInfo events and forwards them to the registered callback.
 */
class WmGestureProvider : public GestureProvider
{
public:
  WmGestureProvider()
  {
    // TODO: Connect to the system D-Bus and add a signal match for:
    //   interface: org.tizen.GestureNavigation
    //   signal:    GestureDetected
    //   path:      /org/tizen/GestureNavigation
    //
    // In the signal handler, parse the gesture type and coordinates,
    // construct a GestureInfo, and call mGestureCallback if set.
  }

  ~WmGestureProvider() override
  {
    // TODO: Remove the D-Bus signal match and disconnect from the bus
  }

  /**
   * @copydoc GestureProvider::onGestureReceived()
   */
  void onGestureReceived(std::function<void(const GestureInfo&)> callback) override
  {
    mGestureCallback = std::move(callback);
    // TODO: If already connected to D-Bus, the signal handler will use
    //       this callback. If not yet connected, initiate connection here.
  }

private:
  std::function<void(const GestureInfo&)> mGestureCallback;

  // TODO: DBusConnection* mConnection{nullptr};  // D-Bus connection handle
  // TODO: guint mSignalSubscriptionId{0};         // g_dbus_connection_signal_subscribe ID
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_WM_GESTURE_PROVIDER_H
