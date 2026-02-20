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
#include <algorithm>
#include <cstdio>
#include <functional>
#include <vector>

#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/public-api/controls/buttons/check-box-button.h>
#include <dali-toolkit/public-api/controls/progress-bar/progress-bar.h>
#include <dali/devel-api/adaptor-framework/actor-accessible.h>

// INTERNAL INCLUDES
#include <accessibility/api/screen-reader-service.h>
#include <accessibility/internal/service/screen-reader/stub/stub-direct-reading-service.h>
#include <accessibility/internal/service/screen-reader/stub/stub-feedback-provider.h>
#include <accessibility/internal/service/screen-reader/stub/stub-screen-reader-switch.h>
#include <accessibility/internal/service/screen-reader/stub/stub-settings-provider.h>
#include <tools/screen-reader/direct-app-registry.h>
#include <tools/screen-reader/mac-tts-engine.h>

using namespace Dali;
using namespace Dali::Toolkit;

namespace
{
/**
 * @brief GestureProvider that allows keyboard events to inject gestures.
 *
 * Same pattern as MockGestureProvider but intended for production demo use.
 */
class KeyboardGestureProvider : public ::Accessibility::GestureProvider
{
public:
  void onGestureReceived(std::function<void(const ::Accessibility::GestureInfo&)> callback) override
  {
    mCallbacks.push_back(std::move(callback));
  }

  void fireGesture(const ::Accessibility::GestureInfo& gesture)
  {
    for(auto& cb : mCallbacks)
    {
      cb(gesture);
    }
  }

private:
  std::vector<std::function<void(const ::Accessibility::GestureInfo&)>> mCallbacks;
};

/**
 * @brief Prints an accessibility tree rooted at the given node.
 */
void PrintTree(::Accessibility::Accessible* node, int depth)
{
  if(!node) return;

  for(int i = 0; i < depth; ++i) fprintf(stdout, "  ");

  auto states = node->GetStates();
  fprintf(stdout, "[%s] \"%s\"", node->GetRoleName().c_str(), node->GetName().c_str());
  if(states[::Accessibility::State::HIGHLIGHTABLE]) fprintf(stdout, " (highlightable)");
  if(states[::Accessibility::State::FOCUSABLE])     fprintf(stdout, " (focusable)");
  fprintf(stdout, "\n");

  for(auto* child : node->GetChildren())
  {
    PrintTree(child, depth + 1);
  }
}
} // namespace

/**
 * @brief Screen reader demo: real DALi controls + embedded ScreenReaderService.
 *
 * Keyboard shortcuts:
 *   Right / n  : Navigate next (FLICK_RIGHT)
 *   Left  / b  : Navigate prev (FLICK_LEFT)
 *   Enter / d  : Activate (DOUBLE_TAP)
 *   Space / p  : Pause/resume TTS (TWO_FINGERS_SINGLE_TAP)
 *   r          : Read from top (THREE_FINGERS_SINGLE_TAP)
 *   t          : Print accessibility tree
 *   Esc / q    : Quit
 */
