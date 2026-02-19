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

/**
 * @brief Integration tests for the full bridge lifecycle over a real D-Bus.
 *
 * Uses GDBusWrapper against a private dbus-daemon (GTestDBus).
 * A FakeAtspiBroker registers the minimal AT-SPI services that the bridge
 * calls during Initialize + ForceUp, then test code exercises bridge
 * methods/properties via DBusClient, mirroring the mock-based test-app.cpp.
 */

// EXTERNAL INCLUDES
#include <gio/gio.h>
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
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <test/gdbus/gdbus-test-utils.h>
#include <test/test-accessible.h>

// =============================================================================
// Helpers (same as test-app.cpp)
// =============================================================================

static std::string MakeObjectPath(uint32_t id)
{
  return std::string{ATSPI_PREFIX_PATH} + std::to_string(id);
}

static DBus::DBusClient CreateAccessibleClient(const std::string& busName, uint32_t accessibleId, const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{
    busName,
    MakeObjectPath(accessibleId),
    Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE),
    conn};
}

static DBus::DBusClient CreateComponentClient(const std::string& busName, uint32_t accessibleId, const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{
    busName,
    MakeObjectPath(accessibleId),
    Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::COMPONENT),
    conn};
}

static DBus::DBusClient CreateSocketClient(const std::string& busName, const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{
    busName,
    std::string{ATSPI_PREFIX_PATH} + "root",
    Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::SOCKET),
    conn};
}

static DBus::DBusClient CreateRootAccessibleClient(const std::string& busName, const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{
    busName,
    std::string{ATSPI_PREFIX_PATH} + "root",
    Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE),
    conn};
}

// =============================================================================
// Main
// =============================================================================

