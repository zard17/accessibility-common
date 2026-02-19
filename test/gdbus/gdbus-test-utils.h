#ifndef ACCESSIBILITY_TEST_GDBUS_TEST_UTILS_H
#define ACCESSIBILITY_TEST_GDBUS_TEST_UTILS_H

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
#include <gio/gio.h>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <accessibility/internal/bridge/dbus/dbus.h>

// =============================================================================
// Test framework
// =============================================================================

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

// =============================================================================
// TestDBusFixture — private dbus-daemon via GTestDBus
// =============================================================================

/**
 * @brief RAII fixture that spins up an isolated dbus-daemon using GTestDBus.
 *
 * If dbus-daemon is not installed, Setup() returns false.
 * The private bus address is exported to DBUS_SESSION_BUS_ADDRESS so that
 * GDBusWrapper's g_bus_get_sync(SESSION) picks it up automatically.
 */
struct TestDBusFixture
{
  GTestDBus*  testBus    = nullptr;
  std::string busAddress;

  /**
   * @brief Checks if dbus-daemon binary is available in PATH.
   *
   * GTestDBus aborts the process if dbus-daemon is not found,
   * so we pre-check to allow graceful skipping.
   */
  static bool IsDbusDaemonAvailable()
  {
    gchar* path = g_find_program_in_path("dbus-daemon");
    if(path)
    {
      g_free(path);
      return true;
    }
    return false;
  }

  /**
   * @brief Starts a private dbus-daemon instance.
   * @return true on success, false if dbus-daemon is not available.
   */
  bool Setup()
  {
    // Check if dbus-daemon is available first, since g_test_dbus_up()
    // calls g_error() (abort) if it can't spawn the daemon.
    if(!IsDbusDaemonAvailable())
    {
      return false;
    }

    testBus = g_test_dbus_new(G_TEST_DBUS_NONE);
    g_test_dbus_up(testBus);
    const char* addr = g_test_dbus_get_bus_address(testBus);
    if(!addr || addr[0] == '\0')
    {
      g_object_unref(testBus);
      testBus = nullptr;
      return false;
    }
    busAddress = addr;
    g_setenv("DBUS_SESSION_BUS_ADDRESS", busAddress.c_str(), TRUE);
    return true;
  }

  /**
   * @brief Tears down the private dbus-daemon.
   */
  void Teardown()
  {
    if(testBus)
    {
      g_test_dbus_down(testBus);
      g_object_unref(testBus);
      testBus = nullptr;
    }
  }

  ~TestDBusFixture()
  {
    Teardown();
  }
};

// =============================================================================
// PlatformCallbacks setup
// =============================================================================

/**
 * @brief Installs minimal PlatformCallbacks suitable for testing.
 */
static void SetupTestPlatformCallbacks()
{
  Accessibility::PlatformCallbacks callbacks;
  callbacks.addIdle = [](std::function<bool()> cb) -> uint32_t {
    if(cb) cb();
    return 1;
  };
  callbacks.removeIdle = [](uint32_t) {};
  callbacks.getToolkitVersion = []() -> std::string { return "gdbus-test-1.0.0"; };
  callbacks.getAppName = []() -> std::string { return "gdbus-test-app"; };
  callbacks.isAdaptorAvailable = []() -> bool { return true; };
  callbacks.onEnableAutoInit = []() {};
  callbacks.createTimer = [](uint32_t, std::function<bool()> cb) -> uint32_t {
    if(cb) cb();
    return 1;
  };
  callbacks.cancelTimer = [](uint32_t) {};
  callbacks.isTimerRunning = [](uint32_t) -> bool { return false; };
  Accessibility::SetPlatformCallbacks(callbacks);
}

// =============================================================================
// Main loop pumping
// =============================================================================

/**
 * @brief Pumps the GLib main context to process pending async events.
 * @param maxIterations Maximum number of iterations to pump.
 */
/**
 * @brief Pumps the GLib main context to process pending async events.
 *
 * Uses non-blocking iterations without early exit, because D-Bus round-trips
 * involve socket I/O between iterations — the next event may not be immediately
 * pending when the previous one finishes. Running all iterations ensures multi-hop
 * async exchanges (broker dispatch → reply delivery) complete fully.
 *
 * @param maxIterations Maximum number of iterations to pump.
 */
static void PumpMainLoop(int maxIterations = 200)
{
  for(int i = 0; i < maxIterations; ++i)
  {
    g_main_context_iteration(nullptr, FALSE);
  }
}

