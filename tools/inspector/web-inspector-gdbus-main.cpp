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
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>

#include <cpp-httplib/httplib.h>

// INTERNAL INCLUDES
#include <test/gdbus/gdbus-test-utils.h>
#include <test/test-accessible.h>
#include <tools/inspector/query-engine.h>
#include <tools/inspector/web-inspector-resources.h>

namespace
{
volatile std::sig_atomic_t gRunning = 1;

void SignalHandler(int)
{
  gRunning = 0;
}

/**
 * @brief Escapes a string for safe embedding in JSON.
 */
std::string JsonEscape(const std::string& s)
{
  std::string out;
  out.reserve(s.size() + 8);
  for(char c : s)
  {
    switch(c)
    {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if(static_cast<unsigned char>(c) < 0x20)
        {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        }
        else
        {
          out += c;
        }
        break;
    }
  }
  return out;
}

/**
 * @brief Serializes an ElementInfo to a JSON string.
 */
std::string ElementInfoToJson(const InspectorEngine::ElementInfo& info)
{
  std::string json = "{";
  json += "\"id\":" + std::to_string(info.id);
  json += ",\"name\":\"" + JsonEscape(info.name) + "\"";
  json += ",\"role\":\"" + JsonEscape(info.role) + "\"";
  json += ",\"description\":\"" + JsonEscape(info.description) + "\"";
  json += ",\"states\":\"" + JsonEscape(info.states) + "\"";
  json += ",\"boundsX\":" + std::to_string(info.boundsX);
  json += ",\"boundsY\":" + std::to_string(info.boundsY);
  json += ",\"boundsWidth\":" + std::to_string(info.boundsWidth);
  json += ",\"boundsHeight\":" + std::to_string(info.boundsHeight);
  json += ",\"childCount\":" + std::to_string(info.childCount);
  json += ",\"childIds\":[";
  for(size_t i = 0; i < info.childIds.size(); ++i)
  {
    if(i > 0) json += ",";
    json += std::to_string(info.childIds[i]);
  }
  json += "]";
  json += ",\"parentId\":" + std::to_string(info.parentId);
  json += "}";
  return json;
}

/**
 * @brief Serializes a TreeNode to a JSON string (recursive).
 */
std::string TreeNodeToJson(const InspectorEngine::TreeNode& node)
{
  std::string json = "{";
  json += "\"id\":" + std::to_string(node.id);
  json += ",\"name\":\"" + JsonEscape(node.name) + "\"";
  json += ",\"role\":\"" + JsonEscape(node.role) + "\"";
  json += ",\"childCount\":" + std::to_string(node.childCount);
  json += ",\"children\":[";
  for(size_t i = 0; i < node.children.size(); ++i)
  {
    if(i > 0) json += ",";
    json += TreeNodeToJson(node.children[i]);
  }
  json += "]";
  json += "}";
  return json;
}

} // anonymous namespace

