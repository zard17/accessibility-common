#ifndef ACCESSIBILITY_API_FEEDBACK_PROVIDER_H
#define ACCESSIBILITY_API_FEEDBACK_PROVIDER_H

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

namespace Accessibility
{
/**
 * @brief Types of auditory feedback sounds.
 */
enum class SoundType
{
  FOCUS_CHAIN_END,
  HIGHLIGHT,
  HIGHLIGHT_ACTIONABLE,
  ACTION,
  LONG_PRESS,
  CONTEXT_MENU,
  WINDOW_STATE_CHANGE
};

/**
 * @brief Abstract interface for non-speech feedback (sounds and vibration).
 *
 * Platform backends implement this to provide auditory and haptic feedback.
 */
class FeedbackProvider
{
public:
  virtual ~FeedbackProvider() = default;

  /**
   * @brief Plays a feedback sound of the given type.
   *
   * @param[in] type The type of sound to play
   */
  virtual void playSound(SoundType type) = 0;

  /**
   * @brief Triggers a haptic vibration.
   *
   * @param[in] durationMs Duration in milliseconds
   * @param[in] intensity Vibration intensity (0-100)
   */
  virtual void vibrate(int32_t durationMs, int32_t intensity) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_FEEDBACK_PROVIDER_H
