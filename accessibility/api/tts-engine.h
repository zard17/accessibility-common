#ifndef ACCESSIBILITY_API_TTS_ENGINE_H
#define ACCESSIBILITY_API_TTS_ENGINE_H

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

namespace Accessibility
{
/**
 * @brief Unique identifier for a TTS utterance command.
 */
using CommandId = uint32_t;

/**
 * @brief Options for a TTS speak command.
 */
struct SpeakOptions
{
  bool discardable = true;
  bool interrupt   = false;
};

/**
 * @brief Abstract interface for text-to-speech engines.
 *
 * Platform backends implement this interface to provide TTS capability.
 * The TtsCommandQueue uses TtsEngine to produce speech output.
 */
class TtsEngine
{
public:
  virtual ~TtsEngine() = default;

  /**
   * @brief Speaks the given text.
   *
   * @param[in] text The text to speak
   * @param[in] options Speak options (discardable, interrupt)
   * @return A CommandId identifying this utterance
   */
  virtual CommandId speak(const std::string& text, const SpeakOptions& options) = 0;

  /**
   * @brief Stops all current speech.
   */
  virtual void stop() = 0;

  /**
   * @brief Pauses current speech.
   *
   * @return true if pause succeeded
   */
  virtual bool pause() = 0;

  /**
   * @brief Resumes paused speech.
   *
   * @return true if resume succeeded
   */
  virtual bool resume() = 0;

  /**
   * @brief Returns whether speech is currently paused.
   */
  virtual bool isPaused() const = 0;

  /**
   * @brief Purges queued or in-progress speech.
   *
   * @param[in] onlyDiscardable If true, only purge discardable commands
   */
  virtual void purge(bool onlyDiscardable) = 0;

  /**
   * @brief Registers a callback for when an utterance starts playing.
   *
   * @param[in] callback Called with the CommandId of the started utterance
   */
  virtual void onUtteranceStarted(std::function<void(CommandId)> callback) = 0;

  /**
   * @brief Registers a callback for when an utterance completes.
   *
   * @param[in] callback Called with the CommandId of the completed utterance
   */
  virtual void onUtteranceCompleted(std::function<void(CommandId)> callback) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_TTS_ENGINE_H
