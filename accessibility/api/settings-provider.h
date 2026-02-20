#ifndef ACCESSIBILITY_API_SETTINGS_PROVIDER_H
#define ACCESSIBILITY_API_SETTINGS_PROVIDER_H

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

namespace Accessibility
{
/**
 * @brief Screen reader configuration settings.
 *
 * Maps to 7 vconf keys on Tizen.
 */
struct ScreenReaderSettings
{
  bool readDescription      = true;
  bool hapticFeedback       = true;
  bool keyboardFeedback     = true;
  bool soundFeedback        = true;
  int  lcdBacklightTimeout  = 15;
  int  ttsSpeed             = 5;    ///< Range: 1-15
  bool multiWindowNavEnabled = false;
};

/**
 * @brief Abstract interface for reading and observing screen reader settings.
 *
 * Platform backends implement this to provide access to system accessibility
 * settings (e.g., Tizen vconf, Android SharedPreferences).
 */
class SettingsProvider
{
public:
  virtual ~SettingsProvider() = default;

  /**
   * @brief Gets the current screen reader settings.
   */
  virtual ScreenReaderSettings getSettings() const = 0;

  /**
   * @brief Registers a callback for when settings change.
   */
  virtual void onSettingsChanged(std::function<void(const ScreenReaderSettings&)> callback) = 0;

  /**
   * @brief Registers a callback for when the TTS language changes.
   */
  virtual void onLanguageChanged(std::function<void()> callback) = 0;

  /**
   * @brief Registers a callback for when the keyboard state changes.
   */
  virtual void onKeyboardStateChanged(std::function<void(bool visible)> callback) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_SETTINGS_PROVIDER_H
