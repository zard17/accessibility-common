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

// CLASS HEADER
#include <accessibility/internal/service/screen-reader/tts-command-queue.h>

// INTERNAL INCLUDES
#include <accessibility/api/tts-engine.h>

namespace Accessibility
{
TtsCommandQueue::TtsCommandQueue(TtsEngine& engine, Config config)
: mEngine(engine),
  mConfig(config)
{
  mEngine.onUtteranceCompleted([this](CommandId id)
  {
    onUtteranceCompleted(id);
  });
}

void TtsCommandQueue::enqueue(const std::string& text, bool discardable, bool interrupt)
{
  if(text.empty())
  {
    return;
  }

  if(interrupt)
  {
    purgeDiscardable();
  }

  auto chunks = chunkText(text, mConfig.maxChunkSize);
  for(auto& chunk : chunks)
  {
    Command cmd;
    cmd.text        = std::move(chunk);
    cmd.discardable = discardable;
    mQueue.push_back(std::move(cmd));
  }

  if(!mSpeaking && !mPaused)
  {
    speakNext();
  }
}

void TtsCommandQueue::purgeDiscardable()
{
  mEngine.purge(true);

  auto it = mQueue.begin();
  while(it != mQueue.end())
  {
    if(it->discardable)
    {
      it = mQueue.erase(it);
    }
    else
    {
      ++it;
    }
  }

  if(mSpeaking)
  {
    mEngine.stop();
    mSpeaking = false;
  }

  if(!mQueue.empty() && !mPaused)
  {
    speakNext();
  }
}

void TtsCommandQueue::purgeAll()
{
  mEngine.stop();
  mQueue.clear();
  mSpeaking = false;
}

void TtsCommandQueue::pause()
{
  if(!mPaused)
  {
    mPaused = true;
    if(mSpeaking)
    {
      mEngine.pause();
    }
  }
}

void TtsCommandQueue::resume()
{
  if(mPaused)
  {
    mPaused = false;
    if(mSpeaking)
    {
      mEngine.resume();
    }
    else if(!mQueue.empty())
    {
      speakNext();
    }
  }
}

bool TtsCommandQueue::isPaused() const
{
  return mPaused;
}

size_t TtsCommandQueue::pendingCount() const
{
  return mQueue.size();
}

void TtsCommandQueue::onUtteranceCompleted(uint32_t commandId)
{
  if(commandId == mCurrentCommandId)
  {
    mSpeaking = false;
    if(!mQueue.empty() && !mPaused)
    {
      speakNext();
    }
  }
}

void TtsCommandQueue::speakNext()
{
  if(mQueue.empty())
  {
    return;
  }

  auto cmd = std::move(mQueue.front());
  mQueue.pop_front();

  SpeakOptions options;
  options.discardable = cmd.discardable;
  options.interrupt   = false;

  mCurrentCommandId = mEngine.speak(cmd.text, options);
  cmd.commandId     = mCurrentCommandId;
  mSpeaking         = true;
}

std::vector<std::string> TtsCommandQueue::chunkText(const std::string& text, size_t maxSize)
{
  std::vector<std::string> chunks;

  if(text.size() <= maxSize)
  {
    chunks.push_back(text);
    return chunks;
  }

  size_t pos = 0;
  while(pos < text.size())
  {
    if(pos + maxSize >= text.size())
    {
      chunks.push_back(text.substr(pos));
      break;
    }

    // Find the last space within maxSize
    size_t end = pos + maxSize;
    size_t lastSpace = text.rfind(' ', end);

    if(lastSpace != std::string::npos && lastSpace > pos)
    {
      chunks.push_back(text.substr(pos, lastSpace - pos));
      pos = lastSpace + 1; // skip the space
    }
    else
    {
      // No space found — force break at maxSize
      chunks.push_back(text.substr(pos, maxSize));
      pos += maxSize;
    }
  }

  return chunks;
}

} // namespace Accessibility
