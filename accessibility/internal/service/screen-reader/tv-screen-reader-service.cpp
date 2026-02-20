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
#include <accessibility/api/screen-reader-service.h>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/node-proxy.h>
#include <accessibility/api/reading-composer.h>

namespace Accessibility
{
struct TvScreenReaderService::Impl
{
  std::unique_ptr<TtsEngine>        ttsEngine;
  std::unique_ptr<SettingsProvider>  settingsProvider;
  ReadingComposer                   composer;
  bool                              running{false};

  Impl()
  : composer(ReadingComposerConfig{true, true}) // TV config: suppress touch hints, include TV traits
  {
  }
};

TvScreenReaderService::TvScreenReaderService(
  std::unique_ptr<AppRegistry>     registry,
  std::unique_ptr<GestureProvider>  gestureProvider,
  std::unique_ptr<TtsEngine>        ttsEngine,
  std::unique_ptr<SettingsProvider>  settingsProvider)
: AccessibilityService(std::move(registry), std::move(gestureProvider)),
  mImpl(std::make_unique<Impl>())
{
  mImpl->ttsEngine       = std::move(ttsEngine);
  mImpl->settingsProvider = std::move(settingsProvider);
}

TvScreenReaderService::~TvScreenReaderService()
{
  if(mImpl->running)
  {
    stopScreenReader();
  }
}

void TvScreenReaderService::startScreenReader()
{
  if(mImpl->running) return;

  start();
  mImpl->running = true;
}

void TvScreenReaderService::stopScreenReader()
{
  if(!mImpl->running) return;

  mImpl->ttsEngine->stop();
  mImpl->running = false;
  stop();
}

void TvScreenReaderService::readNode(std::shared_ptr<NodeProxy> node)
{
  if(!node || !mImpl->running) return;

  auto rm = node->getReadingMaterial();
  auto text = mImpl->composer.compose(rm);
  if(!text.empty())
  {
    SpeakOptions options;
    options.discardable = true;
    options.interrupt   = true;
    mImpl->ttsEngine->speak(text, options);
  }
}

TtsEngine& TvScreenReaderService::getTtsEngine()
{
  return *mImpl->ttsEngine;
}

bool TvScreenReaderService::isScreenReaderRunning() const
{
  return mImpl->running;
}

void TvScreenReaderService::onAccessibilityEvent(const AccessibilityEvent& event)
{
  if(!mImpl->running) return;

  switch(event.type)
  {
    case AccessibilityEvent::Type::STATE_CHANGED:
    {
      // TV mode: focus change triggers read
      if(event.detail == "focused" && event.detail1 == 1)
      {
        auto current = getCurrentNode();
        if(current)
        {
          readNode(current);
        }
      }
      break;
    }
    case AccessibilityEvent::Type::PROPERTY_CHANGED:
    {
      auto current = getCurrentNode();
      if(current)
      {
        readNode(current);
      }
      break;
    }
    case AccessibilityEvent::Type::WINDOW_CHANGED:
    {
      // Announce window change
      if(!event.detail.empty())
      {
        SpeakOptions options;
        options.discardable = true;
        options.interrupt   = true;
        mImpl->ttsEngine->speak(event.detail, options);
      }
      break;
    }
    default:
      break;
  }
}

void TvScreenReaderService::onWindowChanged(std::shared_ptr<NodeProxy> /*window*/)
{
  // Handled via WINDOW_CHANGED event
}

void TvScreenReaderService::onGesture(const GestureInfo& /*gesture*/)
{
  // TV mode does not handle gestures
}

} // namespace Accessibility