int main(int argc, char** argv)
{
  int port = 8080;
  if(argc > 1)
  {
    port = std::atoi(argv[1]);
    if(port <= 0 || port > 65535)
    {
      fprintf(stderr, "Invalid port: %s\n", argv[1]);
      return 1;
    }
  }

  printf("=== GDBus Web Accessibility Inspector ===\n");
  printf("(Real D-Bus IPC — full serialization round-trip)\n\n");

  // Step 1: Start private dbus-daemon
  TestDBusFixture dbusFixture;
  if(!dbusFixture.Setup())
  {
    fprintf(stderr, "FATAL: Could not start dbus-daemon.\n");
    fprintf(stderr, "  On macOS: brew install dbus && brew services start dbus\n");
    return 1;
  }
  printf("Private dbus-daemon: %s\n", dbusFixture.busAddress.c_str());

  // Step 2: Create GDBusConnection for the broker
  GError* err = nullptr;
  GDBusConnection* brokerConn = g_dbus_connection_new_for_address_sync(
    dbusFixture.busAddress.c_str(),
    static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                      G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
    nullptr,
    nullptr,
    &err);
  if(!brokerConn)
  {
    fprintf(stderr, "FATAL: Could not connect to private bus: %s\n",
            err ? err->message : "unknown");
    if(err) g_error_free(err);
    return 1;
  }

  // Step 3: Register FakeAtspiBroker
  FakeAtspiBroker broker(dbusFixture.busAddress);
  if(!broker.Register(brokerConn))
  {
    fprintf(stderr, "FATAL: Could not register FakeAtspiBroker.\n");
    g_object_unref(brokerConn);
    return 1;
  }
  PumpMainLoop(50);
  printf("FakeAtspiBroker registered.\n");

  // Step 4: Initialize engine with GDBus backend
  InspectorEngine::AccessibilityQueryEngine engine;
  auto pumpFn = [](int iterations) { PumpMainLoop(iterations); };

  if(!engine.InitializeGDBus(dbusFixture.busAddress, pumpFn))
  {
    fprintf(stderr, "Failed to initialize GDBus accessibility engine.\n");
    broker.Unregister();
    g_object_unref(brokerConn);
    return 1;
  }
  printf("GDBus accessibility engine initialized (root=%u).\n", engine.GetRootId());

  // Step 5: Set up signal handlers
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  // Step 6: Set up HTTP server
  std::mutex engineMutex;
  httplib::Server server;

  server.Get("/", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(WebInspectorResources::HTML, "text/html");
  });

  server.Get("/api/tree", [&](const httplib::Request&, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(engineMutex);
    pumpFn(50);
    auto tree = engine.BuildTree(engine.GetRootId());
    std::string json = "{";
    json += "\"focusedId\":" + std::to_string(engine.GetFocusedId());
    json += ",\"tree\":" + TreeNodeToJson(tree);
    json += "}";
    res.set_content(json, "application/json");
  });

  server.Get(R"(/api/element/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
    uint32_t id = static_cast<uint32_t>(std::stoul(req.matches[1]));
    std::lock_guard<std::mutex> lock(engineMutex);
    pumpFn(50);
    auto info = engine.GetElementInfo(id);
    res.set_content(ElementInfoToJson(info), "application/json");
  });

  server.Post("/api/navigate", [&](const httplib::Request& req, httplib::Response& res) {
    std::string direction;
    auto pos = req.body.find("\"direction\"");
    if(pos != std::string::npos)
    {
      auto colonPos = req.body.find(':', pos);
      if(colonPos != std::string::npos)
      {
        auto quoteStart = req.body.find('"', colonPos + 1);
        auto quoteEnd   = req.body.find('"', quoteStart + 1);
        if(quoteStart != std::string::npos && quoteEnd != std::string::npos)
        {
          direction = req.body.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
      }
    }

    std::lock_guard<std::mutex> lock(engineMutex);
    pumpFn(50);

    uint32_t currentId = engine.GetFocusedId();
    uint32_t newId     = currentId;

    if(direction == "next")
    {
      newId = engine.Navigate(currentId, true);
    }
    else if(direction == "prev")
    {
      newId = engine.Navigate(currentId, false);
    }
    else if(direction == "child")
    {
      newId = engine.NavigateChild(currentId);
    }
    else if(direction == "parent")
    {
      newId = engine.NavigateParent(currentId);
    }

    pumpFn(50);
    engine.SetFocusedId(newId);
    auto info = engine.GetElementInfo(newId);

    std::string json = "{";
    json += "\"focusedId\":" + std::to_string(newId);
    json += ",\"changed\":" + std::string(newId != currentId ? "true" : "false");
    json += ",\"element\":" + ElementInfoToJson(info);
    json += "}";
    res.set_content(json, "application/json");
  });

  // Step 7: Start HTTP server on background thread
  printf("Starting web server on http://localhost:%d\n", port);
  printf("Press Ctrl+C to stop.\n\n");

  std::thread serverThread([&]() {
    server.listen("0.0.0.0", port);
  });

  // Step 8: Main thread pumps GLib main context for D-Bus events
  while(gRunning)
  {
    g_main_context_iteration(nullptr, FALSE);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Step 9: Cleanup
  printf("\nShutting down...\n");
  server.stop();
  if(serverThread.joinable())
  {
    serverThread.join();
  }

  engine.Shutdown();
  broker.Unregister();
  g_object_unref(brokerConn);

  printf("Done.\n");
  return 0;
}
