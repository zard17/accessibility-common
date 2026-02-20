#ifndef ACCESSIBILITY_TEST_MOCK_SCREEN_READER_SWITCH_H
#define ACCESSIBILITY_TEST_MOCK_SCREEN_READER_SWITCH_H

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

/**
 * @brief Mock ScreenReaderSwitch that records all method calls for test assertions.
 */
class MockScreenReaderSwitch : public Accessibility::ScreenReaderSwitch
{
public:
  void setScreenReaderEnabled(bool enabled) override
  {
    mScreenReaderEnabled = enabled;
    ++mSetScreenReaderEnabledCount;
  }

  bool getScreenReaderEnabled() const override
  {
    return mScreenReaderEnabled;
  }

  void setIsEnabled(bool enabled) override
  {
    mIsEnabled = enabled;
    ++mSetIsEnabledCount;
  }

  void setWmEnabled(bool enabled) override
  {
    mWmEnabled = enabled;
    ++mSetWmEnabledCount;
  }

  // Test accessors
  int getSetScreenReaderEnabledCount() const { return mSetScreenReaderEnabledCount; }
  int getSetIsEnabledCount() const { return mSetIsEnabledCount; }
  int getSetWmEnabledCount() const { return mSetWmEnabledCount; }
  bool isWmEnabled() const { return mWmEnabled; }
  bool isIsEnabled() const { return mIsEnabled; }

  void reset()
  {
    mScreenReaderEnabled = false;
    mIsEnabled = false;
    mWmEnabled = false;
    mSetScreenReaderEnabledCount = 0;
    mSetIsEnabledCount = 0;
    mSetWmEnabledCount = 0;
  }

private:
  bool mScreenReaderEnabled{false};
  bool mIsEnabled{false};
  bool mWmEnabled{false};
  int  mSetScreenReaderEnabledCount{0};
  int  mSetIsEnabledCount{0};
  int  mSetWmEnabledCount{0};
};

#endif // ACCESSIBILITY_TEST_MOCK_SCREEN_READER_SWITCH_H
