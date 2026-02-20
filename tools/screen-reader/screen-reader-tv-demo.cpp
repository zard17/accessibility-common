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
#include <vector>

#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/public-api/controls/buttons/check-box-button.h>
#include <dali-toolkit/public-api/controls/progress-bar/progress-bar.h>
#include <dali/devel-api/adaptor-framework/actor-accessible.h>

// INTERNAL INCLUDES
#include <accessibility/api/screen-reader-service.h>
#include <accessibility/internal/service/screen-reader/stub/stub-settings-provider.h>
#include <accessibility/internal/service/stub/stub-gesture-provider.h>
#include <tools/screen-reader/direct-app-registry.h>
#include <tools/screen-reader/mac-tts-engine.h>

using namespace Dali;
using namespace Dali::Toolkit;

namespace
{
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
 * @brief TV screen reader demo: DALi KeyboardFocusManager + TvScreenReaderService.
 *
 * Unlike the gesture-based ScreenReaderDemo, this demo uses DALi's built-in
 * KeyboardFocusManager for navigation. Arrow keys move focus between controls,
 * and FocusChangedSignal triggers TvScreenReaderService to read the focused element.
 *
 * Keyboard shortcuts:
 *   Up/Down    : Move focus (KeyboardFocusManager)
 *   Left/Right : Move focus (KeyboardFocusManager)
 *   Enter      : Activate focused element
 *   t          : Print accessibility tree
 *   Esc / q    : Quit
 */
class ScreenReaderTvDemo : public ConnectionTracker
{
public:
  ScreenReaderTvDemo(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &ScreenReaderTvDemo::Create);
  }

  ~ScreenReaderTvDemo() = default;

  void Create(Application& application)
  {
    Window  window     = application.GetWindow();
    Vector2 windowSize = window.GetSize();

    window.SetBackgroundColor(Color::WHITE);

    // --- Create real DALi controls ---

    // Title label
    mTitle = TextLabel::New("TV Screen Reader Demo");
    mTitle.SetProperty(Actor::Property::NAME, "TV Screen Reader Demo");
    mTitle.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER);
    mTitle.SetProperty(Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER);
    mTitle.SetProperty(Actor::Property::POSITION, Vector2(0.0f, 30.0f));
    mTitle.SetProperty(Actor::Property::SIZE, Vector2(windowSize.width * 0.8f, 60.0f));
    mTitle.SetProperty(TextLabel::Property::HORIZONTAL_ALIGNMENT, "CENTER");
    mTitle.SetProperty(TextLabel::Property::POINT_SIZE, 15.0f);
    mTitle.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(mTitle);

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
    mStatusLabel.SetProperty(Actor::Property::KEYBOARD_FOCUSABLE, true);
    window.Add(mStatusLabel);

    // Connect button signals
    mPlayBtn.ClickedSignal().Connect(this, &ScreenReaderTvDemo::OnPlayClicked);
    mStopBtn.ClickedSignal().Connect(this, &ScreenReaderTvDemo::OnStopClicked);
    mAutoplayCheck.ClickedSignal().Connect(this, &ScreenReaderTvDemo::OnAutoplayClicked);

    // --- Build focus order ---

    mFocusOrder = {mTitle, mPlayBtn, mStopBtn, mVolumeBar, mAutoplayCheck, mStatusLabel};

    // --- KeyboardFocusManager setup ---

    auto focusManager = KeyboardFocusManager::Get();

    // PreFocusChangeSignal: define manual navigation order (vertical list)
    focusManager.PreFocusChangeSignal().Connect(this, &ScreenReaderTvDemo::OnPreFocusChange);

    // FocusChangedSignal: focus change → TvScreenReaderService event
    focusManager.FocusChangedSignal().Connect(this, &ScreenReaderTvDemo::OnFocusChanged);

    // Enter key: activate the currently focused element
    focusManager.FocusedActorEnterKeySignal().Connect(this, &ScreenReaderTvDemo::OnEnterPressed);

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

    // --- Create TvScreenReaderService (4 deps) ---

    auto registry = std::make_unique<DirectAppRegistry>(rootAccessible);
    auto gesture  = std::make_unique<::Accessibility::StubGestureProvider>();
    auto tts      = std::make_unique<MacTtsEngine>();
    auto settings = std::make_unique<::Accessibility::StubSettingsProvider>();

    // Keep a reference to the registry for proxy creation
    mRegistry = static_cast<DirectAppRegistry*>(registry.get());

    mService = std::make_unique<::Accessibility::TvScreenReaderService>(
      std::move(registry),
      std::move(gesture),
      std::move(tts),
      std::move(settings));

    mService->startScreenReader();

    mRootAccessible = rootAccessible;

    fprintf(stdout, "TV screen reader started. Controls:\n");
    fprintf(stdout, "  Up/Down    : Move focus\n");
    fprintf(stdout, "  Left/Right : Move focus\n");
    fprintf(stdout, "  Enter      : Activate\n");
    fprintf(stdout, "  t          : Print accessibility tree\n");
    fprintf(stdout, "  Esc/q      : Quit\n\n");

    // --- Connect key events (for quit and tree print only) ---

