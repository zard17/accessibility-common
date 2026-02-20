#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_SETTINGS_PROVIDER_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_SETTINGS_PROVIDER_H

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
#include <accessibility/api/settings-provider.h>

namespace Accessibility
{
/**
 * @brief Tizen settings provider scaffold.
 *
 * Implements the SettingsProvider interface using Tizen vconf keys.
 * Returns default settings for now; callbacks are stored but not yet
 * connected to vconf change notifications.
 */
class TizenSettingsProvider : public SettingsProvider
{
public:
  TizenSettingsProvider() = default;

  ~TizenSettingsProvider() override
  {
    // TODO: Call vconf_ignore_key_changed() for all registered vconf keys
  }

  ScreenReaderSettings getSettings() const override
  {
    // TODO: Read from vconf keys:
    //   readDescription      <- VCONFKEY_SETAPPL_ACCESSIBILITY_READ_DESCRIPTION
    //   hapticFeedback       <- VCONFKEY_SETAPPL_ACCESSIBILITY_VIBRATION_FEEDBACK
    //   keyboardFeedback     <- VCONFKEY_SETAPPL_ACCESSIBILITY_KEYBOARD_FEEDBACK
    //   soundFeedback        <- VCONFKEY_SETAPPL_ACCESSIBILITY_SOUND_FEEDBACK
    //   lcdBacklightTimeout  <- VCONFKEY_SETAPPL_LCD_TIMEOUT_NORMAL
    //   ttsSpeed             <- VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_SPEECH_RATE
    //   multiWindowNavEnabled <- VCONFKEY_SETAPPL_ACCESSIBILITY_MULTI_WINDOW_NAV
    return ScreenReaderSettings{};
  }

  void onSettingsChanged(std::function<void(const ScreenReaderSettings&)> callback) override
  {
    mSettingsChangedCallback = std::move(callback);
    // TODO: Register vconf_notify_key_changed() for each relevant vconf key
  }

  void onLanguageChanged(std::function<void()> callback) override
  {
    mLanguageChangedCallback = std::move(callback);
    // TODO: Register vconf_notify_key_changed(VCONFKEY_LANGSET, ...)
  }

  void onKeyboardStateChanged(std::function<void(bool)> callback) override
  {
    mKeyboardStateChangedCallback = std::move(callback);
    // TODO: Register vconf_notify_key_changed(VCONFKEY_ISF_INPUT_PANEL_STATE, ...)
  }

private:
  std::function<void(const ScreenReaderSettings&)> mSettingsChangedCallback;
  std::function<void()>                            mLanguageChangedCallback;
  std::function<void(bool)>                        mKeyboardStateChangedCallback;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_SETTINGS_PROVIDER_H
