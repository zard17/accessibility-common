#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_TTS_ENGINE_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_TTS_ENGINE_H

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
#include <functional>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/tts-engine.h>

namespace Accessibility
{
/**
 * @brief Tizen TTS engine scaffold.
 *
 * Implements the TtsEngine interface using Tizen TTS native API.
 * All methods are scaffold implementations with TODO comments for
 * real tts_create / tts_play / tts_stop integration.
 */
class TizenTtsEngine : public TtsEngine
{
public:
  TizenTtsEngine()
  : mNextCommandId{1u},
    mPaused{false}
  {
    // TODO: Call tts_create() and tts_prepare() to initialize the Tizen TTS handle
  }

  ~TizenTtsEngine() override
  {
    // TODO: Call tts_unprepare() and tts_destroy() to release the Tizen TTS handle
  }

  /**
   * @copydoc TtsEngine::speak()
   */
  CommandId speak(const std::string& text, const SpeakOptions& options) override
  {
    CommandId id = mNextCommandId++;

    // TODO: Call tts_add_text() and tts_play() with the Tizen TTS handle
    // TODO: Map CommandId to the Tizen utterance ID for tracking

    if(mUtteranceStartedCallback)
    {
      // TODO: Fire this from the tts_state_changed_cb when playback begins
      mUtteranceStartedCallback(id);
    }

    return id;
  }

  /**
   * @copydoc TtsEngine::stop()
   */
  void stop() override
  {
    // TODO: Call tts_stop() with the Tizen TTS handle
    mPaused = false;
  }

  /**
   * @copydoc TtsEngine::pause()
   */
  bool pause() override
  {
    // TODO: Call tts_pause() with the Tizen TTS handle and check return value
    mPaused = true;
    return true;
  }

  /**
   * @copydoc TtsEngine::resume()
   */
  bool resume() override
  {
    // TODO: Call tts_play() to resume from paused state
    mPaused = false;
    return true;
  }

  /**
   * @copydoc TtsEngine::isPaused()
   */
  bool isPaused() const override
  {
    // TODO: Query tts_get_state() instead of local flag
    return false;
  }

  /**
   * @copydoc TtsEngine::purge()
   */
  void purge(bool onlyDiscardable) override
  {
    // TODO: Call tts_stop() and clear any queued utterances
    // TODO: If onlyDiscardable, only remove utterances marked discardable
  }

  /**
   * @copydoc TtsEngine::onUtteranceStarted()
   */
  void onUtteranceStarted(std::function<void(CommandId)> callback) override
  {
    mUtteranceStartedCallback = std::move(callback);
    // TODO: Register tts_utterance_started_cb with Tizen TTS API
  }

  /**
   * @copydoc TtsEngine::onUtteranceCompleted()
   */
  void onUtteranceCompleted(std::function<void(CommandId)> callback) override
  {
    mUtteranceCompletedCallback = std::move(callback);
    // TODO: Register tts_utterance_completed_cb with Tizen TTS API
  }

private:
  CommandId mNextCommandId;
  bool      mPaused;

  std::function<void(CommandId)> mUtteranceStartedCallback;
  std::function<void(CommandId)> mUtteranceCompletedCallback;

  // TODO: tts_h mTtsHandle{nullptr};  // Tizen TTS native handle
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TIZEN_TIZEN_TTS_ENGINE_H
