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
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

// INTERNAL INCLUDES
#include <accessibility/internal/service/inspector-service.h>
#include <test/mock/mock-app-registry.h>
#include <test/mock/mock-gesture-provider.h>

namespace
{
volatile std::sig_atomic_t gRunning = 1;

void SignalHandler(int)
{
  gRunning = 0;
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

  printf("=== InspectorService Web Inspector ===\n\n");

  auto registry = std::make_unique<MockAppRegistry>();
  auto gesture  = std::make_unique<MockGestureProvider>();

  Accessibility::InspectorService::Config config;
  config.port = port;

  Accessibility::InspectorService service(std::move(registry), std::move(gesture), config);
  service.startInspector();

  printf("Web inspector: http://localhost:%d\n", port);
  printf("Press Ctrl+C to stop.\n\n");

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
  while(gRunning)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  printf("\nShutting down...\n");
  service.stopInspector();

  return 0;
}
