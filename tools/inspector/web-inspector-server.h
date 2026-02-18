#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_WEB_INSPECTOR_SERVER_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_WEB_INSPECTOR_SERVER_H

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
#include <memory>

namespace InspectorEngine
{
class DirectQueryEngine;

/**
 * @brief Embeddable HTTP server that serves the web inspector frontend and REST API.
 *
 * Uses PIMPL pattern to avoid leaking httplib.h into the header.
 * The server runs on a background thread and serves read-only access
 * to the DirectQueryEngine's snapshot.
 */
class WebInspectorServer
{
public:
  WebInspectorServer();
  ~WebInspectorServer();

  /**
   * @brief Starts the HTTP server on a background thread.
   *
   * @param[in] engine Reference to the DirectQueryEngine to serve data from
   * @param[in] port The port to listen on (default 8080)
   */
  void Start(DirectQueryEngine& engine, int port = 8080);

  /**
   * @brief Stops the HTTP server and joins the background thread.
   */
  void Stop();

  /**
   * @brief Checks if the server is currently running.
   */
  bool IsRunning() const;

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // namespace InspectorEngine

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_WEB_INSPECTOR_SERVER_H
