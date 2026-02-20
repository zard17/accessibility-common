#ifndef ACCESSIBILITY_TEST_MOCK_TTS_ENGINE_H
#define ACCESSIBILITY_TEST_MOCK_TTS_ENGINE_H

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
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/tts-engine.h>

/**
 * @brief Mock TtsEngine that records all calls for test assertions.
 */
class MockTtsEngine : public Accessibility::TtsEngine
{
public:
  Accessibility::CommandId speak(const std::string& text, const Accessibility::SpeakOptions& options) override
  {
    mSpokenTexts.push_back(text);
    mSpeakOptions.push_back(options);
    auto id = ++mNextId;
    if(mStartedCallback) mStartedCallback(id);
    return id;
  }

  void stop() override { ++mStopCount; }

  bool pause() override { mPaused = true; return true; }

  bool resume() override { mPaused = false; return true; }

  bool isPaused() const override { return mPaused; }

  void purge(bool onlyDiscardable) override
  {
    ++mPurgeCount;
    mLastPurgeOnlyDiscardable = onlyDiscardable;
  }

  void onUtteranceStarted(std::function<void(Accessibility::CommandId)> callback) override
  {
    mStartedCallback = std::move(callback);
  }

  void onUtteranceCompleted(std::function<void(Accessibility::CommandId)> callback) override
  {
    mCompletedCallback = std::move(callback);
  }

  // Test helpers
  const std::vector<std::string>& getSpokenTexts() const { return mSpokenTexts; }
  const std::vector<Accessibility::SpeakOptions>& getSpeakOptions() const { return mSpeakOptions; }
  int getStopCount() const { return mStopCount; }
  int getPurgeCount() const { return mPurgeCount; }
  bool getLastPurgeOnlyDiscardable() const { return mLastPurgeOnlyDiscardable; }

  void fireUtteranceCompleted(Accessibility::CommandId id)
  {
    if(mCompletedCallback) mCompletedCallback(id);
  }

  void reset()
  {
    mSpokenTexts.clear();
    mSpeakOptions.clear();
    mStopCount = 0;
    mPurgeCount = 0;
    mPaused = false;
  }

private:
  std::vector<std::string> mSpokenTexts;
  std::vector<Accessibility::SpeakOptions> mSpeakOptions;
  std::function<void(Accessibility::CommandId)> mStartedCallback;
  std::function<void(Accessibility::CommandId)> mCompletedCallback;
  Accessibility::CommandId mNextId{0};
  int  mStopCount{0};
  int  mPurgeCount{0};
  bool mPaused{false};
  bool mLastPurgeOnlyDiscardable{false};
};

#endif // ACCESSIBILITY_TEST_MOCK_TTS_ENGINE_H