int main(int, char**)
{
  std::cout << "=== GDBus Integration Tests ===" << std::endl;

  // ===== Step 1: Private dbus-daemon =====
  std::cout << "\n[1] Starting private dbus-daemon..." << std::endl;
  TestDBusFixture fixture;
  if(!fixture.Setup())
  {
    std::cout << "SKIP: dbus-daemon not available, skipping GDBus integration tests." << std::endl;
    return 0;
  }
  std::cout << "  Private bus at: " << fixture.busAddress << std::endl;

  // ===== Step 2: Register fake AT-SPI services =====
  std::cout << "\n[2] Registering FakeAtspiBroker..." << std::endl;

  // Connect directly via GLib for the broker (separate from the bridge's connection)
  GError* gerr = nullptr;
  GDBusConnection* rawBrokerConn = g_dbus_connection_new_for_address_sync(
    fixture.busAddress.c_str(),
    static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
    nullptr, nullptr, &gerr);
  if(gerr)
  {
    std::cerr << "FATAL: Cannot create broker connection: " << gerr->message << std::endl;
    g_error_free(gerr);
    return 1;
  }
  TEST_CHECK(rawBrokerConn != nullptr, "Broker connection established");

  FakeAtspiBroker broker(fixture.busAddress);
  bool brokerOk = broker.Register(rawBrokerConn);
  TEST_CHECK(brokerOk, "FakeAtspiBroker registered successfully");
  if(!brokerOk)
  {
    std::cerr << "FATAL: FakeAtspiBroker registration failed." << std::endl;
    g_object_unref(rawBrokerConn);
    return 1;
  }

  PumpMainLoop(50);
  std::cout << "  Fake AT-SPI services registered." << std::endl;

  // ===== Step 3: Set PlatformCallbacks =====
  std::cout << "\n[3] Setting PlatformCallbacks..." << std::endl;
  SetupTestPlatformCallbacks();
  std::cout << "  PlatformCallbacks set." << std::endl;

  // ===== Step 4: Create accessibility tree =====
  std::cout << "\n[4] Creating accessibility tree..." << std::endl;

  auto window = std::make_shared<TestAccessible>("TestWindow", Accessibility::Role::WINDOW);
  auto panel  = std::make_shared<TestAccessible>("Panel", Accessibility::Role::PANEL);
  auto button = std::make_shared<TestAccessible>("OK", Accessibility::Role::PUSH_BUTTON);
  auto label  = std::make_shared<TestAccessible>("Hello World", Accessibility::Role::LABEL);

  Accessibility::States buttonStates;
  buttonStates[Accessibility::State::ENABLED]   = true;
  buttonStates[Accessibility::State::SENSITIVE]  = true;
  buttonStates[Accessibility::State::VISIBLE]    = true;
  buttonStates[Accessibility::State::SHOWING]    = true;
  buttonStates[Accessibility::State::FOCUSABLE]  = true;
  button->SetStates(buttonStates);
  button->SetExtents({10.0f, 20.0f, 200.0f, 50.0f});

  Accessibility::States labelStates;
  labelStates[Accessibility::State::ENABLED] = true;
  labelStates[Accessibility::State::VISIBLE] = true;
  labelStates[Accessibility::State::SHOWING] = true;
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

  std::cout << "  Tree: window > panel > [button('OK'), label('Hello World')]" << std::endl;

  // ===== Step 5: Initialize bridge =====
  std::cout << "\n[5] Initializing bridge over real D-Bus..." << std::endl;

  auto bridge = Accessibility::Bridge::GetCurrentBridge();
  TEST_CHECK(bridge != nullptr, "Bridge::GetCurrentBridge() returns non-null");
  if(!bridge)
  {
    std::cerr << "FATAL: Bridge is null." << std::endl;
    g_object_unref(rawBrokerConn);
    return 1;
  }

  bridge->SetApplicationName("test-app");
  bridge->SetToolkitName("gdbus-test-toolkit");

  bridge->AddAccessible(window->GetId(), window);
  bridge->AddAccessible(panel->GetId(), panel);
  bridge->AddAccessible(button->GetId(), button);
  bridge->AddAccessible(label->GetId(), label);
  bridge->AddTopLevelWindow(window.get());

  // Initialize reads status properties from FakeAtspiBroker
  bridge->Initialize();

  // Pump to let async property reads complete
  PumpMainLoop(100);

  // ApplicationResumed triggers SwitchBridge → ForceUp
  bridge->ApplicationResumed();
  PumpMainLoop(100);

  // ===== Test: BridgeInitOverRealDBus =====
  bool isUp = bridge->IsUp();
  TEST_CHECK(isUp, "BridgeInitOverRealDBus — Bridge is up");
  if(!isUp)
  {
    std::cerr << "FATAL: Bridge is not up after Initialize + ApplicationResumed." << std::endl;
    std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;
    g_object_unref(rawBrokerConn);
    return gFailCount > 0 ? 1 : 0;
  }

  // ===== Test: BridgeGetsBusName =====
  std::string busName = bridge->GetBusName();
  std::cout << "  Bus name: " << busName << std::endl;
  TEST_CHECK(!busName.empty(), "BridgeGetsBusName");

  // Get a client connection on the AT-SPI bus (same private bus since GetAddress returns it)
  auto conn = DBusWrapper::Installed()->eldbus_address_connection_get_impl(fixture.busAddress);
  TEST_CHECK(conn != nullptr, "Client connection to AT-SPI bus established");

  // ===== Test: GetRoleOverDBus =====
  std::cout << "\n[6] Testing GetRole over real D-Bus..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, button->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
    TEST_CHECK(!!result, "GetRoleOverDBus call succeeds for button");
    if(result)
    {
      uint32_t roleVal = std::get<0>(result.getValues());
      TEST_CHECK(roleVal == static_cast<uint32_t>(Accessibility::Role::PUSH_BUTTON),
                 "GetRoleOverDBus — button is PUSH_BUTTON (" + std::to_string(roleVal) + ")");
    }
  }
  {
    auto client = CreateAccessibleClient(busName, label->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
    TEST_CHECK(!!result, "GetRoleOverDBus call succeeds for label");
    if(result)
    {
      uint32_t roleVal = std::get<0>(result.getValues());
      TEST_CHECK(roleVal == static_cast<uint32_t>(Accessibility::Role::LABEL),
                 "GetRoleOverDBus — label is LABEL (" + std::to_string(roleVal) + ")");
    }
  }

  // ===== Test: NamePropertyOverDBus =====
  std::cout << "\n[7] Testing Name property over real D-Bus..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, button->GetId(), conn);
    auto result = client.property<std::string>("Name").get();
    TEST_CHECK(!!result, "NamePropertyOverDBus get succeeds for button");
    if(result)
    {
      std::string name = std::get<0>(result.getValues());
      TEST_CHECK(name == "OK", "NamePropertyOverDBus — button name is 'OK' (got '" + name + "')");
    }
  }
  {
    auto client = CreateAccessibleClient(busName, label->GetId(), conn);
    auto result = client.property<std::string>("Name").get();
    TEST_CHECK(!!result, "NamePropertyOverDBus get succeeds for label");
    if(result)
    {
      std::string name = std::get<0>(result.getValues());
      TEST_CHECK(name == "Hello World", "NamePropertyOverDBus — label name is 'Hello World' (got '" + name + "')");
    }
  }

  // ===== Test: ChildCountOverDBus =====
  std::cout << "\n[8] Testing ChildCount over real D-Bus..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, panel->GetId(), conn);
    auto result = client.property<int>("ChildCount").get();
    TEST_CHECK(!!result, "ChildCountOverDBus get succeeds for panel");
    if(result)
    {
      int count = std::get<0>(result.getValues());
      TEST_CHECK(count == 2, "ChildCountOverDBus — panel has 2 children (got " + std::to_string(count) + ")");
    }
  }
  {
    auto client = CreateAccessibleClient(busName, window->GetId(), conn);
    auto result = client.property<int>("ChildCount").get();
    TEST_CHECK(!!result, "ChildCountOverDBus get succeeds for window");
    if(result)
    {
      int count = std::get<0>(result.getValues());
      TEST_CHECK(count == 1, "ChildCountOverDBus — window has 1 child (got " + std::to_string(count) + ")");
    }
  }

  // ===== Test: GetStateOverDBus =====
  std::cout << "\n[9] Testing GetState over real D-Bus..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, button->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<std::array<uint32_t, 2>>()>("GetState").call();
    TEST_CHECK(!!result, "GetStateOverDBus call succeeds for button");
    if(result)
    {
      auto stateData = std::get<0>(result.getValues());
      Accessibility::States states{stateData};
      TEST_CHECK(states[Accessibility::State::ENABLED], "GetStateOverDBus — ENABLED set");
      TEST_CHECK(states[Accessibility::State::VISIBLE], "GetStateOverDBus — VISIBLE set");
      TEST_CHECK(states[Accessibility::State::FOCUSABLE], "GetStateOverDBus — FOCUSABLE set");
    }
  }

  // ===== Test: GetExtentsOverDBus =====
  std::cout << "\n[10] Testing GetExtents over real D-Bus..." << std::endl;
  {
    auto client = CreateComponentClient(busName, button->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<std::tuple<int32_t, int32_t, int32_t, int32_t>>(uint32_t)>("GetExtents").call(static_cast<uint32_t>(Accessibility::CoordinateType::SCREEN));
    TEST_CHECK(!!result, "GetExtentsOverDBus call succeeds");
    if(result)
    {
      auto extents = std::get<0>(result.getValues());
      auto x = std::get<0>(extents);
      auto y = std::get<1>(extents);
      auto w = std::get<2>(extents);
      auto h = std::get<3>(extents);
      TEST_CHECK(x == 10, "GetExtentsOverDBus — x=10 (got " + std::to_string(x) + ")");
      TEST_CHECK(y == 20, "GetExtentsOverDBus — y=20 (got " + std::to_string(y) + ")");
      TEST_CHECK(w == 200, "GetExtentsOverDBus — w=200 (got " + std::to_string(w) + ")");
      TEST_CHECK(h == 50, "GetExtentsOverDBus — h=50 (got " + std::to_string(h) + ")");
    }
  }

  // ===== Test: GetChildAtIndexOverDBus =====
  std::cout << "\n[11] Testing GetChildAtIndex over real D-Bus..." << std::endl;
  {
    auto client = CreateAccessibleClient(busName, panel->GetId(), conn);
    auto result = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(0);
    TEST_CHECK(!!result, "GetChildAtIndexOverDBus call succeeds");
    if(result)
    {
      auto address = std::get<0>(result.getValues());
      TEST_CHECK(address.GetBus() == busName, "GetChildAtIndexOverDBus — correct bus name");
      TEST_CHECK(address.GetPath() == std::to_string(button->GetId()),
                 "GetChildAtIndexOverDBus — path is button ID (" + address.GetPath() + ")");
    }
  }

  // ===== Test: SocketEmbedOverDBus =====
  std::cout << "\n[12] Testing Socket Embed/Unembed over real D-Bus..." << std::endl;
  {
    auto socketClient = CreateSocketClient(busName, conn);
    Accessibility::Address plugAddr{"plug.bus.test", "plug_test"};
    auto embedResult = socketClient.method<DBus::ValueOrError<Accessibility::Address>(Accessibility::Address)>("Embed").call(plugAddr);
    TEST_CHECK(!!embedResult, "SocketEmbedOverDBus — Embed call succeeds");
    if(embedResult)
    {
      auto addr = std::get<0>(embedResult.getValues());
      TEST_CHECK(addr.GetPath() == "root", "SocketEmbedOverDBus — Embed returns root path (got '" + addr.GetPath() + "')");
    }

    auto unembedResult = socketClient.method<DBus::ValueOrError<void>(Accessibility::Address)>("Unembed").call(plugAddr);
    TEST_CHECK(!!unembedResult, "SocketEmbedOverDBus — Unembed call succeeds");
  }

  // ===== Test: SetOffsetOverDBus =====
  std::cout << "\n[13] Testing SetOffset over real D-Bus..." << std::endl;
  {
    auto socketClient = CreateSocketClient(busName, conn);
    Accessibility::Address plugAddr{"plug.bus.offset", "plug_offset"};

    socketClient.method<DBus::ValueOrError<Accessibility::Address>(Accessibility::Address)>("Embed").call(plugAddr);
    socketClient.method<DBus::ValueOrError<void>(std::int32_t, std::int32_t)>("SetOffset").call(100, 200);

    auto compClient = CreateComponentClient(busName, button->GetId(), conn);
    auto extResult = compClient.method<DBus::ValueOrError<std::tuple<int32_t, int32_t, int32_t, int32_t>>(uint32_t)>("GetExtents").call(static_cast<uint32_t>(Accessibility::CoordinateType::SCREEN));
    TEST_CHECK(!!extResult, "SetOffsetOverDBus — GetExtents succeeds after SetOffset");
    if(extResult)
    {
      auto extents = std::get<0>(extResult.getValues());
      auto x = std::get<0>(extents);
      auto y = std::get<1>(extents);
      TEST_CHECK(x == 110, "SetOffsetOverDBus — x shifted by 100 (got " + std::to_string(x) + ")");
      TEST_CHECK(y == 220, "SetOffsetOverDBus — y shifted by 200 (got " + std::to_string(y) + ")");
    }

    // Cleanup
    socketClient.method<DBus::ValueOrError<void>(Accessibility::Address)>("Unembed").call(plugAddr);
  }

  // ===== Test: MultipleClients =====
  std::cout << "\n[14] Testing multiple concurrent clients..." << std::endl;
  {
    auto conn2 = DBusWrapper::Installed()->eldbus_address_connection_get_impl(fixture.busAddress);
    TEST_CHECK(conn2 != nullptr, "MultipleClients — second connection established");

    auto client1 = CreateAccessibleClient(busName, button->GetId(), conn);
    auto client2 = CreateAccessibleClient(busName, label->GetId(), conn2);

    auto result1 = client1.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
    auto result2 = client2.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();

    TEST_CHECK(!!result1, "MultipleClients — client1 GetRole succeeds");
    TEST_CHECK(!!result2, "MultipleClients — client2 GetRole succeeds");
    if(result1 && result2)
    {
      TEST_CHECK(std::get<0>(result1.getValues()) == static_cast<uint32_t>(Accessibility::Role::PUSH_BUTTON),
                 "MultipleClients — client1 role correct");
      TEST_CHECK(std::get<0>(result2.getValues()) == static_cast<uint32_t>(Accessibility::Role::LABEL),
                 "MultipleClients — client2 role correct");
    }
  }

  // ===== Test: BridgeTerminateClean =====
  std::cout << "\n[15] Testing bridge termination..." << std::endl;
  {
    bridge->Terminate();
    TEST_CHECK(!bridge->IsUp(), "BridgeTerminateClean — Bridge is down after Terminate");
  }

  // ===== Summary =====
  std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;

  // Cleanup
  broker.Unregister();
  g_object_unref(rawBrokerConn);

  return gFailCount > 0 ? 1 : 0;
}
