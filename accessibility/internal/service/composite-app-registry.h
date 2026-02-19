#ifndef ACCESSIBILITY_INTERNAL_SERVICE_COMPOSITE_APP_REGISTRY_H
#define ACCESSIBILITY_INTERNAL_SERVICE_COMPOSITE_APP_REGISTRY_H

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
 * @brief Composite AppRegistry that merges D-Bus and TIDL app registries.
 *
 * Combines two AppRegistry implementations (e.g., AtSpiAppRegistry and TidlAppRegistry)
 * into a single unified registry. Applications from both sources appear in one list.
 */
class CompositeAppRegistry : public AppRegistry
{
public:
  /**
   * @brief Constructs a CompositeAppRegistry.
   *
   * @param[in] atspiRegistry The D-Bus AT-SPI registry (for web, GTK, Qt apps)
   * @param[in] tidlRegistry The TIDL registry (for DALi apps)
   */
  CompositeAppRegistry(std::unique_ptr<AppRegistry> atspiRegistry,
                       std::unique_ptr<AppRegistry> tidlRegistry);

  ~CompositeAppRegistry() override;

  std::shared_ptr<NodeProxy> getDesktop() override;
  std::shared_ptr<NodeProxy> getActiveWindow() override;
  void onAppRegistered(AppCallback callback) override;
  void onAppDeregistered(AppCallback callback) override;

private:
  std::unique_ptr<AppRegistry> mAtspiRegistry;
  std::unique_ptr<AppRegistry> mTidlRegistry;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_COMPOSITE_APP_REGISTRY_H
