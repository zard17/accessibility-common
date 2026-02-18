#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_TYPES_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_TYPES_H

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
#include <cstdint>
#include <string>
#include <vector>

namespace InspectorEngine
{
/**
 * @brief Information about an accessible element.
 */
struct ElementInfo
{
  uint32_t    id;
  std::string name;
  std::string role;
  std::string description;
  std::string states;
  float       boundsX;
  float       boundsY;
  float       boundsWidth;
  float       boundsHeight;
  int         childCount;
  std::vector<uint32_t> childIds;
  uint32_t    parentId;
};

/**
 * @brief A node in the accessibility tree.
 */
struct TreeNode
{
  uint32_t    id;
  std::string name;
  std::string role;
  int         childCount;
  std::vector<TreeNode> children;
};

} // namespace InspectorEngine

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_TYPES_H
