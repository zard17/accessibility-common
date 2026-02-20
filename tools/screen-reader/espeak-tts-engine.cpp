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
#include <cstdio>
#include <espeak-ng/speak_lib.h>

// INTERNAL INCLUDES
#include <tools/screen-reader/espeak-tts-engine.h>

EspeakTtsEngine::EspeakTtsEngine()
{
  int result = espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, nullptr, 0);
  if(result == EE_INTERNAL_ERROR)
  {
    fprintf(stderr, "EspeakTtsEngine: espeak_Initialize failed\n");
  }
  else
  {
    mInitialized = true;
  }
}

EspeakTtsEngine::~EspeakTtsEngine()
{
  if(mInitialized)
  {
    espeak_Cancel();
    espeak_Terminate();
  }
}

Accessibility::CommandId EspeakTtsEngine::speak(const std::string& text, const Accessibility::SpeakOptions& options)
{
  if(!mInitialized)
  {
    return 0;
  }

  if(options.interrupt)
  {
    espeak_Cancel();
  }

  mCurrentId = mNextId++;

  if(mStartedCallback)
  {
    mStartedCallback(mCurrentId);
  }

  espeak_Synth(text.c_str(),
               text.size() + 1,
               0,     // position
               POS_CHARACTER,
               0,     // end position (0 = no end)
               espeakCHARS_UTF8,
               nullptr,
               nullptr);

  // Fire completion callback after synth call.
  // espeak_Synth in AUDIO_OUTPUT_PLAYBACK mode is async but we simplify
  // by signalling completion immediately (the audio continues playing).
  if(mCompletedCallback)
  {
    mCompletedCallback(mCurrentId);
  }

  return mCurrentId;
}

void EspeakTtsEngine::stop()
{
  if(mInitialized)
  {
    espeak_Cancel();
  }
}

bool EspeakTtsEngine::pause()
{
  // espeak-ng has no pause API
  return false;
}

bool EspeakTtsEngine::resume()
{
  // espeak-ng has no resume API
  return false;
}

bool EspeakTtsEngine::isPaused() const
{
  return false;
}

void EspeakTtsEngine::purge(bool /*onlyDiscardable*/)
{
  if(mInitialized)
  {
    espeak_Cancel();
  }
}

void EspeakTtsEngine::onUtteranceStarted(std::function<void(Accessibility::CommandId)> callback)
{
  mStartedCallback = std::move(callback);
}

void EspeakTtsEngine::onUtteranceCompleted(std::function<void(Accessibility::CommandId)> callback)
{
  mCompletedCallback = std::move(callback);
}
