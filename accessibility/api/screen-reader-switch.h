#ifndef ACCESSIBILITY_API_SCREEN_READER_SWITCH_H
#define ACCESSIBILITY_API_SCREEN_READER_SWITCH_H

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

namespace Accessibility
{
/**
 * @brief Abstract interface for the screen reader on/off switch.
 *
 * Controls org.a11y.Status properties + WM gesture module activation.
 */
class ScreenReaderSwitch
{
public:
  virtual ~ScreenReaderSwitch() = default;

  /**
   * @brief Sets the ScreenReaderEnabled property on org.a11y.Status.
   */
  virtual void setScreenReaderEnabled(bool enabled) = 0;

  /**
   * @brief Gets the ScreenReaderEnabled property.
   */
  virtual bool getScreenReaderEnabled() const = 0;

  /**
   * @brief Sets the IsEnabled property on org.a11y.Status.
   */
  virtual void setIsEnabled(bool enabled) = 0;

  /**
   * @brief Notifies the WM gesture module to enable/disable.
   */
  virtual void setWmEnabled(bool enabled) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_SCREEN_READER_SWITCH_H