class ScreenReaderDemo : public ConnectionTracker
{
public:
  ScreenReaderDemo(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &ScreenReaderDemo::Create);
  }

  ~ScreenReaderDemo() = default;

  void Create(Application& application)
  {
    Window  window     = application.GetWindow();
    Vector2 windowSize = window.GetSize();

    window.SetBackgroundColor(Color::WHITE);

    // --- Create real DALi controls ---

    // Title label
    TextLabel title = TextLabel::New("Screen Reader Demo");
    title.SetProperty(Actor::Property::NAME, "Screen Reader Demo");
    title.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER);
    title.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER);
    title.SetProperty(Actor::Property::POSITION, Vector2(0.0f, 30.0f));
    title.SetProperty(Actor::Property::SIZE, Vector2(windowSize.width * 0.8f, 60.0f));
    title.SetProperty(TextLabel::Property::HORIZONTAL_ALIGNMENT, "CENTER");
    title.SetProperty(TextLabel::Property::POINT_SIZE, 15.0f);
    title.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(title);

    // Play button
    mPlayBtn = PushButton::New();
    mPlayBtn.SetProperty(Actor::Property::NAME, "Play");
    mPlayBtn.SetProperty(Button::Property::LABEL, "Play");
    mPlayBtn.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER);
    mPlayBtn.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER);
    mPlayBtn.SetProperty(Actor::Property::POSITION, Vector2(0.0f, 120.0f));
    mPlayBtn.SetProperty(Actor::Property::SIZE, Vector2(200.0f, 60.0f));
    mPlayBtn.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(mPlayBtn);

    // Stop button
    mStopBtn = PushButton::New();
    mStopBtn.SetProperty(Actor::Property::NAME, "Stop");
    mStopBtn.SetProperty(Button::Property::LABEL, "Stop");
    mStopBtn.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER);
    mStopBtn.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER);
    mStopBtn.SetProperty(Actor::Property::POSITION, Vector2(0.0f, 200.0f));
    mStopBtn.SetProperty(Actor::Property::SIZE, Vector2(200.0f, 60.0f));
    mStopBtn.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(mStopBtn);

    // Volume progress bar
    mVolumeBar = ProgressBar::New();
    mVolumeBar.SetProperty(Actor::Property::NAME, "Volume");
    mVolumeBar.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER);
    mVolumeBar.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER);
    mVolumeBar.SetProperty(Actor::Property::POSITION, Vector2(0.0f, 300.0f));
    mVolumeBar.SetProperty(Actor::Property::SIZE, Vector2(windowSize.width * 0.7f, 50.0f));
    mVolumeBar.SetProperty(ProgressBar::Property::PROGRESS_VALUE, 0.5f);
    mVolumeBar.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(mVolumeBar);

    // Autoplay checkbox
    mAutoplayCheck = CheckBoxButton::New();
    mAutoplayCheck.SetProperty(Actor::Property::NAME, "Autoplay");
    mAutoplayCheck.SetProperty(Button::Property::LABEL, "Autoplay");
    mAutoplayCheck.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER);
    mAutoplayCheck.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER);
    mAutoplayCheck.SetProperty(Actor::Property::POSITION, Vector2(0.0f, 390.0f));
    mAutoplayCheck.SetProperty(Actor::Property::SIZE, Vector2(200.0f, 50.0f));
    mAutoplayCheck.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(mAutoplayCheck);

    // Status label
    mStatusLabel = TextLabel::New("Use arrow keys to navigate, Enter to activate");
    mStatusLabel.SetProperty(Actor::Property::NAME, "Status");
    mStatusLabel.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::BOTTOM_CENTER);
    mStatusLabel.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::BOTTOM_CENTER);
    mStatusLabel.SetProperty(Actor::Property::POSITION, Vector2(0.0f, -40.0f));
    mStatusLabel.SetProperty(Actor::Property::SIZE, Vector2(windowSize.width * 0.9f, 40.0f));
    mStatusLabel.SetProperty(TextLabel::Property::HORIZONTAL_ALIGNMENT, "CENTER");
    mStatusLabel.SetProperty(TextLabel::Property::POINT_SIZE, 9.0f);
    window.Add(mStatusLabel);

    // Connect button signals
    mPlayBtn.ClickedSignal().Connect(this, &ScreenReaderDemo::OnPlayClicked);
    mStopBtn.ClickedSignal().Connect(this, &ScreenReaderDemo::OnStopClicked);
    mAutoplayCheck.ClickedSignal().Connect(this, &ScreenReaderDemo::OnAutoplayClicked);

    // --- Get root accessible from DALi ---

    ::Accessibility::Accessible* rootAccessible =
      Dali::Accessibility::ActorAccessible::Get(window.GetRootLayer());

    if(!rootAccessible)
    {
      fprintf(stderr, "ERROR: Could not get root accessible. Exiting.\n");
      mApplication.Quit();
      return;
    }

    // Print accessibility tree
    fprintf(stdout, "\n=== Accessibility Tree ===\n");
    PrintTree(rootAccessible, 0);
    fprintf(stdout, "==========================\n\n");

    // --- Create ScreenReaderService ---

    auto registry   = std::make_unique<DirectAppRegistry>(rootAccessible);
    auto gesture    = std::make_unique<KeyboardGestureProvider>();
    auto tts        = std::make_unique<MacTtsEngine>();
    auto feedback   = std::make_unique<::Accessibility::StubFeedbackProvider>();
    auto settings   = std::make_unique<::Accessibility::StubSettingsProvider>();
    auto srSwitch   = std::make_unique<::Accessibility::StubScreenReaderSwitch>();
    auto directRead = std::make_unique<::Accessibility::StubDirectReadingService>();

    mGestureProvider = gesture.get();

    mService = std::make_unique<::Accessibility::ScreenReaderService>(
      std::move(registry),
      std::move(gesture),
      std::move(tts),
      std::move(feedback),
      std::move(settings),
      std::move(srSwitch),
      std::move(directRead));

    mService->startScreenReader();

    mRootAccessible = rootAccessible;

    fprintf(stdout, "Screen reader started. Controls:\n");
    fprintf(stdout, "  Right/n : Next element\n");
    fprintf(stdout, "  Left/b  : Previous element\n");
    fprintf(stdout, "  Enter/d : Activate\n");
    fprintf(stdout, "  Space/p : Pause/resume TTS\n");
    fprintf(stdout, "  r       : Read from top\n");
    fprintf(stdout, "  t       : Print accessibility tree\n");
    fprintf(stdout, "  Esc/q   : Quit\n\n");

    // --- Connect key events ---

    window.KeyEventSignal().Connect(this, &ScreenReaderDemo::OnKeyEvent);
  }

