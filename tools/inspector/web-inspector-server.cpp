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

// INTERNAL INCLUDES
#include <tools/inspector/web-inspector-server.h>
#include <tools/inspector/inspector-query-interface.h>
#include <tools/inspector/direct-query-engine.h>
#include <tools/inspector/web-inspector-resources.h>

// EXTERNAL INCLUDES
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

#include <cpp-httplib/httplib.h>

namespace
{
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

namespace InspectorEngine
{
struct WebInspectorServer::Impl
{
  httplib::Server server;
  std::thread     thread;
  std::mutex      engineMutex;
  bool            running{false};
};

WebInspectorServer::WebInspectorServer()
: mImpl(std::make_unique<Impl>())
{
}

WebInspectorServer::~WebInspectorServer()
{
  Stop();
}

void WebInspectorServer::Start(InspectorQueryInterface& engine, int port)
{
  if(mImpl->running) return;

  auto& engineMutex = mImpl->engineMutex;

  // Serve the embedded HTML page
  mImpl->server.Get("/", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(WebInspectorResources::HTML, "text/html");
  });

  // GET /api/tree — returns the full tree and current focused ID
  mImpl->server.Get("/api/tree", [&engine, &engineMutex](const httplib::Request&, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(engineMutex);
    auto tree = engine.BuildTree(engine.GetRootId());
    std::string json = "{";
    json += "\"focusedId\":" + std::to_string(engine.GetFocusedId());
    json += ",\"tree\":" + TreeNodeToJson(tree);
    json += "}";
    res.set_content(json, "application/json");
  });

  // GET /api/element/:id — returns element details
  mImpl->server.Get(R"(/api/element/(\d+))", [&engine, &engineMutex](const httplib::Request& req, httplib::Response& res) {
    uint32_t id = static_cast<uint32_t>(std::stoul(req.matches[1]));
    std::lock_guard<std::mutex> lock(engineMutex);
    auto info = engine.GetElementInfo(id);
    res.set_content(ElementInfoToJson(info), "application/json");
  });

  // POST /api/navigate — navigates in the given direction
  mImpl->server.Post("/api/navigate", [&engine, &engineMutex](const httplib::Request& req, httplib::Response& res) {
    // Parse direction from request body
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

    engine.SetFocusedId(newId);
    auto info = engine.GetElementInfo(newId);

    std::string json = "{";
    json += "\"focusedId\":" + std::to_string(newId);
    json += ",\"changed\":" + std::string(newId != currentId ? "true" : "false");
    json += ",\"element\":" + ElementInfoToJson(info);
    json += "}";
    res.set_content(json, "application/json");
  });

  mImpl->running = true;
  mImpl->thread = std::thread([this, port]() {
    mImpl->server.listen("0.0.0.0", port);
  });
}

void WebInspectorServer::Start(DirectQueryEngine& engine, int port)
{
  Start(static_cast<InspectorQueryInterface&>(engine), port);
}

void WebInspectorServer::Stop()
{
  if(!mImpl->running) return;
  mImpl->server.stop();
  if(mImpl->thread.joinable())
  {
    mImpl->thread.join();
  }
  mImpl->running = false;
}

bool WebInspectorServer::IsRunning() const
{
  return mImpl->running;
}

} // namespace InspectorEngine
