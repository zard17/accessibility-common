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
#include <tools/inspector/direct-query-engine.h>

// EXTERNAL INCLUDES
#include <cstdio>

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>
#include <accessibility/api/accessibility.h>

namespace InspectorEngine
{
DirectQueryEngine::DirectQueryEngine() = default;
DirectQueryEngine::~DirectQueryEngine() = default;

std::string DirectQueryEngine::RoleToString(Accessibility::Role role)
{
  static const char* names[] = {
    "INVALID", "ACCELERATOR_LABEL", "ALERT", "ANIMATION", "ARROW", "CALENDAR",
    "CANVAS", "CHECK_BOX", "CHECK_MENU_ITEM", "COLOR_CHOOSER", "COLUMN_HEADER",
    "COMBO_BOX", "DATE_EDITOR", "DESKTOP_ICON", "DESKTOP_FRAME", "DIAL", "DIALOG",
    "DIRECTORY_PANE", "DRAWING_AREA", "FILE_CHOOSER", "FILLER", "FOCUS_TRAVERSABLE",
    "FONT_CHOOSER", "FRAME", "GLASS_PANE", "HTML_CONTAINER", "ICON", "IMAGE",
    "INTERNAL_FRAME", "LABEL", "LAYERED_PANE", "LIST", "LIST_ITEM", "MENU",
    "MENU_BAR", "MENU_ITEM", "OPTION_PANE", "PAGE_TAB", "PAGE_TAB_LIST", "PANEL",
    "PASSWORD_TEXT", "POPUP_MENU", "PROGRESS_BAR", "PUSH_BUTTON", "RADIO_BUTTON",
    "RADIO_MENU_ITEM", "ROOT_PANE", "ROW_HEADER", "SCROLL_BAR", "SCROLL_PANE",
    "SEPARATOR", "SLIDER", "SPIN_BUTTON", "SPLIT_PANE", "STATUS_BAR", "TABLE",
    "TABLE_CELL", "TABLE_COLUMN_HEADER", "TABLE_ROW_HEADER", "TEAROFF_MENU_ITEM",
    "TERMINAL", "TEXT", "TOGGLE_BUTTON", "TOOL_BAR", "TOOL_TIP", "TREE", "TREE_TABLE",
    "UNKNOWN", "VIEWPORT", "WINDOW", "EXTENDED", "HEADER", "FOOTER", "PARAGRAPH",
    "RULER", "APPLICATION", "AUTOCOMPLETE", "EDITBAR", "EMBEDDED", "ENTRY", "CHART",
    "CAPTION", "DOCUMENT_FRAME", "HEADING", "PAGE", "SECTION", "REDUNDANT_OBJECT",
    "FORM", "LINK", "INPUT_METHOD_WINDOW", "TABLE_ROW", "TREE_ITEM", "DOCUMENT_SPREADSHEET",
    "DOCUMENT_PRESENTATION", "DOCUMENT_TEXT", "DOCUMENT_WEB", "DOCUMENT_EMAIL",
    "COMMENT", "LIST_BOX", "GROUPING", "IMAGE_MAP", "NOTIFICATION", "INFO_BAR",
    "LEVEL_BAR", "TITLE_BAR", "BLOCK_QUOTE", "AUDIO", "VIDEO", "DEFINITION",
    "ARTICLE", "LANDMARK", "LOG", "MARQUEE", "MATH", "RATING", "TIMER", "STATIC",
    "MATH_FRACTION", "MATH_ROOT", "SUBSCRIPT", "SUPERSCRIPT"
  };
  auto idx = static_cast<size_t>(role);
  if(idx < sizeof(names) / sizeof(names[0]))
  {
    return names[idx];
  }
  return "ROLE_" + std::to_string(idx);
}

std::string DirectQueryEngine::StatesToString(Accessibility::Accessible* accessible)
{
  auto states = accessible->GetStates();

  static const std::pair<Accessibility::State, const char*> stateNames[] = {
    {Accessibility::State::ENABLED, "ENABLED"},
    {Accessibility::State::VISIBLE, "VISIBLE"},
    {Accessibility::State::SHOWING, "SHOWING"},
    {Accessibility::State::SENSITIVE, "SENSITIVE"},
    {Accessibility::State::FOCUSABLE, "FOCUSABLE"},
    {Accessibility::State::FOCUSED, "FOCUSED"},
    {Accessibility::State::ACTIVE, "ACTIVE"},
    {Accessibility::State::CHECKED, "CHECKED"},
    {Accessibility::State::SELECTED, "SELECTED"},
    {Accessibility::State::EXPANDED, "EXPANDED"},
    {Accessibility::State::PRESSED, "PRESSED"},
    {Accessibility::State::HIGHLIGHTABLE, "HIGHLIGHTABLE"},
    {Accessibility::State::HIGHLIGHTED, "HIGHLIGHTED"},
    {Accessibility::State::EDITABLE, "EDITABLE"},
    {Accessibility::State::READ_ONLY, "READ_ONLY"},
  };

  std::string result;
  for(auto& [state, name] : stateNames)
  {
    if(states[state])
    {
      if(!result.empty()) result += ", ";
      result += name;
    }
  }
  return result.empty() ? "(none)" : result;
}

uint32_t DirectQueryEngine::ExtractId(Accessibility::Accessible* accessible)
{
  auto path = accessible->GetAddress().GetPath();
  try
  {
    return static_cast<uint32_t>(std::stoul(path));
  }
  catch(...)
  {
    return 0;
  }
}

void DirectQueryEngine::TraverseTree(Accessibility::Accessible* node, uint32_t parentId)
{
  if(!node) return;

  uint32_t id = ExtractId(node);
  if(id == 0) return;

  CachedElement elem;
  elem.id          = id;
  elem.name        = node->GetName();
  elem.role        = RoleToString(node->GetRole());
  elem.description = node->GetDescription();
  elem.states      = StatesToString(node);
  elem.parentId    = parentId;

  auto extents     = node->GetExtents(Accessibility::CoordinateType::SCREEN);
  elem.boundsX     = extents.x;
  elem.boundsY     = extents.y;
  elem.boundsWidth = extents.width;
  elem.boundsHeight = extents.height;

  auto children     = node->GetChildren();
  elem.childCount   = static_cast<int>(children.size());
  for(auto* child : children)
  {
    uint32_t childId = ExtractId(child);
    if(childId != 0)
    {
      elem.childIds.push_back(childId);
    }
  }

  mSnapshot[id] = std::move(elem);

  for(auto* child : children)
  {
    TraverseTree(child, id);
  }
}

void DirectQueryEngine::BuildSnapshot(Accessibility::Accessible* root)
{
  mSnapshot.clear();
  if(!root) return;

  mRootId = ExtractId(root);
  TraverseTree(root, 0);

  // Default focused to first highlightable element
  if(mFocusedId == 0)
  {
    for(auto& [id, elem] : mSnapshot)
    {
      if(elem.states.find("HIGHLIGHTABLE") != std::string::npos)
      {
        mFocusedId = id;
        break;
      }
    }
  }
}

uint32_t DirectQueryEngine::GetRootId() const
{
  return mRootId;
}

uint32_t DirectQueryEngine::GetFocusedId() const
{
  return mFocusedId;
}

void DirectQueryEngine::SetFocusedId(uint32_t id)
{
  mFocusedId = id;
}

ElementInfo DirectQueryEngine::GetElementInfo(uint32_t id)
{
  auto it = mSnapshot.find(id);
  if(it == mSnapshot.end())
  {
    ElementInfo info{};
    info.id   = id;
    info.name = "(not found)";
    info.role = "UNKNOWN";
    info.states = "(none)";
    return info;
  }

  auto& elem = it->second;
  ElementInfo info;
  info.id           = elem.id;
  info.name         = elem.name;
  info.role         = elem.role;
  info.description  = elem.description;
  info.states       = elem.states;
  info.boundsX      = elem.boundsX;
  info.boundsY      = elem.boundsY;
  info.boundsWidth  = elem.boundsWidth;
  info.boundsHeight = elem.boundsHeight;
  info.childCount   = elem.childCount;
  info.childIds     = elem.childIds;
  info.parentId     = elem.parentId;
  return info;
}

TreeNode DirectQueryEngine::BuildTree(uint32_t rootId)
{
  TreeNode node;
  node.id = rootId;

  auto it = mSnapshot.find(rootId);
  if(it == mSnapshot.end())
  {
    node.name       = "(not found)";
    node.role       = "UNKNOWN";
    node.childCount = 0;
    return node;
  }

  auto& elem      = it->second;
  node.name        = elem.name;
  node.role        = elem.role;
  node.childCount  = elem.childCount;

  for(auto childId : elem.childIds)
  {
    node.children.push_back(BuildTree(childId));
  }

  return node;
}

} // namespace InspectorEngine
