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

#include <tools/inspector/tts.h>

void Speak(const std::string& text)
{
  @autoreleasepool
  {
    static AVSpeechSynthesizer* synth = nil;
    if(!synth)
    {
      synth = [[AVSpeechSynthesizer alloc] init];
    }
    if([synth isSpeaking])
    {
      [synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    }
    AVSpeechUtterance* utterance = [AVSpeechUtterance speechUtteranceWithString:
      [NSString stringWithUTF8String:text.c_str()]];
    utterance.rate = AVSpeechUtteranceDefaultSpeechRate;
    [synth speakUtterance:utterance];
  }
}
