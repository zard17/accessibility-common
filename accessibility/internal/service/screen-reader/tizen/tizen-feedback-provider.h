#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_FEEDBACK_PROVIDER_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_FEEDBACK_PROVIDER_H

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
#include <cstdint>

// INTERNAL INCLUDES
#include <accessibility/api/feedback-provider.h>

namespace Accessibility
{
/**
 * @brief Tizen feedback provider scaffold.
 *
 * Implements the FeedbackProvider interface using Tizen feedback and
 * haptic native APIs. All methods are scaffold implementations with TODO
 * comments for real feedback_play / haptic integration.
 */
class TizenFeedbackProvider : public FeedbackProvider
{
public:
  TizenFeedbackProvider()
  {
    // TODO: Call feedback_initialize() to set up the Tizen feedback module
    // TODO: Call haptic_open() to obtain a haptic device handle
  }

  ~TizenFeedbackProvider() override
  {
    // TODO: Call feedback_deinitialize() to release feedback resources
    // TODO: Call haptic_close() to release the haptic device handle
  }

  /**
   * @copydoc FeedbackProvider::playSound()
   */
  void playSound(SoundType type) override
  {
    // TODO: Map SoundType to Tizen feedback pattern and call feedback_play_type()
    // Example mapping:
    //   SoundType::FOCUS_CHAIN_END   -> FEEDBACK_PATTERN_SIP
    //   SoundType::HIGHLIGHT         -> FEEDBACK_PATTERN_TAP
    //   SoundType::ACTION            -> FEEDBACK_PATTERN_KEY0
    //   etc.
    (void)type;
  }

  /**
   * @copydoc FeedbackProvider::vibrate()
   */
  void vibrate(int32_t durationMs, int32_t intensity) override
  {
    // TODO: Call haptic_vibrate_monotone() with the device handle,
    //       durationMs and mapped intensity level
    (void)durationMs;
    (void)intensity;
  }

private:
  // TODO: feedback_h      mFeedbackHandle{nullptr};  // Tizen feedback handle
  // TODO: haptic_device_h mHapticHandle{nullptr};     // Tizen haptic device handle
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_FEEDBACK_PROVIDER_H
