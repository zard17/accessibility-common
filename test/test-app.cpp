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
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/api/accessible.h>
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <test/mock/mock-dbus-wrapper.h>
#include <test/test-accessible.h>

// Test framework
static int gPassCount = 0;
static int gFailCount = 0;

#define TEST_CHECK(condition, name)                                   \
  do                                                                  \
  {                                                                   \
    if(condition)                                                     \
    {                                                                 \
      ++gPassCount;                                                   \
      std::cout << "  PASS: " << (name) << std::endl;                \
    }                                                                 \
    else                                                              \
    {                                                                 \
      ++gFailCount;                                                   \
      std::cerr << "  FAIL: " << (name) << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
    }                                                                 \
  } while(0)

/**
 * @brief Makes a D-Bus object path for the given accessible ID.
 */
static std::string MakeObjectPath(uint32_t id)
{
  return std::string{ATSPI_PREFIX_PATH} + std::to_string(id);
}

/**
 * @brief Creates a DBusClient pointing to a specific accessible object in the bridge.
 */
static DBus::DBusClient CreateAccessibleClient(const std::string& busName, uint32_t accessibleId, const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{
    busName,
    MakeObjectPath(accessibleId),
    Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE),
    conn};
}

/**
 * @brief Creates a DBusClient for the Component interface.
 */
static DBus::DBusClient CreateComponentClient(const std::string& busName, uint32_t accessibleId, const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{
    busName,
    MakeObjectPath(accessibleId),
    Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::COMPONENT),
    conn};
}