    window.KeyEventSignal().Connect(this, &ScreenReaderTvDemo::OnKeyEvent);

    // Set initial focus
    focusManager.SetCurrentFocusActor(mTitle);
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

  /**
   * @brief PreFocusChangeSignal: define navigation order for Up/Down/Left/Right.
   */
  Actor OnPreFocusChange(Actor current, Actor proposed, Control::KeyboardFocus::Direction direction)
  {
    // If no current focus, start at the first element
    if(!current)
    {
      return mFocusOrder[0];
    }

    // Find current position in focus order
    int currentIndex = -1;
    for(size_t i = 0; i < mFocusOrder.size(); ++i)
    {
      if(mFocusOrder[i] == current)
      {
        currentIndex = static_cast<int>(i);
        break;
      }
    }

    if(currentIndex < 0)
    {
      return mFocusOrder[0];
    }

    int nextIndex = currentIndex;
    int count     = static_cast<int>(mFocusOrder.size());

    switch(direction)
    {
      case Control::KeyboardFocus::UP:
      case Control::KeyboardFocus::LEFT:
      {
        nextIndex = (currentIndex - 1 + count) % count;
        break;
      }
      case Control::KeyboardFocus::DOWN:
      case Control::KeyboardFocus::RIGHT:
      {
        nextIndex = (currentIndex + 1) % count;
        break;
      }
      default:
        break;
    }

    return mFocusOrder[nextIndex];
  }

  /**
   * @brief FocusChangedSignal: focus change → TvScreenReaderService event dispatch.
   */
  void OnFocusChanged(Actor oldFocused, Actor newFocused)
  {
    if(!newFocused || !mService) return;

    // 1. Get ActorAccessible for the focused actor
    auto* accessible = Dali::Accessibility::ActorAccessible::Get(newFocused);
    if(!accessible) return;

    // 2. Create DirectNodeProxy via factory
    auto proxy = GetOrCreateProxy(accessible);
    if(!proxy) return;

    // 3. Set currentNode in the service
    mService->highlightNode(proxy);

    // 4. Dispatch STATE_CHANGED(focused) event
    ::Accessibility::AccessibilityEvent event;
    event.type    = ::Accessibility::AccessibilityEvent::Type::STATE_CHANGED;
    event.detail  = "focused";
    event.detail1 = 1;
    mService->dispatchEvent(event);

    fprintf(stdout, "[Focus] %s (%s)\n",
            accessible->GetName().c_str(),
            accessible->GetRoleName().c_str());
  }

  /**
   * @brief FocusedActorEnterKeySignal: activate the currently focused element.
   *
   * DALi's KeyboardFocusManager already triggers DoAction("activate") on the
   * focused control, which fires ClickedSignal for buttons/checkboxes.
   * We only log here — actual state changes are handled by ClickedSignal callbacks.
   */
  void OnEnterPressed(Actor actor)
  {
    if(!actor) return;

    std::string name = actor.GetProperty<std::string>(Actor::Property::NAME);
    fprintf(stdout, "[Activate] %s\n", name.c_str());
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

    // Print tree
    if(keyName == "t")
    {
      fprintf(stdout, "\n=== Accessibility Tree ===\n");
      PrintTree(mRootAccessible, 0);
      fprintf(stdout, "==========================\n\n");
      return;
    }

    // All other keys (arrows, Enter) are handled by KeyboardFocusManager
  }

  /**
   * @brief Gets or creates a DirectNodeProxy for the given accessible.
   */
  std::shared_ptr<DirectNodeProxy> GetOrCreateProxy(::Accessibility::Accessible* accessible)
  {
    if(!accessible) return nullptr;

    auto it = mProxyCache.find(accessible);
    if(it != mProxyCache.end())
    {
      auto locked = it->second.lock();
      if(locked) return locked;
    }

    auto proxy = std::make_shared<DirectNodeProxy>(accessible, mProxyFactory);
    mProxyCache[accessible] = proxy;
    return proxy;
  }

  Application&                                               mApplication;
  std::unique_ptr<::Accessibility::TvScreenReaderService>    mService;
  DirectAppRegistry*                                         mRegistry{nullptr};
  ::Accessibility::Accessible*                               mRootAccessible{nullptr};
  TextLabel                                                  mTitle;
  TextLabel                                                  mStatusLabel;
  ProgressBar                                                mVolumeBar;
  PushButton                                                 mPlayBtn;
  PushButton                                                 mStopBtn;
  CheckBoxButton                                             mAutoplayCheck;
  std::vector<Actor>                                         mFocusOrder;

  // Proxy cache for DirectNodeProxy instances
  std::unordered_map<::Accessibility::Accessible*, std::weak_ptr<DirectNodeProxy>> mProxyCache;
  DirectNodeProxy::ProxyFactory mProxyFactory{[this](::Accessibility::Accessible* acc) -> std::shared_ptr<DirectNodeProxy>
  {
    return GetOrCreateProxy(acc);
  }};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv, "", Application::OPAQUE, PositionSize(0, 0, 480, 800));

  ScreenReaderTvDemo demo(application);

  application.MainLoop();

  return 0;
}
