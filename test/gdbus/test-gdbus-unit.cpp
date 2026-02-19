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
 * @brief Unit tests for GDBusWrapper.
 *
 * Exercises GDBusWrapper methods directly (serialization, connection,
 * interface registration) over a private dbus-daemon via GTestDBus.
 * No bridge, no accessibility tree — just raw DBusServer/DBusClient.
 */

// EXTERNAL INCLUDES
#include <gio/gio.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <test/gdbus/gdbus-test-utils.h>

// =============================================================================
// Echo service helpers
// =============================================================================

static const char* ECHO_BUS_NAME  = "com.test.EchoService";
static const char* ECHO_PATH      = "/com/test/Echo";
static const char* ECHO_INTERFACE = "com.test.Echo";

/**
 * @brief Sets up a DBusServer that registers echo methods for various types
 *        and returns a connection that clients can use.
 */
static DBus::DBusServer SetupEchoServer(DBusWrapper::ConnectionPtr& conn)
{
  conn = DBusWrapper::Installed()->eldbus_connection_get_impl(DBusWrapper::ConnectionType::SESSION);
  DBus::DBusServer server(conn);

  // Request the well-known bus name
  DBus::requestBusName(conn, ECHO_BUS_NAME);

  // --- Basic type echo methods ---
  DBus::DBusInterfaceDescription desc(ECHO_INTERFACE);

  desc.addMethod<DBus::ValueOrError<uint8_t>(uint8_t)>("EchoUint8",
    [](uint8_t v) -> DBus::ValueOrError<uint8_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<uint16_t>(uint16_t)>("EchoUint16",
    [](uint16_t v) -> DBus::ValueOrError<uint16_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<uint32_t>(uint32_t)>("EchoUint32",
    [](uint32_t v) -> DBus::ValueOrError<uint32_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<uint64_t>(uint64_t)>("EchoUint64",
    [](uint64_t v) -> DBus::ValueOrError<uint64_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<int16_t>(int16_t)>("EchoInt16",
    [](int16_t v) -> DBus::ValueOrError<int16_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<int32_t>(int32_t)>("EchoInt32",
    [](int32_t v) -> DBus::ValueOrError<int32_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<int64_t>(int64_t)>("EchoInt64",
    [](int64_t v) -> DBus::ValueOrError<int64_t> { return {v}; });

  desc.addMethod<DBus::ValueOrError<double>(double)>("EchoDouble",
    [](double v) -> DBus::ValueOrError<double> { return {v}; });

  desc.addMethod<DBus::ValueOrError<bool>(bool)>("EchoBool",
    [](bool v) -> DBus::ValueOrError<bool> { return {v}; });

  desc.addMethod<DBus::ValueOrError<std::string>(std::string)>("EchoString",
    [](std::string v) -> DBus::ValueOrError<std::string> { return {std::move(v)}; });

  desc.addMethod<DBus::ValueOrError<std::string>(std::string)>("EchoObjectPath",
    [](std::string v) -> DBus::ValueOrError<std::string> { return {std::move(v)}; });

  // --- Container type echo methods ---
  desc.addMethod<DBus::ValueOrError<std::tuple<int32_t, std::string, bool>>(std::tuple<int32_t, std::string, bool>)>("EchoStruct",
    [](std::tuple<int32_t, std::string, bool> v) -> DBus::ValueOrError<std::tuple<int32_t, std::string, bool>> { return {std::move(v)}; });

  desc.addMethod<DBus::ValueOrError<std::vector<int32_t>>(std::vector<int32_t>)>("EchoArray",
    [](std::vector<int32_t> v) -> DBus::ValueOrError<std::vector<int32_t>> { return {std::move(v)}; });

  desc.addMethod<DBus::ValueOrError<std::map<std::string, int32_t>>(std::map<std::string, int32_t>)>("EchoDict",
    [](std::map<std::string, int32_t> v) -> DBus::ValueOrError<std::map<std::string, int32_t>> { return {std::move(v)}; });

  desc.addMethod<DBus::ValueOrError<std::vector<std::tuple<std::string, int32_t>>>(std::vector<std::tuple<std::string, int32_t>>)>("EchoNested",
    [](std::vector<std::tuple<std::string, int32_t>> v) -> DBus::ValueOrError<std::vector<std::tuple<std::string, int32_t>>> { return {std::move(v)}; });

  // --- Error method (always returns error) ---
  desc.addMethod<DBus::ValueOrError<void>()>("AlwaysFail",
    []() -> DBus::ValueOrError<void> {
      return Ipc::Error{"Test error message"};
    });

  // --- Property ---
  static std::string storedProperty = "initial";
  desc.addProperty<std::string>("TestProp",
    []() -> DBus::ValueOrError<std::string> { return {storedProperty}; },
    [](std::string v) -> DBus::ValueOrError<void> { storedProperty = std::move(v); return {}; });

  server.addInterface(ECHO_PATH, desc, false);

  return server;
}

// =============================================================================
// Test Groups
// =============================================================================

// ---- A. Connection Tests ----

static void TestSessionConnection()
{
  std::cout << "\n[A] Connection Tests" << std::endl;

  auto conn = DBusWrapper::Installed()->eldbus_connection_get_impl(DBusWrapper::ConnectionType::SESSION);
  TEST_CHECK(conn != nullptr, "SessionConnectionSucceeds");
}

static void TestConnectionUniqueName()
{
  auto conn = DBusWrapper::Installed()->eldbus_connection_get_impl(DBusWrapper::ConnectionType::SESSION);
  std::string name = DBusWrapper::Installed()->eldbus_connection_unique_name_get_impl(conn);
  TEST_CHECK(!name.empty() && name[0] == ':', "ConnectionUniqueName starts with ':' (got '" + name + "')");
}

static void TestAddressConnection(const std::string& busAddress)
{
  auto conn = DBusWrapper::Installed()->eldbus_address_connection_get_impl(busAddress);
  TEST_CHECK(conn != nullptr, "AddressConnectionSucceeds");
}

// ---- B. Basic Type Serialization Roundtrip ----

static void TestBasicTypeSerialization(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[B] Basic Type Serialization Roundtrip" << std::endl;

  DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);

  // uint8_t
  {
    auto result = client.method<DBus::ValueOrError<uint8_t>(uint8_t)>("EchoUint8").call(uint8_t(42));
    TEST_CHECK(result && std::get<0>(result.getValues()) == 42, "Uint8Roundtrip");
  }
  // uint16_t
  {
    auto result = client.method<DBus::ValueOrError<uint16_t>(uint16_t)>("EchoUint16").call(uint16_t(1234));
    TEST_CHECK(result && std::get<0>(result.getValues()) == 1234, "Uint16Roundtrip");
  }
  // uint32_t
  {
    auto result = client.method<DBus::ValueOrError<uint32_t>(uint32_t)>("EchoUint32").call(uint32_t(123456));
    TEST_CHECK(result && std::get<0>(result.getValues()) == 123456, "Uint32Roundtrip");
  }
  // uint64_t
  {
    auto result = client.method<DBus::ValueOrError<uint64_t>(uint64_t)>("EchoUint64").call(uint64_t(9876543210ULL));
    TEST_CHECK(result && std::get<0>(result.getValues()) == 9876543210ULL, "Uint64Roundtrip");
  }
  // int16_t
  {
    auto result = client.method<DBus::ValueOrError<int16_t>(int16_t)>("EchoInt16").call(int16_t(-123));
    TEST_CHECK(result && std::get<0>(result.getValues()) == -123, "Int16Roundtrip");
  }
  // int32_t
  {
    auto result = client.method<DBus::ValueOrError<int32_t>(int32_t)>("EchoInt32").call(int32_t(-99999));
    TEST_CHECK(result && std::get<0>(result.getValues()) == -99999, "Int32Roundtrip");
  }
  // int64_t
  {
    auto result = client.method<DBus::ValueOrError<int64_t>(int64_t)>("EchoInt64").call(int64_t(-9876543210LL));
    TEST_CHECK(result && std::get<0>(result.getValues()) == -9876543210LL, "Int64Roundtrip");
  }
  // double
  {
    auto result = client.method<DBus::ValueOrError<double>(double)>("EchoDouble").call(3.14159);
    TEST_CHECK(result && std::abs(std::get<0>(result.getValues()) - 3.14159) < 1e-10, "DoubleRoundtrip");
  }
  // bool true
  {
    auto result = client.method<DBus::ValueOrError<bool>(bool)>("EchoBool").call(true);
    TEST_CHECK(result && std::get<0>(result.getValues()) == true, "BoolTrueRoundtrip");
  }
  // bool false
  {
    auto result = client.method<DBus::ValueOrError<bool>(bool)>("EchoBool").call(false);
    TEST_CHECK(result && std::get<0>(result.getValues()) == false, "BoolFalseRoundtrip");
  }
  // string
  {
    auto result = client.method<DBus::ValueOrError<std::string>(std::string)>("EchoString").call(std::string("hello world"));
    TEST_CHECK(result && std::get<0>(result.getValues()) == "hello world", "StringRoundtrip");
  }
}

// ---- C. Container Serialization ----

static void TestContainerSerialization(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[C] Container Serialization" << std::endl;

  DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);

  // Struct roundtrip
  {
    auto input  = std::make_tuple(int32_t(42), std::string("test"), true);
    auto result = client.method<DBus::ValueOrError<std::tuple<int32_t, std::string, bool>>(std::tuple<int32_t, std::string, bool>)>("EchoStruct").call(input);
    TEST_CHECK(result, "StructRoundtrip call succeeds");
    if(result)
    {
      auto val = std::get<0>(result.getValues());
      TEST_CHECK(std::get<0>(val) == 42 && std::get<1>(val) == "test" && std::get<2>(val) == true,
                 "StructRoundtrip values match");
    }
  }

  // Array roundtrip (non-empty)
  {
    std::vector<int32_t> input = {1, 2, 3, 4, 5};
    auto result = client.method<DBus::ValueOrError<std::vector<int32_t>>(std::vector<int32_t>)>("EchoArray").call(input);
    TEST_CHECK(result, "ArrayRoundtrip call succeeds");
    if(result)
    {
      auto val = std::get<0>(result.getValues());
      TEST_CHECK(val == input, "ArrayRoundtrip values match");
    }
  }

  // Array roundtrip (empty)
  {
    std::vector<int32_t> input;
    auto result = client.method<DBus::ValueOrError<std::vector<int32_t>>(std::vector<int32_t>)>("EchoArray").call(input);
    TEST_CHECK(result, "EmptyArrayRoundtrip call succeeds");
    if(result)
    {
      auto val = std::get<0>(result.getValues());
      TEST_CHECK(val.empty(), "EmptyArrayRoundtrip is empty");
    }
  }

  // Dict roundtrip
  {
    std::map<std::string, int32_t> input = {{"a", 1}, {"b", 2}, {"c", 3}};
    auto result = client.method<DBus::ValueOrError<std::map<std::string, int32_t>>(std::map<std::string, int32_t>)>("EchoDict").call(input);
    TEST_CHECK(result, "DictRoundtrip call succeeds");
    if(result)
    {
      auto val = std::get<0>(result.getValues());
      TEST_CHECK(val == input, "DictRoundtrip values match");
    }
  }

  // Nested roundtrip: vector<tuple<string, int32_t>>
  {
    std::vector<std::tuple<std::string, int32_t>> input = {{"alpha", 1}, {"beta", 2}};
    auto result = client.method<DBus::ValueOrError<std::vector<std::tuple<std::string, int32_t>>>(std::vector<std::tuple<std::string, int32_t>>)>("EchoNested").call(input);
    TEST_CHECK(result, "NestedRoundtrip call succeeds");
    if(result)
    {
      auto val = std::get<0>(result.getValues());
      TEST_CHECK(val == input, "NestedRoundtrip values match");
    }
  }
}

// ---- D. Object/Proxy ----

static void TestObjectProxy(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[D] Object/Proxy Tests" << std::endl;

  auto obj = DBusWrapper::Installed()->eldbus_object_get_impl(conn, ECHO_BUS_NAME, ECHO_PATH);
  TEST_CHECK(obj != nullptr, "ObjectGet returns non-null");

  auto proxy = DBusWrapper::Installed()->eldbus_proxy_get_impl(obj, ECHO_INTERFACE);
  TEST_CHECK(proxy != nullptr, "ProxyGet returns non-null");

  std::string ifaceName = DBusWrapper::Installed()->eldbus_proxy_interface_get_impl(proxy);
  TEST_CHECK(ifaceName == ECHO_INTERFACE, "ProxyGet has correct interface (got '" + ifaceName + "')");

  auto proxyCopy = DBusWrapper::Installed()->eldbus_proxy_copy_impl(proxy);
  TEST_CHECK(proxyCopy != nullptr, "ProxyCopy returns non-null");
  std::string copyIfaceName = DBusWrapper::Installed()->eldbus_proxy_interface_get_impl(proxyCopy);
  TEST_CHECK(copyIfaceName == ECHO_INTERFACE, "ProxyCopy has same interface name");
}

// ---- E. Bus Name ----

static void TestBusName(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[E] Bus Name Tests" << std::endl;

  // Request should not crash
  DBus::requestBusName(conn, "com.test.BusNameTest");
  TEST_CHECK(true, "RequestName does not crash");

  // Release should not crash
  DBus::releaseBusName(conn, "com.test.BusNameTest");
  TEST_CHECK(true, "ReleaseName does not crash");
}

// ---- F. Interface Registration ----

static void TestInterfaceRegistration(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[F] Interface Registration Tests" << std::endl;

  // Non-fallback method registration — we already registered the echo service;
  // test it by calling via a fresh client
  {
    DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);
    auto result = client.method<DBus::ValueOrError<int32_t>(int32_t)>("EchoInt32").call(int32_t(777));
    TEST_CHECK(result && std::get<0>(result.getValues()) == 777, "NonFallbackMethodCallSucceeds");
  }

  // Fallback method registration — register on a parent path, call from child path
  {
    auto serverConn = DBusWrapper::Installed()->eldbus_connection_get_impl(DBusWrapper::ConnectionType::SESSION);
    DBus::DBusServer fallbackServer(serverConn);
    DBus::requestBusName(serverConn, "com.test.FallbackService");

    DBus::DBusInterfaceDescription fallbackDesc("com.test.Fallback");
    fallbackDesc.addMethod<DBus::ValueOrError<std::string>()>("WhoAmI",
      []() -> DBus::ValueOrError<std::string> {
        return {DBus::DBusServer::getCurrentObjectPath()};
      });
    fallbackServer.addInterface("/com/test/fallback", fallbackDesc, true);

    PumpMainLoop(20);

    // Call from a child path — the fallback handler should respond
    DBus::DBusClient childClient("com.test.FallbackService", "/com/test/fallback/child", "com.test.Fallback", conn);
    auto result = childClient.method<DBus::ValueOrError<std::string>()>("WhoAmI").call();
    TEST_CHECK(result, "FallbackMethodCallSucceeds");
    if(result)
    {
      auto path = std::get<0>(result.getValues());
      TEST_CHECK(path == "/com/test/fallback/child",
                 "FallbackMethodReturnsChildPath (got '" + path + "')");
    }

    DBus::releaseBusName(serverConn, "com.test.FallbackService");
  }

  // Property get/set via registered callbacks
  {
    DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);

    auto getResult = client.property<std::string>("TestProp").get();
    TEST_CHECK(getResult, "PropertyGetSucceeds");
    if(getResult)
    {
      auto val = std::get<0>(getResult.getValues());
      TEST_CHECK(val == "initial", "PropertyGetReturnsInitialValue (got '" + val + "')");
    }

    auto setResult = client.property<std::string>("TestProp").set("updated");
    TEST_CHECK(!!setResult, "PropertySetSucceeds");

    auto getResult2 = client.property<std::string>("TestProp").get();
    TEST_CHECK(getResult2, "PropertyGetAfterSetSucceeds");
    if(getResult2)
    {
      auto val = std::get<0>(getResult2.getValues());
      TEST_CHECK(val == "updated", "PropertyGetReturnsUpdatedValue (got '" + val + "')");
    }
  }
}

// ---- G. Signal ----

static void TestSignal(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[G] Signal Tests" << std::endl;

  auto serverConn = DBusWrapper::Installed()->eldbus_connection_get_impl(DBusWrapper::ConnectionType::SESSION);
  DBus::DBusServer signalServer(serverConn);
  DBus::requestBusName(serverConn, "com.test.SignalService");

  // Register an interface with signals declared
  DBus::DBusInterfaceDescription desc("com.test.Signal");
  desc.addMethod<DBus::ValueOrError<void>()>("Noop",
    []() -> DBus::ValueOrError<void> { return {}; });
  signalServer.addInterface("/com/test/signal", desc, false);

  PumpMainLoop(20);

  // Subscribe to signal
  bool signalReceived = false;
  std::string receivedStr;
  int32_t     receivedInt = 0;

  DBus::DBusClient signalClient("com.test.SignalService", "/com/test/signal", "com.test.Signal", conn);
  signalClient.addSignal<void(std::string, int32_t)>("TestSignal",
    [&](std::string s, int32_t i)
    {
      signalReceived = true;
      receivedStr    = s;
      receivedInt    = i;
    });

  PumpMainLoop(20);

  // Emit signal from server
  signalServer.emit2("/com/test/signal", "com.test.Signal", "TestSignal", std::string("hello"), int32_t(99));

  // Pump main loop to deliver the signal
  for(int i = 0; i < 200 && !signalReceived; ++i)
  {
    g_main_context_iteration(nullptr, FALSE);
    g_usleep(1000); // 1ms
  }

  TEST_CHECK(signalReceived, "SignalCallbackFires");
  TEST_CHECK(receivedStr == "hello", "SignalStringArgCorrect (got '" + receivedStr + "')");
  TEST_CHECK(receivedInt == 99, "SignalIntArgCorrect (got " + std::to_string(receivedInt) + ")");

  DBus::releaseBusName(serverConn, "com.test.SignalService");
}

// ---- H. Error Handling ----

static void TestErrorHandling(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[H] Error Handling Tests" << std::endl;

  // Call non-existent method
  {
    DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);
    auto result = client.method<DBus::ValueOrError<void>()>("NonExistentMethod").call();
    TEST_CHECK(!result, "NonExistentMethodReturnsError");
  }

  // Call method that returns error
  {
    DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);
    auto result = client.method<DBus::ValueOrError<void>()>("AlwaysFail").call();
    TEST_CHECK(!result, "AlwaysFailReturnsError");
    if(!result)
    {
      auto errMsg = result.getError().message;
      TEST_CHECK(!errMsg.empty(), "ErrorMessagePopulated (got '" + errMsg + "')");
    }
  }

  // Null connection → proxy creation returns null
  {
    DBusWrapper::ConnectionPtr nullConn;
    auto obj = DBusWrapper::Installed()->eldbus_object_get_impl(nullConn, "x", "/x");
    // Object may or may not be null depending on implementation;
    // the key check is that no crash occurs
    TEST_CHECK(true, "NullConnectionNoCrash");
  }
}

