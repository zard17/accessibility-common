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

// EXTERNAL INCLUDES
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unordered_map>

// INTERNAL INCLUDES
#include <accessibility/api/log.h>
#include <accessibility/api/accessible.h>
#include <accessibility/internal/bridge/accessibility-common.h>
#ifdef ENABLE_TIDL_BACKEND
#include <accessibility/internal/bridge/tidl/tidl-transport-factory.h>
#else
#include <accessibility/internal/bridge/dbus/dbus-transport-factory.h>
#endif
#include <accessibility/internal/bridge/bridge-accessible.h>
#include <accessibility/internal/bridge/bridge-action.h>
#include <accessibility/internal/bridge/bridge-application.h>
#include <accessibility/internal/bridge/bridge-collection.h>
#include <accessibility/internal/bridge/bridge-component.h>
#include <accessibility/internal/bridge/bridge-editable-text.h>
#include <accessibility/internal/bridge/bridge-hyperlink.h>
#include <accessibility/internal/bridge/bridge-hypertext.h>
#include <accessibility/internal/bridge/bridge-object.h>
#include <accessibility/internal/bridge/bridge-selection.h>
#include <accessibility/internal/bridge/bridge-socket.h>
#include <accessibility/internal/bridge/bridge-text.h>
#include <accessibility/internal/bridge/bridge-value.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <accessibility/internal/bridge/dummy/dummy-atspi.h>

// Environment variable names (previously from dali-adaptor internal header)
#define DALI_ENV_DISABLE_ATSPI "DALI_DISABLE_ATSPI"
#define DALI_ENV_SUPPRESS_SCREEN_READER "DALI_SUPPRESS_SCREEN_READER"

using namespace Accessibility;

namespace // unnamed namespace
{
const int RETRY_INTERVAL = 1000;
} // unnamed namespace

/**
 * @brief The BridgeImpl class is to implement some Bridge functions.
 */