// =============================================================================
// FakeAtspiBroker — minimal AT-SPI services for integration tests
// =============================================================================

/**
 * @brief Registers the minimal set of AT-SPI services that the bridge
 *        calls during Initialize() + ForceUp().
 *
 * Services provided:
 *  - org.a11y.Bus at /org/a11y/bus
 *    - Method: GetAddress → returns the private bus address
 *  - org.a11y.Status properties on /org/a11y/bus
 *    - IsEnabled → true
 *    - ScreenReaderEnabled → true
 *  - org.a11y.atspi.Registry at /org/a11y/atspi/registry
 *    - Method: GetRegisteredEvents → empty array a(ss)
 *  - org.a11y.atspi.Socket stub at /org/a11y/atspi/accessible/root
 *    - Method: Embed → returns dummy parent Address
 *    - Method: Unembed → no-op
 */
class FakeAtspiBroker
{
public:
  FakeAtspiBroker(const std::string& busAddress)
  : mBusAddress(busAddress)
  {
  }

  ~FakeAtspiBroker()
  {
    Unregister();
  }

  /**
   * @brief Registers all fake services on the given connection.
   * @return true on success.
   */
  bool Register(GDBusConnection* conn)
  {
    mConnection = conn;

    if(!RegisterA11yBus())
      return false;
    if(!RegisterA11yStatus())
      return false;
    if(!RegisterRegistry())
      return false;
    if(!RegisterSocket())
      return false;

    // Request well-known names
    RequestName("org.a11y.Bus");
    RequestName("org.a11y.atspi.Registry");

    return true;
  }

  void Unregister()
  {
    for(auto id : mRegistrationIds)
    {
      if(id > 0 && mConnection)
      {
        g_dbus_connection_unregister_object(mConnection, id);
      }
    }
    mRegistrationIds.clear();

    for(auto* info : mNodeInfos)
    {
      if(info)
      {
        g_dbus_node_info_unref(info);
      }
    }
    mNodeInfos.clear();

    for(auto* vt : mVtables)
    {
      delete vt;
    }
    mVtables.clear();
  }

private:
  std::string            mBusAddress;
  GDBusConnection*       mConnection = nullptr;
  std::vector<guint>     mRegistrationIds;
  std::vector<GDBusNodeInfo*> mNodeInfos;
  std::vector<GDBusInterfaceVTable*> mVtables;

