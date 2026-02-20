#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_SCREEN_READER_SWITCH_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_SCREEN_READER_SWITCH_H

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

// INTERNAL INCLUDES
#include <accessibility/api/screen-reader-switch.h>

namespace Accessibility
{
/**
 * @brief Tizen screen reader switch scaffold.
 *
 * Implements the ScreenReaderSwitch interface using D-Bus property
 * set on org.a11y.Status and WM notification.
 */
class TizenScreenReaderSwitch : public ScreenReaderSwitch
{
public:
  TizenScreenReaderSwitch()
  {
    // TODO: Read initial state from org.a11y.Status ScreenReaderEnabled property
  }

  ~TizenScreenReaderSwitch() override = default;

  void setScreenReaderEnabled(bool enabled) override
  {
    mScreenReaderEnabled = enabled;
    // TODO: Set org.a11y.Status ScreenReaderEnabled D-Bus property
  }

  bool getScreenReaderEnabled() const override
  {
    // TODO: Query org.a11y.Status ScreenReaderEnabled D-Bus property
    return mScreenReaderEnabled;
  }

  void setIsEnabled(bool enabled) override
  {
    mIsEnabled = enabled;
    // TODO: Set org.a11y.Status IsEnabled D-Bus property
  }

  void setWmEnabled(bool enabled) override
  {
    mWmEnabled = enabled;
    // TODO: Send D-Bus message to org.enlightenment.wm-screen-reader
    //       to enable/disable gesture recognition module
  }

private:
  bool mScreenReaderEnabled{false};
  bool mIsEnabled{false};
  bool mWmEnabled{false};
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_SCREEN_READER_SWITCH_H
