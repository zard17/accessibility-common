#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_SCREEN_READER_SWITCH_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_SCREEN_READER_SWITCH_H

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
 * @brief Stub ScreenReaderSwitch that stores state in memory. No platform integration.
 */
class StubScreenReaderSwitch : public ScreenReaderSwitch
{
public:
  void setScreenReaderEnabled(bool enabled) override { mEnabled = enabled; }

  bool getScreenReaderEnabled() const override { return mEnabled; }

  void setIsEnabled(bool enabled) override { mIsEnabled = enabled; }

  void setWmEnabled(bool enabled) override { mWmEnabled = enabled; }

private:
  bool mEnabled{false};
  bool mIsEnabled{false};
  bool mWmEnabled{false};
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_SCREEN_READER_SWITCH_H
