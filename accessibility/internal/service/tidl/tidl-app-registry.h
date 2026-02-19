#ifndef ACCESSIBILITY_INTERNAL_SERVICE_TIDL_APP_REGISTRY_H
#define ACCESSIBILITY_INTERNAL_SERVICE_TIDL_APP_REGISTRY_H

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
#include <accessibility/api/app-registry.h>

namespace Accessibility
{
/**
 * @brief TIDL scaffold implementation of AppRegistry.
 *
 * Returns empty app lists. Will be implemented with Tizen aul API
 * in Phase 2.6 Stage B when Tizen device is available.
 */
class TidlAppRegistry : public AppRegistry
{
public:
  TidlAppRegistry() = default;

  std::shared_ptr<NodeProxy> getDesktop() override { return nullptr; }
  std::shared_ptr<NodeProxy> getActiveWindow() override { return nullptr; }
  void onAppRegistered(AppCallback /*callback*/) override {}
  void onAppDeregistered(AppCallback /*callback*/) override {}
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_TIDL_APP_REGISTRY_H