private:
  bool OnPlayClicked(Button button)
  {
    mStatusLabel.SetProperty(TextLabel::Property::TEXT, "Playing...");
    fprintf(stdout, "[Action] Play button activated\n");
    return true;
  }

  bool OnStopClicked(Button button)
  {
    mStatusLabel.SetProperty(TextLabel::Property::TEXT, "Stopped.");
    fprintf(stdout, "[Action] Stop button activated\n");
    return true;
  }

  bool OnAutoplayClicked(Button button)
  {
    bool selected = button.GetProperty<bool>(Button::Property::SELECTED);
    std::string msg = selected ? "Autoplay: ON" : "Autoplay: OFF";
    mStatusLabel.SetProperty(TextLabel::Property::TEXT, msg);
    fprintf(stdout, "[Action] Autoplay toggled: %s\n", selected ? "ON" : "OFF");
    return true;
  }

  void ActivateCurrentNode()
  {
    if(!mService) return;
    auto node = mService->getCurrentNode();
    if(!node) return;

    std::string name = node->getName();
    fprintf(stdout, "[Activate] %s (%s)\n", name.c_str(), node->getRoleName().c_str());

    if(name == "Play")
    {
      mStatusLabel.SetProperty(TextLabel::Property::TEXT, "Playing...");
    }
    else if(name == "Stop")
    {
      mStatusLabel.SetProperty(TextLabel::Property::TEXT, "Stopped.");
    }
    else if(name == "Autoplay")
    {
      bool selected = mAutoplayCheck.GetProperty<bool>(Button::Property::SELECTED);
      mAutoplayCheck.SetProperty(Button::Property::SELECTED, !selected);
      std::string msg = !selected ? "Autoplay: ON" : "Autoplay: OFF";
      mStatusLabel.SetProperty(TextLabel::Property::TEXT, msg);
    }
  }

  void OnKeyEvent(const KeyEvent& event)
  {
    if(event.GetState() != KeyEvent::DOWN) return;

    std::string keyName = event.GetKeyName();

    // Quit
    if(IsKey(event, DALI_KEY_ESCAPE) || IsKey(event, DALI_KEY_BACK) || keyName == "q")
    {
      if(mService)
      {
        mService->stopScreenReader();
      }
      mApplication.Quit();
      return;
    }

    // Volume Up/Down
    if(keyName == "Up" || keyName == "Down")
    {
      float value = mVolumeBar.GetProperty<float>(ProgressBar::Property::PROGRESS_VALUE);
      if(keyName == "Up")
      {
        value = std::min(1.0f, value + 0.1f);
      }
      else
      {
        value = std::max(0.0f, value - 0.1f);
      }
      mVolumeBar.SetProperty(ProgressBar::Property::PROGRESS_VALUE, value);
      char buf[64];
      snprintf(buf, sizeof(buf), "Volume: %d%%", static_cast<int>(value * 100.0f));
      mStatusLabel.SetProperty(TextLabel::Property::TEXT, std::string(buf));
      fprintf(stdout, "[Action] %s\n", buf);
      return;
    }

    // Print tree
    if(keyName == "t")
    {
      fprintf(stdout, "\n=== Accessibility Tree ===\n");
      PrintTree(mRootAccessible, 0);
      fprintf(stdout, "==========================\n\n");
      return;
    }

    // Map key to gesture
    if(!mGestureProvider) return;

    ::Accessibility::GestureInfo gi{};
    gi.state = ::Accessibility::GestureState::ENDED;

    if(keyName == "Right" || keyName == "n")
    {
      gi.type = ::Accessibility::Gesture::ONE_FINGER_FLICK_RIGHT;
    }
    else if(keyName == "Left" || keyName == "b")
    {
      gi.type = ::Accessibility::Gesture::ONE_FINGER_FLICK_LEFT;
    }
    else if(keyName == "Return" || keyName == "d")
    {
      gi.type = ::Accessibility::Gesture::ONE_FINGER_DOUBLE_TAP;
      // Also perform direct activation since DALi DoAction may not
      // trigger ClickedSignal on macOS
      ActivateCurrentNode();
    }
    else if(keyName == "space" || keyName == "p")
    {
      gi.type = ::Accessibility::Gesture::TWO_FINGERS_SINGLE_TAP;
    }
    else if(keyName == "r")
    {
      gi.type = ::Accessibility::Gesture::THREE_FINGERS_SINGLE_TAP;
    }
    else
    {
      return;
    }

    mGestureProvider->fireGesture(gi);
  }

  Application&                                    mApplication;
  std::unique_ptr<::Accessibility::ScreenReaderService> mService;
  KeyboardGestureProvider*                          mGestureProvider{nullptr};
  ::Accessibility::Accessible*                      mRootAccessible{nullptr};
  TextLabel                                         mStatusLabel;
  ProgressBar                                       mVolumeBar;
  PushButton                                        mPlayBtn;
  PushButton                                        mStopBtn;
  CheckBoxButton                                    mAutoplayCheck;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv, "", Application::OPAQUE, PositionSize(0, 0, 480, 800));

  ScreenReaderDemo demo(application);

  application.MainLoop();

  return 0;
}
