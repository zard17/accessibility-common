#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_TTS_ENGINE_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_TTS_ENGINE_H

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
#include <iostream>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/tts-engine.h>

namespace Accessibility
{
/**
 * @brief Stub TtsEngine that prints speech to stdout.
 */
class StubTtsEngine : public TtsEngine
{
public:
  CommandId speak(const std::string& text, const SpeakOptions& /*options*/) override
  {
    auto id = ++mNextCommandId;
    std::cout << "[TTS] " << text << std::endl;
    return id;
  }

  void stop() override {}

  bool pause() override { mPaused = true; return true; }

  bool resume() override { mPaused = false; return true; }

  bool isPaused() const override { return mPaused; }

  void purge(bool /*onlyDiscardable*/) override { stop(); }

  void onUtteranceStarted(std::function<void(CommandId)> callback) override
  {
    mStartedCallback = std::move(callback);
  }

  void onUtteranceCompleted(std::function<void(CommandId)> callback) override
  {
    mCompletedCallback = std::move(callback);
  }

private:
  CommandId                       mNextCommandId{0};
  bool                            mPaused{false};
  std::function<void(CommandId)>  mStartedCallback;
  std::function<void(CommandId)>  mCompletedCallback;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_TTS_ENGINE_H
