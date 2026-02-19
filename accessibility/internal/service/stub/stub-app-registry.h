#ifndef ACCESSIBILITY_INTERNAL_SERVICE_STUB_APP_REGISTRY_H
#define ACCESSIBILITY_INTERNAL_SERVICE_STUB_APP_REGISTRY_H

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

// INTERNAL INCLUDES
#include <accessibility/api/app-registry.h>

namespace Accessibility
{
/**
 * @brief Stub AppRegistry for platforms without D-Bus or TIDL (e.g., macOS).
 *
 * Returns a single mock window node if set, or nullptr.
 */
class StubAppRegistry : public AppRegistry
{
public:
  StubAppRegistry() = default;

  /**
   * @brief Sets the mock window to return from getActiveWindow().
   */
  void setMockWindow(std::shared_ptr<NodeProxy> window)
  {
    mMockWindow = std::move(window);
  }

  std::shared_ptr<NodeProxy> getDesktop() override
  {
    return mMockWindow;
  }

  std::shared_ptr<NodeProxy> getActiveWindow() override
  {
    return mMockWindow;
  }

  void onAppRegistered(AppCallback /*callback*/) override {}
  void onAppDeregistered(AppCallback /*callback*/) override {}

private:
  std::shared_ptr<NodeProxy> mMockWindow;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_STUB_APP_REGISTRY_H
