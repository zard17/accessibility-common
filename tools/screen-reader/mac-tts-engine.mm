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

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <tools/screen-reader/mac-tts-engine.h>

namespace
{
/**
 * @brief File-scope data shared between MacTtsEngine and the ObjC delegate.
 */
struct TtsImplData
{
  AVSpeechSynthesizer*                          synth{nil};
  id                                            delegate{nil};
  Accessibility::CommandId                      nextId{1};
  Accessibility::CommandId                      currentId{0};
  bool                                          paused{false};
  std::function<void(Accessibility::CommandId)> startedCallback;
  std::function<void(Accessibility::CommandId)> completedCallback;
};
} // namespace

struct MacTtsEngine::Impl : public TtsImplData
{
};

// Delegate to bridge AVSpeechSynthesizerDelegate callbacks to C++ lambdas
@interface MacTtsDelegate : NSObject <AVSpeechSynthesizerDelegate>
@property (nonatomic, assign) TtsImplData* implData;
@end

@implementation MacTtsDelegate

- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didStartSpeechUtterance:(AVSpeechUtterance*)utterance
{
  if(self.implData && self.implData->startedCallback)
  {
    self.implData->startedCallback(self.implData->currentId);
  }
}

- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance*)utterance
{
  if(self.implData && self.implData->completedCallback)
  {
    self.implData->completedCallback(self.implData->currentId);
  }
}

- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didCancelSpeechUtterance:(AVSpeechUtterance*)utterance
{
  if(self.implData && self.implData->completedCallback)
  {
    self.implData->completedCallback(self.implData->currentId);
  }
}

@end

MacTtsEngine::MacTtsEngine()
: mImpl(new Impl)
{
  @autoreleasepool
  {
    mImpl->synth = [[AVSpeechSynthesizer alloc] init];
    MacTtsDelegate* delegate = [[MacTtsDelegate alloc] init];
    delegate.implData = mImpl;
    mImpl->delegate = delegate;
    mImpl->synth.delegate = (id<AVSpeechSynthesizerDelegate>)mImpl->delegate;
  }
}

MacTtsEngine::~MacTtsEngine()
{
  @autoreleasepool
  {
    [mImpl->synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    mImpl->synth.delegate = nil;
    mImpl->synth    = nil;
    mImpl->delegate = nil;
  }
  delete mImpl;
}

Accessibility::CommandId MacTtsEngine::speak(const std::string& text, const Accessibility::SpeakOptions& options)
{
  @autoreleasepool
  {
    if(options.interrupt && [mImpl->synth isSpeaking])
    {
      [mImpl->synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    }

    mImpl->currentId = mImpl->nextId++;
    mImpl->paused    = false;

    AVSpeechUtterance* utterance = [AVSpeechUtterance speechUtteranceWithString:
      [NSString stringWithUTF8String:text.c_str()]];
    utterance.rate = AVSpeechUtteranceDefaultSpeechRate;

    [mImpl->synth speakUtterance:utterance];

    return mImpl->currentId;
  }
}

void MacTtsEngine::stop()
{
  @autoreleasepool
  {
    mImpl->paused = false;
    [mImpl->synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
  }
}

bool MacTtsEngine::pause()
{
  @autoreleasepool
  {
    if([mImpl->synth isSpeaking] && !mImpl->paused)
    {
      mImpl->paused = [mImpl->synth pauseSpeakingAtBoundary:AVSpeechBoundaryImmediate];
      return mImpl->paused;
    }
    return false;
  }
}

bool MacTtsEngine::resume()
{
  @autoreleasepool
  {
    if(mImpl->paused)
    {
      mImpl->paused = false;
      return [mImpl->synth continueSpeaking];
    }
    return false;
  }
}

bool MacTtsEngine::isPaused() const
{
  return mImpl->paused;
}

void MacTtsEngine::purge(bool /*onlyDiscardable*/)
{
  @autoreleasepool
  {
    mImpl->paused = false;
    [mImpl->synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
  }
}

void MacTtsEngine::onUtteranceStarted(std::function<void(Accessibility::CommandId)> callback)
{
  mImpl->startedCallback = std::move(callback);
}

void MacTtsEngine::onUtteranceCompleted(std::function<void(Accessibility::CommandId)> callback)
{
  mImpl->completedCallback = std::move(callback);
}
