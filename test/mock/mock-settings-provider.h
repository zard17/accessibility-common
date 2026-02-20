#ifndef ACCESSIBILITY_TEST_MOCK_SETTINGS_PROVIDER_H
#define ACCESSIBILITY_TEST_MOCK_SETTINGS_PROVIDER_H

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
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/settings-provider.h>

/**
 * @brief Mock SettingsProvider with configurable settings for tests.
 */
class MockSettingsProvider : public Accessibility::SettingsProvider
{
public:
  Accessibility::ScreenReaderSettings getSettings() const override
  {
    return mSettings;
  }

  void onSettingsChanged(std::function<void(const Accessibility::ScreenReaderSettings&)> callback) override
  {
    mSettingsCallbacks.push_back(std::move(callback));
  }

  void onLanguageChanged(std::function<void()> callback) override
  {
    mLanguageCallbacks.push_back(std::move(callback));
  }

  void onKeyboardStateChanged(std::function<void(bool)> callback) override
  {
    mKeyboardCallbacks.push_back(std::move(callback));
  }

  // Test helpers
  void setSettings(const Accessibility::ScreenReaderSettings& settings)
  {
    mSettings = settings;
    fireSettingsChanged();
  }

  void fireSettingsChanged()
  {
    for(auto& cb : mSettingsCallbacks)
    {
      cb(mSettings);
    }
  }

  void fireLanguageChanged()
  {
    for(auto& cb : mLanguageCallbacks)
    {
      cb();
    }
  }

  void fireKeyboardStateChanged(bool visible)
  {
    for(auto& cb : mKeyboardCallbacks)
    {
      cb(visible);
    }
  }

private:
  Accessibility::ScreenReaderSettings mSettings;
  std::vector<std::function<void(const Accessibility::ScreenReaderSettings&)>> mSettingsCallbacks;
  std::vector<std::function<void()>> mLanguageCallbacks;
  std::vector<std::function<void(bool)>> mKeyboardCallbacks;
};

#endif // ACCESSIBILITY_TEST_MOCK_SETTINGS_PROVIDER_H
