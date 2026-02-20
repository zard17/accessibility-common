#ifndef ACCESSIBILITY_TOOLS_SCREEN_READER_MAC_TTS_ENGINE_H
#define ACCESSIBILITY_TOOLS_SCREEN_READER_MAC_TTS_ENGINE_H

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
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/tts-engine.h>

/**
 * @brief TtsEngine implementation using macOS AVSpeechSynthesizer.
 *
 * Supports speak, stop, pause, resume, and utterance callbacks.
 * Objective-C++ implementation is in mac-tts-engine.mm.
 */
class MacTtsEngine : public Accessibility::TtsEngine
{
public:
  MacTtsEngine();
  ~MacTtsEngine() override;

  Accessibility::CommandId speak(const std::string& text, const Accessibility::SpeakOptions& options) override;
  void stop() override;
  bool pause() override;
  bool resume() override;
  bool isPaused() const override;
  void purge(bool onlyDiscardable) override;
  void onUtteranceStarted(std::function<void(Accessibility::CommandId)> callback) override;
  void onUtteranceCompleted(std::function<void(Accessibility::CommandId)> callback) override;

private:
  struct Impl;
  Impl* mImpl;
};

#endif // ACCESSIBILITY_TOOLS_SCREEN_READER_MAC_TTS_ENGINE_H
