#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TTS_COMMAND_QUEUE_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TTS_COMMAND_QUEUE_H

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
#include <deque>
#include <string>
#include <vector>

namespace Accessibility
{
class TtsEngine;

/**
 * @brief TTS command queue with chunking, discard policy, and pause/resume.
 *
 * Pure C++ logic, no platform dependency. Wraps a TtsEngine and manages
 * the order and chunking of utterances.
 *
 * Features:
 * - Text chunking at configurable max length (default 300 chars)
 * - Discardable vs non-discardable commands
 * - Purge discardable commands (for interrupt)
 * - Pause/resume state tracking
 */
class TtsCommandQueue
{
public:
  struct Config
  {
    size_t maxChunkSize = 300;
  };

  explicit TtsCommandQueue(TtsEngine& engine, Config config = {300});

  /**
   * @brief Enqueues text to be spoken.
   *
   * Long text is automatically chunked. If interrupt is true,
   * all existing discardable commands are purged first.
   *
   * @param[in] text The text to speak
   * @param[in] discardable Whether this command can be purged
   * @param[in] interrupt Whether to purge existing commands first
   */
  void enqueue(const std::string& text, bool discardable = true, bool interrupt = false);

  /**
   * @brief Purges all discardable commands and stops current speech.
   */
  void purgeDiscardable();

  /**
   * @brief Purges all commands and stops current speech.
   */
  void purgeAll();

  /**
   * @brief Pauses the queue. Current utterance is paused via TTS engine.
   */
  void pause();

  /**
   * @brief Resumes the queue. Current utterance is resumed via TTS engine.
   */
  void resume();

  /**
   * @brief Returns whether the queue is paused.
   */
  bool isPaused() const;

  /**
   * @brief Returns the number of pending commands.
   */
  size_t pendingCount() const;

  /**
   * @brief Called when an utterance completes. Advances to next command.
   */
  void onUtteranceCompleted(uint32_t commandId);

  /**
   * @brief Splits text into chunks at word boundaries.
   *
   * @param[in] text The text to chunk
   * @param[in] maxSize Maximum chunk size in characters
   * @return Vector of text chunks
   */
  static std::vector<std::string> chunkText(const std::string& text, size_t maxSize);

private:
  struct Command
  {
    std::string text;
    bool        discardable;
    uint32_t    commandId{0};
  };

  void speakNext();

  TtsEngine&          mEngine;
  Config              mConfig;
  std::deque<Command> mQueue;
  bool                mPaused{false};
  bool                mSpeaking{false};
  uint32_t            mCurrentCommandId{0};
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_TTS_COMMAND_QUEUE_H