// ---- I. Async Method Call ----

static void TestAsyncMethodCall(const DBusWrapper::ConnectionPtr& conn)
{
  std::cout << "\n[I] Async Method Call Tests" << std::endl;

  DBus::DBusClient client(ECHO_BUS_NAME, ECHO_PATH, ECHO_INTERFACE, conn);

  bool callbackFired = false;
  int32_t asyncResult = 0;

  client.method<DBus::ValueOrError<int32_t>(int32_t)>("EchoInt32").asyncCall(
    [&](DBus::ValueOrError<int32_t> result)
    {
      callbackFired = true;
      if(result)
      {
        asyncResult = std::get<0>(result.getValues());
      }
    },
    int32_t(42));

  // Pump main loop until callback fires
  for(int i = 0; i < 200 && !callbackFired; ++i)
  {
    g_main_context_iteration(nullptr, FALSE);
    g_usleep(1000);
  }

  TEST_CHECK(callbackFired, "AsyncCallbackFires");
  TEST_CHECK(asyncResult == 42, "AsyncResultCorrect (got " + std::to_string(asyncResult) + ")");
}

// =============================================================================
// Main
// =============================================================================

int main(int, char**)
{
  std::cout << "=== GDBus Unit Tests ===" << std::endl;

  // Set up private dbus-daemon
  TestDBusFixture fixture;
  if(!fixture.Setup())
  {
    std::cout << "SKIP: dbus-daemon not available, skipping GDBus unit tests." << std::endl;
    return 0;
  }
  std::cout << "Private bus at: " << fixture.busAddress << std::endl;

  // GDBusWrapper is auto-installed by DBusWrapper::Installed() since we linked dbus-gdbus.cpp.
  // The SESSION bus will use our private daemon (DBUS_SESSION_BUS_ADDRESS was set by fixture).

  // Set up the echo server on the SESSION bus.
  // The server uses g_bus_get_sync(SESSION) which returns a shared singleton.
  DBusWrapper::ConnectionPtr serverConn;
  auto server = SetupEchoServer(serverConn);

  // Give the server a moment to register
  PumpMainLoop(20);

  // Get a SEPARATE client connection via address (not the singleton).
  // This avoids deadlocks when making synchronous calls within the same
  // process: the server's method handler is dispatched on the main context,
  // and g_dbus_connection_call_sync blocks. With separate connections, the
  // server's handler runs on its own connection's dispatch.
  auto clientConn = DBusWrapper::Installed()->eldbus_address_connection_get_impl(fixture.busAddress);

  // Run test groups
  TestSessionConnection();
  TestConnectionUniqueName();
  TestAddressConnection(fixture.busAddress);
  TestBasicTypeSerialization(clientConn);
  TestContainerSerialization(clientConn);
  TestObjectProxy(clientConn);
  TestBusName(clientConn);
  TestInterfaceRegistration(clientConn);
  TestSignal(clientConn);
  TestErrorHandling(clientConn);
  TestAsyncMethodCall(clientConn);

  // Summary
  std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;

  // Cleanup
  DBus::releaseBusName(serverConn, ECHO_BUS_NAME);

  return gFailCount > 0 ? 1 : 0;
}
