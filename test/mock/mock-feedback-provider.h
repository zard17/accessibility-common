#ifndef ACCESSIBILITY_TEST_MOCK_FEEDBACK_PROVIDER_H
#define ACCESSIBILITY_TEST_MOCK_FEEDBACK_PROVIDER_H

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
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/feedback-provider.h>

/**
 * @brief Mock FeedbackProvider that records all calls for test assertions.
 */
class MockFeedbackProvider : public Accessibility::FeedbackProvider
{
public:
  void playSound(Accessibility::SoundType type) override
  {
    mPlayedSounds.push_back(type);
  }

  void vibrate(int32_t durationMs, int32_t intensity) override
  {
    ++mVibrateCount;
  }

  const std::vector<Accessibility::SoundType>& getPlayedSounds() const { return mPlayedSounds; }
  int getVibrateCount() const { return mVibrateCount; }

  void reset()
  {
    mPlayedSounds.clear();
    mVibrateCount = 0;
  }

private:
  std::vector<Accessibility::SoundType> mPlayedSounds;
  int mVibrateCount{0};
};

#endif // ACCESSIBILITY_TEST_MOCK_FEEDBACK_PROVIDER_H