int main(int argc, char** argv)
{
  std::cout << "=== Accessibility Mock D-Bus Test ===" << std::endl;

  // ===== Step 1: Install MockDBusWrapper =====
  std::cout << "\n[1] Installing MockDBusWrapper..." << std::endl;
  auto mockWrapper = std::make_unique<MockDBusWrapper>();
  auto* mockPtr = mockWrapper.get();
  (void)mockPtr; // for potential later use
  DBusWrapper::Install(std::move(mockWrapper));
  std::cout << "  MockDBusWrapper installed." << std::endl;

  // ===== Step 2: Set PlatformCallbacks =====
  std::cout << "\n[2] Setting PlatformCallbacks..." << std::endl;
  Accessibility::PlatformCallbacks callbacks;
  callbacks.addIdle = [](std::function<bool()> cb) -> uint32_t {
    // Execute immediately and return 0 (no handle management needed)
    if(cb) cb();
    return 1;
  };
  callbacks.removeIdle = [](uint32_t) {};
  callbacks.getToolkitVersion = []() -> std::string { return "mock-1.0.0"; };
  callbacks.getAppName = []() -> std::string { return "test-app"; };
  callbacks.isAdaptorAvailable = []() -> bool { return true; };
  callbacks.onEnableAutoInit = []() {};
  callbacks.createTimer = [](uint32_t intervalMs, std::function<bool()> cb) -> uint32_t {
    // Execute once immediately for test purposes
    if(cb) cb();
    return 1;
  };
  callbacks.cancelTimer = [](uint32_t) {};
  callbacks.isTimerRunning = [](uint32_t) -> bool { return false; };
  Accessibility::SetPlatformCallbacks(callbacks);
  std::cout << "  PlatformCallbacks set." << std::endl;

  // ===== Step 3: Create accessibility tree =====
  std::cout << "\n[3] Creating accessibility tree..." << std::endl;

  auto window = std::make_shared<TestAccessible>("TestWindow", Accessibility::Role::WINDOW);
  auto panel  = std::make_shared<TestAccessible>("Panel", Accessibility::Role::PANEL);
  auto button = std::make_shared<TestAccessible>("OK", Accessibility::Role::PUSH_BUTTON);
  auto label  = std::make_shared<TestAccessible>("Hello World", Accessibility::Role::LABEL);

  Accessibility::States buttonStates;
  buttonStates[Accessibility::State::ENABLED]       = true;
  buttonStates[Accessibility::State::SENSITIVE]      = true;
  buttonStates[Accessibility::State::VISIBLE]        = true;
  buttonStates[Accessibility::State::SHOWING]        = true;
  buttonStates[Accessibility::State::FOCUSABLE]      = true;
  button->SetStates(buttonStates);
  button->SetExtents({10.0f, 20.0f, 200.0f, 50.0f});

  Accessibility::States labelStates;
  labelStates[Accessibility::State::ENABLED]  = true;
  labelStates[Accessibility::State::VISIBLE]  = true;
  labelStates[Accessibility::State::SHOWING]  = true;
  label->SetStates(labelStates);
  label->SetExtents({10.0f, 80.0f, 300.0f, 30.0f});

  Accessibility::States windowStates;
  windowStates[Accessibility::State::ENABLED] = true;
  windowStates[Accessibility::State::VISIBLE] = true;
  windowStates[Accessibility::State::SHOWING] = true;
  windowStates[Accessibility::State::ACTIVE]  = true;
  window->SetStates(windowStates);
  window->SetExtents({0.0f, 0.0f, 480.0f, 800.0f});

  panel->AddChild(button);
  panel->AddChild(label);
  window->AddChild(panel);

  std::cout << "  Tree created:" << std::endl;
  std::cout << "    window (WINDOW, id=" << window->GetId() << ")" << std::endl;
  std::cout << "      panel (PANEL, id=" << panel->GetId() << ")" << std::endl;
  std::cout << "        button (PUSH_BUTTON, id=" << button->GetId() << ", name='OK')" << std::endl;
  std::cout << "        label (LABEL, id=" << label->GetId() << ", name='Hello World')" << std::endl;

  // ===== Step 4: Get bridge and configure =====
  std::cout << "\n[4] Getting bridge and configuring..." << std::endl;

  auto bridge = Accessibility::Bridge::GetCurrentBridge();
  TEST_CHECK(bridge != nullptr, "Bridge::GetCurrentBridge() returns non-null");
  if(!bridge)
  {
    std::cerr << "FATAL: Bridge is null, cannot continue." << std::endl;
    return 1;
  }

  bridge->SetApplicationName("test-app");
  bridge->SetToolkitName("mock-toolkit");

  // Register test accessibles with the bridge
  bridge->AddAccessible(window->GetId(), window);
  bridge->AddAccessible(panel->GetId(), panel);
  bridge->AddAccessible(button->GetId(), button);
  bridge->AddAccessible(label->GetId(), label);

  bridge->AddTopLevelWindow(window.get());

  // ===== Step 5: Initialize and force up the bridge =====
  std::cout << "\n[5] Initializing bridge..." << std::endl;

  // Initialize reads status properties (IsEnabled, ScreenReaderEnabled) from mock
  // These return true, which sets mIsEnabled/mIsScreenReaderEnabled
  bridge->Initialize();

  // ApplicationResumed() triggers SwitchBridge() -> ForceUp()
  bridge->ApplicationResumed();

  bool isUp = bridge->IsUp();
  TEST_CHECK(isUp, "Bridge is up after Initialize + ApplicationResumed");
  if(!isUp)
  {
    std::cerr << "FATAL: Bridge is not up, cannot run D-Bus tests." << std::endl;
    std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;
    return gFailCount > 0 ? 1 : 0;
  }

  std::string busName = bridge->GetBusName();
  std::cout << "  Bus name: " << busName << std::endl;
  TEST_CHECK(!busName.empty(), "Bridge has a bus name");

  // Get bridge connection for creating clients
  // Use the wrapper to get a connection (same as what the bridge uses)
  auto conn = DBusWrapper::Installed()->eldbus_address_connection_get_impl("unix:path=/tmp/mock-atspi");

  // ===== Step 6: Test - GetRole via Accessible interface =====
  std::cout << "\n[6] Testing GetRole..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, button->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
    TEST_CHECK(!!result, "GetRole call succeeds for button");
    if(result)
    {
      uint32_t roleVal = std::get<0>(result.getValues());
      TEST_CHECK(roleVal == static_cast<uint32_t>(Accessibility::Role::PUSH_BUTTON),
                 "Button role is PUSH_BUTTON (" + std::to_string(roleVal) + ")");
    }
  }
  {
    auto client = CreateAccessibleClient(busName, label->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
    TEST_CHECK(!!result, "GetRole call succeeds for label");
    if(result)
    {
      uint32_t roleVal = std::get<0>(result.getValues());
      TEST_CHECK(roleVal == static_cast<uint32_t>(Accessibility::Role::LABEL),
                 "Label role is LABEL (" + std::to_string(roleVal) + ")");
    }
  }

  // ===== Step 7: Test - GetName via property =====
  std::cout << "\n[7] Testing Name property..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, button->GetId(), conn);
    auto result = client.property<std::string>("Name").get();
    TEST_CHECK(!!result, "Name property get succeeds for button");
    if(result)
    {
      std::string name = std::get<0>(result.getValues());
      TEST_CHECK(name == "OK", "Button name is 'OK' (got '" + name + "')");
    }
  }
  {
    auto client = CreateAccessibleClient(busName, label->GetId(), conn);
    auto result = client.property<std::string>("Name").get();
    TEST_CHECK(!!result, "Name property get succeeds for label");
    if(result)
    {
      std::string name = std::get<0>(result.getValues());
      TEST_CHECK(name == "Hello World", "Label name is 'Hello World' (got '" + name + "')");
    }
  }

  // ===== Step 8: Test - ChildCount property =====
  std::cout << "\n[8] Testing ChildCount property..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, panel->GetId(), conn);
    auto result = client.property<int>("ChildCount").get();
    TEST_CHECK(!!result, "ChildCount property get succeeds for panel");
    if(result)
    {
      int count = std::get<0>(result.getValues());
      TEST_CHECK(count == 2, "Panel has 2 children (got " + std::to_string(count) + ")");
    }
  }
  {
    auto client = CreateAccessibleClient(busName, window->GetId(), conn);
    auto result = client.property<int>("ChildCount").get();
    TEST_CHECK(!!result, "ChildCount property get succeeds for window");
    if(result)
    {
      int count = std::get<0>(result.getValues());
      TEST_CHECK(count == 1, "Window has 1 child (got " + std::to_string(count) + ")");
    }
  }

  // ===== Step 9: Test - GetState =====
  std::cout << "\n[9] Testing GetState..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, button->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<std::array<uint32_t, 2>>()>("GetState").call();
    TEST_CHECK(!!result, "GetState call succeeds for button");
    if(result)
    {
      auto stateData = std::get<0>(result.getValues());
      Accessibility::States states{stateData};
      TEST_CHECK(states[Accessibility::State::ENABLED], "Button state ENABLED is set");
      TEST_CHECK(states[Accessibility::State::SENSITIVE], "Button state SENSITIVE is set");
      TEST_CHECK(states[Accessibility::State::VISIBLE], "Button state VISIBLE is set");
      TEST_CHECK(states[Accessibility::State::FOCUSABLE], "Button state FOCUSABLE is set");
    }
  }

  // ===== Step 10: Test - GetExtents via Component interface =====
  std::cout << "\n[10] Testing GetExtents..." << std::endl;
  {
    auto client = CreateComponentClient(busName, button->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<std::tuple<int32_t, int32_t, int32_t, int32_t>>(uint32_t)>("GetExtents").call(static_cast<uint32_t>(Accessibility::CoordinateType::SCREEN));
    TEST_CHECK(!!result, "GetExtents call succeeds for button");
    if(result)
    {
      auto extents = std::get<0>(result.getValues());
      auto x = std::get<0>(extents);
      auto y = std::get<1>(extents);
      auto w = std::get<2>(extents);
      auto h = std::get<3>(extents);
      TEST_CHECK(x == 10, "Button extents x=10 (got " + std::to_string(x) + ")");
      TEST_CHECK(y == 20, "Button extents y=20 (got " + std::to_string(y) + ")");
      TEST_CHECK(w == 200, "Button extents w=200 (got " + std::to_string(w) + ")");
      TEST_CHECK(h == 50, "Button extents h=50 (got " + std::to_string(h) + ")");
    }
  }

  // ===== Step 11: Test - FindByPath =====
  std::cout << "\n[11] Testing FindByPath..." << std::endl;
  {
    auto found = bridge->FindByPath(std::to_string(button->GetId()));
    TEST_CHECK(found == button.get(), "FindByPath finds button by ID");
  }
  {
    auto found = bridge->FindByPath("root");
    TEST_CHECK(found != nullptr, "FindByPath finds root");
    TEST_CHECK(found == bridge->GetApplication(), "FindByPath('root') returns application");
  }

  // ===== Step 12: Test - GetChildAtIndex =====
  // Note: GetChildAtIndex returns Accessible* serialized as Address (so).
  // Client-side Accessible* deserialization requires CurrentBridgePtr context,
  // so we verify the returned Address directly.
  std::cout << "\n[12] Testing GetChildAtIndex..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, panel->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(0);
    TEST_CHECK(!!result, "GetChildAtIndex(0) call succeeds for panel");
    if(result)
    {
      auto address = std::get<0>(result.getValues());
      TEST_CHECK(address.GetBus() == busName, "Child address has correct bus name");
      TEST_CHECK(address.GetPath() == std::to_string(button->GetId()),
                 "Child address path is button ID (" + address.GetPath() + ")");
    }
  }

  // ===== Summary =====
  std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;

  // Cleanup
  bridge->Terminate();

  return gFailCount > 0 ? 1 : 0;
}