class BridgeImpl : public virtual BridgeBase,
                   public BridgeAccessible,
                   public BridgeObject,
                   public BridgeComponent,
                   public BridgeCollection,
                   public BridgeAction,
                   public BridgeValue,
                   public BridgeText,
                   public BridgeEditableText,
                   public BridgeSelection,
                   public BridgeApplication,
                   public BridgeHypertext,
                   public BridgeHyperlink,
                   public BridgeSocket
{
  std::unique_ptr<Ipc::AccessibilityStatusMonitor>               mStatusMonitor;
  std::unique_ptr<Ipc::KeyEventForwarder>                       mKeyEventForwarder;
  std::unique_ptr<Ipc::DirectReadingClient>                     mDirectReadingClient;
  bool                                                          mIsScreenReaderEnabled{false};
  bool                                                          mIsEnabled{false};
  bool                                                          mIsApplicationRunning{false};
  std::unordered_map<int32_t, std::function<void(std::string)>> mDirectReadingCallbacks{};
  uint32_t                                                      mIdleHandle{0};
  RepeatingTimer                                                mInitializeTimer;
  RepeatingTimer                                                mReadIsEnabledTimer;
  RepeatingTimer                                                mReadScreenReaderEnabledTimer;
  RepeatingTimer                                                mForceUpTimer;
  std::string                                                   mPreferredBusName;
  std::map<uint32_t, std::shared_ptr<Accessible>>               mAccessibles; // Actor.ID to Accessible map

public:
  BridgeImpl()
  {
#ifdef ENABLE_TIDL_BACKEND
    mTransportFactory = std::make_unique<Ipc::TidlTransportFactory>();
#else
    mTransportFactory = std::make_unique<Ipc::DbusTransportFactory>();
#endif
  }
  ~BridgeImpl()
  {
    mBridgeTerminated = true;
    try
    {
      TerminateInternal();
    }
    catch(...)
    {
      // Do nothing.
    }
  }

  /**
   * @copydoc Accessibility::Bridge::AddAccessible()
   */
  bool AddAccessible(uint32_t actorId, std::shared_ptr<Accessible> accessible) override
  {
    mAccessibles[actorId] = std::move(accessible);
    return true;
  }

  /**
   * @copydoc Accessibility::Bridge::RemoveAccessible()
   */
  void RemoveAccessible(uint32_t actorId) override
  {
    mAccessibles.erase(actorId);
  }

  /**
   * @copydoc Accessibility::Bridge::GetAccessible()
   */
  std::shared_ptr<Accessible> GetAccessible(uint32_t objectId) const override
  {
    auto iter = mAccessibles.find(objectId);
    return iter != mAccessibles.end() ? iter->second : nullptr;
  }

  /**
   * @copydoc Accessibility::Bridge::GetAccessible()
   */
  std::shared_ptr<Accessible> GetAccessible(const std::string& path) const override
  {
    try
    {
      uint32_t actorId = static_cast<uint32_t>(std::stoi(path));
      auto     iter    = mAccessibles.find(actorId);
      return iter != mAccessibles.end() ? iter->second : nullptr;
    }
    catch(const std::invalid_argument& ia)
    {
      // Handle invalid argument (e.g., non-numeric characters in the string)
      throw std::runtime_error("Invalid argument: string is not a valid integer");
    }
    catch(const std::out_of_range& oor)
    {
      // Handle out of range (e.g., the number is too large for uint32_t)
      throw std::runtime_error("Out of range: number is too large for uint32_t");
    }
  }

  /**
   * @copydoc Accessibility::Bridge::ShouldIncludeHidden()
   */
  bool ShouldIncludeHidden() const override
  {
    if(auto application = mApplication->GetFeature<Application>())
    {
      return application->GetIncludeHidden();
    }
    return false;
  }

  void NotifyIncludeHiddenChanged() override
  {
    for(const auto& iter : mAccessibles)
    {
      const auto& accessible = iter.second;
      if(accessible->IsHidden())
      {
        auto* parent = accessible->GetParent();
        if(parent)
        {
          // Emit children-changed event so AT-SPI clients refresh the subtree
          // Non-owning shared_ptr to satisfy the interface that expects shared_ptr
          Emit(std::shared_ptr<Accessible>(std::shared_ptr<Accessible>{}, parent), ObjectPropertyChangeEvent::PARENT);
        }
      }
    }
  }

  /**
   * @copydoc Accessibility::Bridge::EmitKeyEvent()
   */
  bool EmitKeyEvent(Accessibility::KeyEvent keyEvent, std::function<void(Accessibility::KeyEvent, bool)> callback) override
  {
    if(!IsUp() || !mIpcServer || !mKeyEventForwarder)
    {
      return false;
    }

    uint32_t keyType   = (keyEvent.state == Accessibility::KeyEvent::State::DOWN ? 0U : 1U);
    auto     timeStamp = static_cast<std::int32_t>(keyEvent.time);
    bool     isText    = !keyEvent.keyString.empty();

    mKeyEventForwarder->notifyListenersSync(
      keyType, keyEvent.keyCode, timeStamp, keyEvent.keyName, isText,
      [keyEvent = std::move(keyEvent), callback = std::move(callback)](Ipc::ValueOrError<bool> reply)
    {
      bool consumed = false;

      if(!reply)
      {
        ACCESSIBILITY_LOG_ERROR("NotifyListenersSync call failed: %s", reply.getError().message.c_str());
      }
      else
      {
        consumed = std::get<0>(reply.getValues());
      }

      callback(std::move(keyEvent), consumed);
    });

    return true;
  }

  /**
   * @copydoc Accessibility::Bridge::Pause()
   */
  void Pause() override
  {
    if(!IsUp() || !mIpcServer || !mDirectReadingClient)
    {
      return;
    }

    mDirectReadingClient->pauseResume(true, [](Ipc::ValueOrError<void> msg)
    {
      if(!msg)
      {
        LOG() << "Direct reading command failed (" << msg.getError().message << ")\n";
      }
    });
  }

  /**
   * @copydoc Accessibility::Bridge::Resume()
   */
  void Resume() override
  {
    if(!IsUp() || !mIpcServer || !mDirectReadingClient)
    {
      return;
    }

    mDirectReadingClient->pauseResume(false, [](Ipc::ValueOrError<void> msg)
    {
      if(!msg)
      {
        LOG() << "Direct reading command failed (" << msg.getError().message << ")\n";
      }
    });
  }

  /**
   * @copydoc Accessibility::Bridge::StopReading()
   */
  void StopReading(bool alsoNonDiscardable) override
  {
    if(!IsUp() || !mIpcServer || !mDirectReadingClient)
    {
      return;
    }

    mDirectReadingClient->stopReading(alsoNonDiscardable, [](Ipc::ValueOrError<void> msg)
    {
      if(!msg)
      {
        LOG() << "Direct reading command failed (" << msg.getError().message << ")\n";
      }
    });
  }

  /**
   * @copydoc Accessibility::Bridge::Say()
   */
  void Say(const std::string& text, bool discardable, std::function<void(std::string)> callback) override
  {
    if(!IsUp() || !mIpcServer || !mDirectReadingClient)
    {
      return;
    }

    mDirectReadingClient->readCommand(text, discardable, [this, callback](Ipc::ValueOrError<std::string, bool, int32_t> msg)
    {
      if(!msg)
      {
        LOG() << "Direct reading command failed (" << msg.getError().message << ")\n";
      }
      else if(callback)
      {
        mDirectReadingCallbacks.emplace(std::get<2>(msg), callback);
      }
    });
  }

  /**
   * @copydoc Accessibility::Bridge::ForceDown()
   */
  void ForceDown() override
  {
    if(mData)
    {
      mData->mCurrentlyHighlightedAccessible = nullptr;

      mDisabledSignal.Emit();

      if(mIpcServer)
      {
        UnembedSocket(mApplication->GetAddress(), {AtspiDbusNameRegistry, "root"});
        ReleaseBusName(mPreferredBusName);
      }
    }

    BridgeAccessible::ForceDown();
    mKeyEventForwarder.reset();
    mDirectReadingClient.reset();
    mDirectReadingCallbacks.clear();
    mApplication->mChildren.clear();
    ClearTimer();
  }

  void ClearTimer()
  {
    mInitializeTimer.Stop();
    mReadIsEnabledTimer.Stop();
    mReadScreenReaderEnabledTimer.Stop();
    mForceUpTimer.Stop();
  }
  /**
   * @copydoc Accessibility::Bridge::Terminate()
   */
  void Terminate() override
  {
    TerminateInternal();
  }

  // Seperated method that we can call at constructor/destructor (to avoid pure virtual method exception)
  void TerminateInternal()
  {
    if(ACCESSIBILITY_UNLIKELY(mTerminateFunctionCalled))
    {
      // Skip terminate function if it called twice.
      return;
    }
    mTerminateFunctionCalled = true;

    if(mData)
    {
      // The ~Window() after this point cannot emit DESTROY, because Bridge is not available. So emit DESTROY here.
      for(auto windowAccessible : mApplication->mChildren)
      {
        BridgeObject::Emit(windowAccessible, WindowEvent::DESTROY);
      }
      mData->mCurrentlyHighlightedAccessible = nullptr;
    }
    mAccessibles.clear();
    ForceDown();
    auto& platformCallbacks = Accessibility::GetPlatformCallbacks();
    if((0 != mIdleHandle) && platformCallbacks.isAdaptorAvailable && platformCallbacks.isAdaptorAvailable())
    {
      if(platformCallbacks.removeIdle)
      {
        platformCallbacks.removeIdle(mIdleHandle);
      }
    }
    mIdleHandle = 0;
    mStatusMonitor.reset();
    mIpcServer.reset();
  }

  bool ForceUpTimerCallback()
  {
    if(ForceUp() != ForceUpResult::FAILED)
    {
      return false;
    }
    return true;
  }

  /**
   * @copydoc Accessibility::Bridge::ForceUp()
   */
  ForceUpResult ForceUp() override
  {
    auto forceUpResult = BridgeAccessible::ForceUp();
    if(forceUpResult == ForceUpResult::ALREADY_UP)
    {
      return forceUpResult;
    }
    else if(forceUpResult == ForceUpResult::FAILED)
    {
      if(!mForceUpTimer)
      {
        mForceUpTimer.Start(RETRY_INTERVAL, [this]() { return ForceUpTimerCallback(); });
      }
      return forceUpResult;
    }

    // IPC-dependent setup: only when transport is available
    if(mIpcServer)
    {
      BridgeObject::RegisterInterfaces();
      BridgeAccessible::RegisterInterfaces();
      BridgeComponent::RegisterInterfaces();
      BridgeCollection::RegisterInterfaces();
      BridgeAction::RegisterInterfaces();
      BridgeValue::RegisterInterfaces();
      BridgeText::RegisterInterfaces();
      BridgeEditableText::RegisterInterfaces();
      BridgeSelection::RegisterInterfaces();
      BridgeApplication::RegisterInterfaces();
      BridgeHypertext::RegisterInterfaces();
      BridgeHyperlink::RegisterInterfaces();
      BridgeSocket::RegisterInterfaces();

      mKeyEventForwarder  = mTransportFactory->createKeyEventForwarder(*mIpcServer);
      mDirectReadingClient = mTransportFactory->createDirectReadingClient(*mIpcServer);

      mDirectReadingClient->listenReadingStateChanged([this](int32_t id, std::string readingState)
      {
        auto it = mDirectReadingCallbacks.find(id);
        if(it != mDirectReadingCallbacks.end())
        {
          it->second(readingState);
          if(readingState != "ReadingPaused" && readingState != "ReadingResumed" && readingState != "ReadingStarted")
          {
            mDirectReadingCallbacks.erase(it);
          }
        }
      });

      RequestBusName(mPreferredBusName);

      auto parentAddress = EmbedSocket(mApplication->GetAddress(), {AtspiDbusNameRegistry, "root"});
      if(auto proxyAccessible = dynamic_cast<ProxyAccessible*>(mApplication->GetParent()))
      {
        proxyAccessible->SetAddress(std::move(parentAddress));
      }
    }

    mEnabledSignal.Emit();

    return ForceUpResult::JUST_STARTED;
  }

  /**
   * @brief Sends a signal to dbus that the window is created.
   *
   * @param[in] windowRoot The window root accessible
   * @see BridgeObject::Emit()
   */
  void EmitCreated(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      Emit(windowRoot, WindowEvent::CREATE, 0);
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is shown.
   *
   * @param[in] windowRoot The window root accessible
   * @see Accessible::EmitShowing() and BridgeObject::EmitStateChanged()
   */
  void EmitShown(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      EmitStateChanged(std::shared_ptr<Accessible>(std::shared_ptr<Accessible>{}, windowRoot), State::SHOWING, 1, 0);
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is hidden.
   *
   * @param[in] windowRoot The window root accessible
   * @see Accessible::EmitShowing() and BridgeObject::EmitStateChanged()
   */
  void EmitHidden(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      EmitStateChanged(std::shared_ptr<Accessible>(std::shared_ptr<Accessible>{}, windowRoot), State::SHOWING, 0, 0);
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is activated.
   *
   * @param[in] windowRoot The window root accessible
   * @see BridgeObject::Emit()
   */
  void EmitActivate(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      Emit(windowRoot, WindowEvent::ACTIVATE, 0);
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is deactivated.
   *
   * @param[in] windowRoot The window root accessible
   * @see BridgeObject::Emit()
   */
  void EmitDeactivate(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      Emit(windowRoot, WindowEvent::DEACTIVATE, 0);
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is minimized.
   *
   * @param[in] windowRoot The window root accessible
   * @see BridgeObject::Emit()
   */
  void EmitMinimize(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      Emit(windowRoot, WindowEvent::MINIMIZE, 0);
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is restored.
   *
   * @param[in] windowRoot The window root accessible
   * @param[in] detail Restored window state
   * @see BridgeObject::Emit()
   */
  void EmitRestore(Accessible* windowRoot, Accessibility::WindowRestoreType detail)
  {
    if(windowRoot)
    {
      Emit(windowRoot, WindowEvent::RESTORE, static_cast<unsigned int>(detail));
    }
  }

  /**
   * @brief Sends a signal to dbus that the window is maximized.
   *
   * @param[in] windowRoot The window root accessible
   * @see BridgeObject::Emit()
   */
  void EmitMaximize(Accessible* windowRoot)
  {
    if(windowRoot)
    {
      Emit(windowRoot, WindowEvent::MAXIMIZE, 0);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowCreated()
   */
  void WindowCreated(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitCreated(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowShown()
   */
  void WindowShown(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitShown(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowHidden()
   */
  void WindowHidden(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitHidden(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowFocused()
   */
  void WindowFocused(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitActivate(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowUnfocused()
   */
  void WindowUnfocused(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitDeactivate(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowMinimized()
   */
  void WindowMinimized(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitMinimize(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowRestored()
   */
  void WindowRestored(Accessible* windowRoot, WindowRestoreType detail) override
  {
    if(IsUp())
    {
      EmitRestore(windowRoot, detail);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::WindowMaximized()
   */
  void WindowMaximized(Accessible* windowRoot) override
  {
    if(IsUp())
    {
      EmitMaximize(windowRoot);
    }
  }

  /**
   * @copydoc Accessibility::Bridge::ApplicationPaused()
   */
  void ApplicationPaused() override
  {
    mIsApplicationRunning = false;
    SwitchBridge();
  }

  /**
   * @copydoc Accessibility::Bridge::ApplicationResumed()
   */
  void ApplicationResumed() override
  {
    mIsApplicationRunning = true;
    SwitchBridge();
  }

  /**
   * @copydoc Accessibility::Bridge::SuppressScreenReader()
   */
  void SuppressScreenReader(bool suppress) override
  {
    if(mIsScreenReaderSuppressed == suppress)
    {
      return;
    }
    mIsScreenReaderSuppressed = suppress;
    ReadScreenReaderEnabledProperty();
  }

  void SwitchBridge()
  {
    if(ACCESSIBILITY_UNLIKELY(mTerminateFunctionCalled))
    {
      // fast-out for terminate case.
      return;
    }

    //If DBusClient is not ready, don't remove initialize timer.
    if(mInitializeTimer.IsRunning())
    {
      return;
    }

    bool isScreenReaderEnabled = mIsScreenReaderEnabled && !mIsScreenReaderSuppressed;

    if((isScreenReaderEnabled || mIsEnabled) && mIsApplicationRunning)
    {
      ForceUp();
    }
    else
    {
      ForceDown();
    }
  }

  bool ReadIsEnabledTimerCallback()
  {
    ReadIsEnabledProperty();
    return false;
  }

  void ReadIsEnabledProperty()
  {
    mStatusMonitor->readIsEnabled([this](Ipc::ValueOrError<bool> msg)
    {
      if(ACCESSIBILITY_UNLIKELY(mTerminateFunctionCalled))
      {
        // fast-out for terminate case.
        return;
      }

      if(!msg)
      {
        ACCESSIBILITY_LOG_ERROR("Get IsEnabled property error: %s\n", msg.getError().message.c_str());
        if(msg.getError().errorType == Ipc::ErrorType::INVALID_REPLY)
        {
          mReadIsEnabledTimer.Start(RETRY_INTERVAL, [this]() { return ReadIsEnabledTimerCallback(); });
        }
        return;
      }

      mReadIsEnabledTimer.Stop();

      mIsEnabled = std::get<0>(msg);
      SwitchBridge();
    });
  }

  void ListenIsEnabledProperty()
  {
    mStatusMonitor->listenIsEnabled([this](bool res)
    {
      mIsEnabled = res;
      SwitchBridge();
    });
  }

  bool ReadScreenReaderEnabledTimerCallback()
  {
    ReadScreenReaderEnabledProperty();
    return false;
  }

  void ReadScreenReaderEnabledProperty()
  {
    // can be true because of SuppressScreenReader before init
    if(!mStatusMonitor)
    {
      return;
    }

    mStatusMonitor->readScreenReaderEnabled([this](Ipc::ValueOrError<bool> msg)
    {
      if(ACCESSIBILITY_UNLIKELY(mTerminateFunctionCalled))
      {
        // fast-out for terminate case.
        return;
      }

      if(!msg)
      {
        ACCESSIBILITY_LOG_ERROR("Get ScreenReaderEnabled property error: %s\n", msg.getError().message.c_str());
        if(msg.getError().errorType == Ipc::ErrorType::INVALID_REPLY)
        {
          mReadScreenReaderEnabledTimer.Start(RETRY_INTERVAL, [this]() { return ReadScreenReaderEnabledTimerCallback(); });
        }
        return;
      }

      mReadScreenReaderEnabledTimer.Stop();

      mIsScreenReaderEnabled = std::get<0>(msg);
      SwitchBridge();
    });
  }

  void EmitScreenReaderEnabledSignal()
  {
    if(mIsScreenReaderEnabled)
    {
      mScreenReaderEnabledSignal.Emit();
    }
    else
    {
      mScreenReaderDisabledSignal.Emit();
    }
  }

  void ListenScreenReaderEnabledProperty()
  {
    mStatusMonitor->listenScreenReaderEnabled([this](bool res)
    {
      mIsScreenReaderEnabled = res;
      EmitScreenReaderEnabledSignal();
      SwitchBridge();
    });
  }

  void ReadAndListenProperties()
  {
    ReadIsEnabledProperty();
    ListenIsEnabledProperty();

    ReadScreenReaderEnabledProperty();
    ListenScreenReaderEnabledProperty();
  }

  bool InitializeAccessibilityStatusClient()
  {
    if(!mTransportFactory || !mTransportFactory->isAvailable())
    {
      return false;
    }

    mStatusMonitor = mTransportFactory->createStatusMonitor();

    if(!mStatusMonitor || !*mStatusMonitor)
    {
      ACCESSIBILITY_LOG_ERROR("Accessibility Status Monitor is not ready\n");
      mStatusMonitor.reset();
      return false;
    }

    return true;
  }

  bool InitializeTimerCallback()
  {
    if(InitializeAccessibilityStatusClient())
    {
      ReadAndListenProperties();
      return false;
    }
    return true;
  }

  bool OnIdleSignal()
  {
    if(InitializeAccessibilityStatusClient())
    {
      ReadAndListenProperties();
      mIdleHandle = 0;
      return false;
    }

    mInitializeTimer.Start(RETRY_INTERVAL, [this]() { return InitializeTimerCallback(); });

    mIdleHandle = 0;
    return false;
  }

  /**
   * @copydoc Accessibility::Bridge::Initialize()
   */
  void Initialize() override
  {
    if(InitializeAccessibilityStatusClient())
    {
      ReadAndListenProperties();
      return;
    }

    // No IPC transport — enable accessibility locally
    if(!mTransportFactory || !mTransportFactory->isAvailable())
    {
      mIsEnabled            = true;
      mIsApplicationRunning = true;
      SwitchBridge();
      return;
    }

    // Initialize failed. Try it again on Idle
    auto& platformCallbacks = Accessibility::GetPlatformCallbacks();
    if(platformCallbacks.isAdaptorAvailable && platformCallbacks.isAdaptorAvailable())
    {
      if(0 == mIdleHandle)
      {
        if(platformCallbacks.addIdle)
        {
          mIdleHandle = platformCallbacks.addIdle([this]() { return OnIdleSignal(); });
          if(ACCESSIBILITY_UNLIKELY(0 == mIdleHandle))
          {
            ACCESSIBILITY_LOG_ERROR("Fail to add idle callback for bridge initialize. Call it synchronously.\n");
            OnIdleSignal();
          }
        }
      }
    }
  }

  /**
   * @copydoc Accessibility::Bridge::GetScreenReaderEnabled()
   */
  bool GetScreenReaderEnabled() override
  {
    return mIsScreenReaderEnabled;
  }

  /**
   * @copydoc Accessibility::Bridge::IsEnabled()
   */
  bool IsEnabled() override
  {
    return mIsEnabled;
  }

  Address EmbedSocket(const Address& plug, const Address& socket) override
  {
    if(!mIpcServer || !mTransportFactory) return {};

    auto client = mTransportFactory->createSocketClient(socket, *mIpcServer);
    auto reply  = client->embed(plug);

    if(!reply)
    {
      ACCESSIBILITY_LOG_ERROR("Failed to embed socket %s: %s", socket.ToString().c_str(), reply.getError().message.c_str());
      return {};
    }

    return std::get<0>(reply.getValues());
  }

  void UnembedSocket(const Address& plug, const Address& socket) override
  {
    if(!mIpcServer || !mTransportFactory) return;

    auto client = mTransportFactory->createSocketClient(socket, *mIpcServer);
    client->unembed(plug, [](Ipc::ValueOrError<void>) {});
  }

  void SetSocketOffset(ProxyAccessible* socket, std::int32_t x, std::int32_t y) override
  {
    if(!mIpcServer || !mTransportFactory) return;

    AddCoalescableMessage(CoalescableMessages::SET_OFFSET, socket, 1.0f, [this, socket, x, y]()
    {
      auto client = mTransportFactory->createSocketClient(socket->GetAddress(), *mIpcServer);
      client->setOffset(x, y, [](Ipc::ValueOrError<void>) {});
    });
  }

  void SetExtentsOffset(std::int32_t x, std::int32_t y) override
  {
    if(mData)
    {
      mData->mExtentsOffset = {x, y};
    }
  }

  void SetPreferredBusName(std::string_view preferredBusName) override
  {
    if(preferredBusName == mPreferredBusName)
    {
      return;
    }

    std::string oldPreferredBusName = std::move(mPreferredBusName);
    mPreferredBusName               = std::string{preferredBusName};

    if(IsUp() && mIpcServer)
    {
      ReleaseBusName(oldPreferredBusName);
      RequestBusName(mPreferredBusName);
    }
    // else: request/release will be handled by ForceUp/ForceDown, respectively
  }

private:
  void RequestBusName(const std::string& busName)
  {
    if(busName.empty() || !mTransportFactory || !mIpcServer)
    {
      return;
    }

    mTransportFactory->requestBusName(*mIpcServer, busName);
  }

  void ReleaseBusName(const std::string& busName)
  {
    if(busName.empty() || !mTransportFactory || !mIpcServer)
    {
      return;
    }

    mTransportFactory->releaseBusName(*mIpcServer, busName);
  }

  bool mTerminateFunctionCalled{false};
}; // BridgeImpl

namespace // unnamed namespace
{
static bool INITIALIZED_BRIDGE = false;

/**
 * @brief Creates BridgeImpl instance.
 *
 * @return The BridgeImpl instance
 * @note This method is to check environment variable first. If ATSPI is disable using env, it returns dummy bridge instance.
 */
std::shared_ptr<Bridge> CreateBridge()
{
  INITIALIZED_BRIDGE = true;

  try
  {
    /* check environment variable first */
    const char* envAtspiDisabled = std::getenv(DALI_ENV_DISABLE_ATSPI);
    if(envAtspiDisabled && std::atoi(envAtspiDisabled) != 0)
    {
      ACCESSIBILITY_LOG_DEBUG_INFO("AT-SPI Disabled. Return dummy instance\n");
      return Accessibility::DummyBridge::GetInstance();
    }

    return std::make_shared<BridgeImpl>();
  }
  catch(const std::exception&)
  {
    ACCESSIBILITY_LOG_ERROR("Failed to initialize AT-SPI bridge");
    return Accessibility::DummyBridge::GetInstance();
  }
}

} // unnamed namespace

// Accessibility::Bridge class implementation

std::shared_ptr<Bridge> Bridge::GetCurrentBridge()
{
  static std::shared_ptr<Bridge> bridge;

  // Guard rare case that call this API after Bridge destructor.
  // (Since static bridge didn't be nullptr at static variables destroy case.)
  if(ACCESSIBILITY_UNLIKELY(Accessibility::Bridge::IsTerminated()))
  {
    ACCESSIBILITY_LOG_ERROR("Bridge destroyed! It is static destructor case. So their is no valid bridge anymore. Return nullptr instead\n");
    return nullptr;
  }

  if(bridge)
  {
    return bridge;
  }
  else if(mAutoInitState == AutoInitState::ENABLED)
  {
    bridge = CreateBridge();

    /* check environment variable for suppressing screen-reader */
    const char* envSuppressScreenReader = std::getenv(DALI_ENV_SUPPRESS_SCREEN_READER);
    if(envSuppressScreenReader && std::atoi(envSuppressScreenReader) != 0)
    {
      bridge->SuppressScreenReader(true);
    }

    return bridge;
  }
  ACCESSIBILITY_LOG_DEBUG_INFO("Bridge::DisableAutoInit() called. Return dummy instance\n");

  return Accessibility::DummyBridge::GetInstance();
}

void Bridge::DisableAutoInit()
{
  if(INITIALIZED_BRIDGE)
  {
    ACCESSIBILITY_LOG_ERROR("Bridge::DisableAutoInit() called after bridge auto-initialization");
  }

  mAutoInitState = AutoInitState::DISABLED;
}

void Bridge::EnableAutoInit()
{
  mAutoInitState = AutoInitState::ENABLED;

  if(INITIALIZED_BRIDGE)
  {
    return;
  }

  // Delegate the platform-specific initialization to the registered callback.
  // dali-adaptor registers this callback to perform Stage/Window/Adaptor work.
  auto& callbacks = GetPlatformCallbacks();
  if(callbacks.onEnableAutoInit)
  {
    callbacks.onEnableAutoInit();
  }
}

std::string Bridge::MakeBusNameForWidget(std::string_view widgetInstanceId, int widgetProcessId)
{
  // The bus name should consist of dot-separated alphanumeric elements, e.g. "com.example.BusName123".
  // Allowed characters in each element: "[A-Z][a-z][0-9]_-", but no element may start with a digit.

  static const char prefix[]   = "elm.atspi.proxy.socket-";
  static const char underscore = '_';

  std::stringstream tmp;

  tmp << prefix;

  for(char ch : widgetInstanceId)
  {
    tmp << (!std::isalnum(ch) && ch != '_' && ch != '-' && ch != '.' ? underscore : ch);
  }

  tmp << '-' << widgetProcessId;

  return tmp.str();
}
