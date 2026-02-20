#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_FEEDBACK_PROVIDER_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_FEEDBACK_PROVIDER_H

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

// INTERNAL INCLUDES
#include <accessibility/api/feedback-provider.h>

namespace Accessibility
{
/**
 * @brief Stub FeedbackProvider for platforms without audio/haptic hardware.
 */
class StubFeedbackProvider : public FeedbackProvider
{
public:
  void playSound(SoundType /*type*/) override {}

  void vibrate(int32_t /*durationMs*/, int32_t /*intensity*/) override {}
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_STUB_FEEDBACK_PROVIDER_H
