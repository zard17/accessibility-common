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
#include <accessibility/internal/service/screen-reader/tts-command-queue.h>

namespace Accessibility
{
struct ScreenReaderService::Impl
{
  std::unique_ptr<TtsEngine>            ttsEngine;
  std::unique_ptr<FeedbackProvider>     feedbackProvider;
  std::unique_ptr<SettingsProvider>     settingsProvider;
  std::unique_ptr<ScreenReaderSwitch>   screenReaderSwitch;
  std::unique_ptr<DirectReadingService> directReadingService;
  ReadingComposer                       composer;
  std::unique_ptr<TtsCommandQueue>      ttsQueue;
  bool                                  running{false};
};

ScreenReaderService::ScreenReaderService(
  std::unique_ptr<AppRegistry>          registry,
  std::unique_ptr<GestureProvider>      gestureProvider,
  std::unique_ptr<TtsEngine>            ttsEngine,
  std::unique_ptr<FeedbackProvider>     feedbackProvider,
  std::unique_ptr<SettingsProvider>     settingsProvider,
  std::unique_ptr<ScreenReaderSwitch>   screenReaderSwitch,
  std::unique_ptr<DirectReadingService> directReadingService)
: AccessibilityService(std::move(registry), std::move(gestureProvider)),
  mImpl(std::make_unique<Impl>())
{
  mImpl->ttsEngine            = std::move(ttsEngine);
  mImpl->feedbackProvider     = std::move(feedbackProvider);
  mImpl->settingsProvider     = std::move(settingsProvider);
  mImpl->screenReaderSwitch   = std::move(screenReaderSwitch);
  mImpl->directReadingService = std::move(directReadingService);
  mImpl->ttsQueue             = std::make_unique<TtsCommandQueue>(*mImpl->ttsEngine);
}

ScreenReaderService::~ScreenReaderService()
{
  if(mImpl->running)
  {
    stopScreenReader();
  }
}

void ScreenReaderService::startScreenReader()
{
  if(mImpl->running) return;

  start();

  if(mImpl->screenReaderSwitch)
  {
    mImpl->screenReaderSwitch->setScreenReaderEnabled(true);
    mImpl->screenReaderSwitch->setWmEnabled(true);
  }

  if(mImpl->directReadingService)
  {
    mImpl->directReadingService->start(*mImpl->ttsEngine);
  }

  mImpl->running = true;
}

void ScreenReaderService::stopScreenReader()
{
  if(!mImpl->running) return;

  mImpl->ttsQueue->purgeAll();

  if(mImpl->directReadingService)
  {
    mImpl->directReadingService->stop();
  }

  if(mImpl->screenReaderSwitch)
  {
    mImpl->screenReaderSwitch->setWmEnabled(false);
    mImpl->screenReaderSwitch->setScreenReaderEnabled(false);
  }

  mImpl->running = false;
  stop();
}

void ScreenReaderService::readNode(std::shared_ptr<NodeProxy> node)
{
  if(!node || !mImpl->running) return;

  auto rm = node->getReadingMaterial();
  auto text = mImpl->composer.compose(rm);
  if(!text.empty())
  {
    mImpl->ttsQueue->enqueue(text, true, true);
  }
}

TtsEngine& ScreenReaderService::getTtsEngine()
{
  return *mImpl->ttsEngine;
}

FeedbackProvider& ScreenReaderService::getFeedbackProvider()
{
  return *mImpl->feedbackProvider;
}

SettingsProvider& ScreenReaderService::getSettingsProvider()
{
  return *mImpl->settingsProvider;
}

bool ScreenReaderService::isScreenReaderRunning() const
{
  return mImpl->running;
}

void ScreenReaderService::onAccessibilityEvent(const AccessibilityEvent& event)
{
  if(!mImpl->running) return;

  switch(event.type)
  {
    case AccessibilityEvent::Type::STATE_CHANGED:
    {
      if(event.detail == "highlighted" && event.detail1 == 1)
      {
        auto current = getCurrentNode();
        if(current)
        {
          readNode(current);
          auto settings = mImpl->settingsProvider->getSettings();
          if(settings.soundFeedback)
          {
            auto states = current->getStates();
            if(states[State::FOCUSABLE])
            {
              mImpl->feedbackProvider->playSound(SoundType::HIGHLIGHT_ACTIONABLE);
            }
            else
            {
              mImpl->feedbackProvider->playSound(SoundType::HIGHLIGHT);
            }
          }
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
      auto settings = mImpl->settingsProvider->getSettings();
      if(settings.soundFeedback)
      {
        mImpl->feedbackProvider->playSound(SoundType::WINDOW_STATE_CHANGE);
      }
      break;
    }
    default:
      break;
  }
}

void ScreenReaderService::onWindowChanged(std::shared_ptr<NodeProxy> /*window*/)
{
  // Window change is handled via WINDOW_CHANGED event
}

void ScreenReaderService::onGesture(const GestureInfo& gesture)
{
  if(!mImpl->running) return;

  switch(gesture.type)
  {
    case Gesture::ONE_FINGER_FLICK_RIGHT:
    {
      auto node = navigateNext();
      if(node)
      {
        readNode(node);
        auto settings = mImpl->settingsProvider->getSettings();
        if(settings.soundFeedback)
        {
          mImpl->feedbackProvider->playSound(SoundType::HIGHLIGHT);
        }
      }
      else
      {
        auto settings = mImpl->settingsProvider->getSettings();
        if(settings.soundFeedback)
        {
          mImpl->feedbackProvider->playSound(SoundType::FOCUS_CHAIN_END);
        }
      }
      break;
    }
    case Gesture::ONE_FINGER_FLICK_LEFT:
    {
      auto node = navigatePrev();
      if(node)
      {
        readNode(node);
        auto settings = mImpl->settingsProvider->getSettings();
        if(settings.soundFeedback)
        {
          mImpl->feedbackProvider->playSound(SoundType::HIGHLIGHT);
        }
      }
      else
      {
        auto settings = mImpl->settingsProvider->getSettings();
        if(settings.soundFeedback)
        {
          mImpl->feedbackProvider->playSound(SoundType::FOCUS_CHAIN_END);
        }
      }
      break;
    }
    case Gesture::ONE_FINGER_DOUBLE_TAP:
    {
      auto current = getCurrentNode();
      if(current)
      {
        bool activated = current->doActionByName("activate");
        auto settings = mImpl->settingsProvider->getSettings();
        if(settings.soundFeedback)
        {
          mImpl->feedbackProvider->playSound(SoundType::ACTION);
        }
        (void)activated;
      }
      break;
    }
    case Gesture::TWO_FINGERS_SINGLE_TAP:
    {
      if(mImpl->ttsQueue->isPaused())
      {
        mImpl->ttsQueue->resume();
      }
      else
      {
        mImpl->ttsQueue->pause();
      }
      break;
    }
    case Gesture::THREE_FINGERS_SINGLE_TAP:
    {
      // Review from top: navigate to first element and read
      auto window = getActiveWindow();
      if(window)
      {
        auto first = navigateNext();
        if(first)
        {
          readNode(first);
        }
      }
      break;
    }
    case Gesture::ONE_FINGER_SINGLE_TAP:
    {
      // Point navigation not implemented in mock (no navigableAtPoint)
      break;
    }
    default:
      break;
  }
}

bool ScreenReaderService::onKeyEvent(const KeyEvent& key)
{
  if(!mImpl->running) return false;

  if(key.state == KeyEvent::State::DOWN)
  {
    if(key.keyName == "Back")
    {
      auto prev = navigatePrev();
      if(prev)
      {
        readNode(prev);
      }
      return true;
    }
    else if(key.keyName == "Power")
    {
      mImpl->ttsQueue->purgeAll();
      return true;
    }
  }

  return false;
}

} // namespace Accessibility