  void RequestName(const char* name)
  {
    GError*  err    = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
      mConnection,
      "org.freedesktop.DBus",
      "/org/freedesktop/DBus",
      "org.freedesktop.DBus",
      "RequestName",
      g_variant_new("(su)", name, 0x4u),
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      1000,
      nullptr,
      &err);
    if(err)
      g_error_free(err);
    if(result)
      g_variant_unref(result);
  }

  // --- org.a11y.Bus interface (GetAddress method) ---
  bool RegisterA11yBus()
  {
    static const char* xml =
      "<node>"
      "  <interface name='org.a11y.Bus'>"
      "    <method name='GetAddress'>"
      "      <arg name='address' type='s' direction='out'/>"
      "    </method>"
      "  </interface>"
      "</node>";

    return RegisterInterface(xml, "/org/a11y/bus", &FakeAtspiBroker::HandleA11yBusMethod, nullptr, nullptr);
  }

  static void HandleA11yBusMethod(GDBusConnection*, const gchar*, const gchar*, const gchar*,
                                  const gchar* methodName, GVariant*, GDBusMethodInvocation* invocation,
                                  gpointer userData)
  {
    auto* self = static_cast<FakeAtspiBroker*>(userData);
    if(g_strcmp0(methodName, "GetAddress") == 0)
    {
      g_dbus_method_invocation_return_value(invocation,
        g_variant_new("(s)", self->mBusAddress.c_str()));
    }
    else
    {
      g_dbus_method_invocation_return_dbus_error(invocation,
        "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
    }
  }

  // --- org.a11y.Status interface (IsEnabled, ScreenReaderEnabled properties) ---
  bool RegisterA11yStatus()
  {
    static const char* xml =
      "<node>"
      "  <interface name='org.a11y.Status'>"
      "    <property name='IsEnabled' type='b' access='read'/>"
      "    <property name='ScreenReaderEnabled' type='b' access='read'/>"
      "  </interface>"
      "</node>";

    return RegisterInterface(xml, "/org/a11y/bus", nullptr, &FakeAtspiBroker::HandleStatusGetProperty, nullptr);
  }

  static GVariant* HandleStatusGetProperty(GDBusConnection*, const gchar*, const gchar*, const gchar*,
                                           const gchar* propertyName, GError** error, gpointer)
  {
    if(g_strcmp0(propertyName, "IsEnabled") == 0)
    {
      return g_variant_new_boolean(TRUE);
    }
    if(g_strcmp0(propertyName, "ScreenReaderEnabled") == 0)
    {
      return g_variant_new_boolean(TRUE);
    }
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property: %s", propertyName);
    return nullptr;
  }

  // --- org.a11y.atspi.Registry (GetRegisteredEvents) ---
  bool RegisterRegistry()
  {
    static const char* xml =
      "<node>"
      "  <interface name='org.a11y.atspi.Registry'>"
      "    <method name='GetRegisteredEvents'>"
      "      <arg name='events' type='a(ss)' direction='out'/>"
      "    </method>"
      "  </interface>"
      "</node>";

    return RegisterInterface(xml, "/org/a11y/atspi/registry", &FakeAtspiBroker::HandleRegistryMethod, nullptr, nullptr);
  }

  static void HandleRegistryMethod(GDBusConnection*, const gchar*, const gchar*, const gchar*,
                                   const gchar* methodName, GVariant*, GDBusMethodInvocation* invocation,
                                   gpointer)
  {
    if(g_strcmp0(methodName, "GetRegisteredEvents") == 0)
    {
      GVariantBuilder builder;
      g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ss)"));
      g_dbus_method_invocation_return_value(invocation,
        g_variant_new("(a(ss))", &builder));
    }
    else
    {
      g_dbus_method_invocation_return_dbus_error(invocation,
        "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
    }
  }

  // --- org.a11y.atspi.Socket stub at /org/a11y/atspi/accessible/root ---
  bool RegisterSocket()
  {
    static const char* xml =
      "<node>"
      "  <interface name='org.a11y.atspi.Socket'>"
      "    <method name='Embed'>"
      "      <arg name='plug' type='(so)' direction='in'/>"
      "      <arg name='parent' type='(so)' direction='out'/>"
      "    </method>"
      "    <method name='Unembed'>"
      "      <arg name='plug' type='(so)' direction='in'/>"
      "    </method>"
      "  </interface>"
      "</node>";

    return RegisterInterface(xml, "/org/a11y/atspi/accessible/root", &FakeAtspiBroker::HandleSocketMethod, nullptr, nullptr);
  }

  static void HandleSocketMethod(GDBusConnection*, const gchar*, const gchar*, const gchar*,
                                 const gchar* methodName, GVariant*, GDBusMethodInvocation* invocation,
                                 gpointer)
  {
    if(g_strcmp0(methodName, "Embed") == 0)
    {
      // Return a dummy parent address: ("org.a11y.atspi.Registry", "/org/a11y/atspi/accessible/root")
      g_dbus_method_invocation_return_value(invocation,
        g_variant_new("((so))", "org.a11y.atspi.Registry", "/org/a11y/atspi/accessible/root"));
    }
    else if(g_strcmp0(methodName, "Unembed") == 0)
    {
      g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else
    {
      g_dbus_method_invocation_return_dbus_error(invocation,
        "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
    }
  }

  // --- Generic interface registration helper ---
  bool RegisterInterface(const char* xml, const char* path,
                         GDBusInterfaceMethodCallFunc methodHandler,
                         GDBusInterfaceGetPropertyFunc getPropertyHandler,
                         GDBusInterfaceSetPropertyFunc setPropertyHandler)
  {
    GError* err = nullptr;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(xml, &err);
    if(err)
    {
      std::cerr << "FakeAtspiBroker: XML parse error: " << err->message << std::endl;
      g_error_free(err);
      return false;
    }
    mNodeInfos.push_back(nodeInfo);

    // Each registration needs its own vtable — GDBus stores a pointer, not a copy.
    auto* vtable = new GDBusInterfaceVTable{};
    vtable->method_call  = methodHandler;
    vtable->get_property = getPropertyHandler;
    vtable->set_property = setPropertyHandler;
    mVtables.push_back(vtable);

    guint regId = g_dbus_connection_register_object(
      mConnection,
      path,
      nodeInfo->interfaces[0],
      vtable,
      this,
      nullptr,
      &err);

    if(err)
    {
      std::cerr << "FakeAtspiBroker: registration failed at " << path << ": " << err->message << std::endl;
      g_error_free(err);
      return false;
    }

    mRegistrationIds.push_back(regId);
    return true;
  }
};

#endif // ACCESSIBILITY_TEST_GDBUS_TEST_UTILS_H
