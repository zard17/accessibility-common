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
#include <accessibility/internal/bridge/bridge-platform.h>

namespace Accessibility
{
namespace
{
PlatformCallbacks gPlatformCallbacks;
} // unnamed namespace

void SetPlatformCallbacks(const PlatformCallbacks& callbacks)
{
  gPlatformCallbacks = callbacks;
}

const PlatformCallbacks& GetPlatformCallbacks()
{
  return gPlatformCallbacks;
}

RepeatingTimer::~RepeatingTimer()
{
  Stop();
}

RepeatingTimer::RepeatingTimer(RepeatingTimer&& rhs) noexcept
: mHandle(rhs.mHandle)
{
  rhs.mHandle = 0;
}

RepeatingTimer& RepeatingTimer::operator=(RepeatingTimer&& rhs) noexcept
{
  if(this != &rhs)
  {
    Stop();
    mHandle     = rhs.mHandle;
    rhs.mHandle = 0;
  }
  return *this;
}

void RepeatingTimer::Start(uint32_t intervalMs, std::function<bool()> callback)
{
  Stop();
  auto& callbacks = GetPlatformCallbacks();
  if(callbacks.createTimer)
  {
    mHandle = callbacks.createTimer(intervalMs, std::move(callback));
  }
}

void RepeatingTimer::Stop()
{
  if(mHandle != 0)
  {
    auto& callbacks = GetPlatformCallbacks();
    if(callbacks.cancelTimer)
    {
      callbacks.cancelTimer(mHandle);
    }
    mHandle = 0;
  }
}

bool RepeatingTimer::IsRunning() const
{
  if(mHandle == 0)
  {
    return false;
  }
  auto& callbacks = GetPlatformCallbacks();
  if(callbacks.isTimerRunning)
  {
    return callbacks.isTimerRunning(mHandle);
  }
  return false;
}

RepeatingTimer::operator bool() const
{
  return mHandle != 0;
}

} // namespace Accessibility
