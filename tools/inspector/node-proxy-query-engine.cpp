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
#include <tools/inspector/node-proxy-query-engine.h>

// EXTERNAL INCLUDES
#include <algorithm>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/node-proxy.h>

namespace InspectorEngine
{
NodeProxyQueryEngine::NodeProxyQueryEngine() = default;
NodeProxyQueryEngine::~NodeProxyQueryEngine() = default;

std::string NodeProxyQueryEngine::RoleToString(Accessibility::Role role)
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

std::string NodeProxyQueryEngine::StatesToString(Accessibility::States states)
{
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

void NodeProxyQueryEngine::TraverseTree(std::shared_ptr<Accessibility::NodeProxy> node, uint32_t parentId, uint32_t& nextId)
{
  if(!node) return;

  uint32_t id = nextId++;

  CachedElement elem;
  elem.id          = id;
  elem.name        = node->getName();
  elem.role        = RoleToString(node->getRole());
  elem.description = node->getDescription();
  elem.states      = StatesToString(node->getStates());
  elem.parentId    = parentId;

  auto extents      = node->getExtents(Accessibility::CoordinateType::SCREEN);
  elem.boundsX      = static_cast<float>(extents.x);
  elem.boundsY      = static_cast<float>(extents.y);
  elem.boundsWidth  = static_cast<float>(extents.width);
  elem.boundsHeight = static_cast<float>(extents.height);

  auto children    = node->getChildren();
  elem.childCount  = static_cast<int>(children.size());

  // Reserve child IDs
  std::vector<uint32_t> childIds;
  uint32_t childStartId = nextId;
  for(size_t i = 0; i < children.size(); ++i)
  {
    childIds.push_back(childStartId + static_cast<uint32_t>(i));
  }
  // Adjust: we don't know subtree sizes yet, so we traverse sequentially
  childIds.clear();

  mSnapshot[id] = std::move(elem);

  for(auto& child : children)
  {
    uint32_t childId = nextId;
    mSnapshot[id].childIds.push_back(childId);
    TraverseTree(child, id, nextId);
  }
  mSnapshot[id].childCount = static_cast<int>(mSnapshot[id].childIds.size());
}

void NodeProxyQueryEngine::BuildSnapshot(std::shared_ptr<Accessibility::NodeProxy> root)
{
  std::lock_guard<std::mutex> lock(mMutex);

  mSnapshot.clear();
  mHighlightableOrder.clear();
  if(!root) return;

  uint32_t nextId = 1;
  mRootId = nextId;
  TraverseTree(root, 0, nextId);

  BuildHighlightableOrder(mRootId);

  if(mFocusedId == 0 && !mHighlightableOrder.empty())
  {
    mFocusedId = mHighlightableOrder.front();
  }
}

void NodeProxyQueryEngine::BuildHighlightableOrder(uint32_t nodeId)
{
  auto it = mSnapshot.find(nodeId);
  if(it == mSnapshot.end()) return;

  auto& elem = it->second;
  if(elem.states.find("HIGHLIGHTABLE") != std::string::npos)
  {
    mHighlightableOrder.push_back(nodeId);
  }

  for(auto childId : elem.childIds)
  {
    BuildHighlightableOrder(childId);
  }
}

uint32_t NodeProxyQueryEngine::GetRootId() const
{
  std::lock_guard<std::mutex> lock(mMutex);
  return mRootId;
}

uint32_t NodeProxyQueryEngine::GetFocusedId() const
{
  std::lock_guard<std::mutex> lock(mMutex);
  return mFocusedId;
}

void NodeProxyQueryEngine::SetFocusedId(uint32_t id)
{
  {
    std::lock_guard<std::mutex> lock(mMutex);
    mFocusedId = id;
  }
  if(mFocusChangedCallback)
  {
    mFocusChangedCallback(id);
  }
}

void NodeProxyQueryEngine::SetFocusChangedCallback(std::function<void(uint32_t)> callback)
{
  mFocusChangedCallback = std::move(callback);
}

ElementInfo NodeProxyQueryEngine::GetElementInfo(uint32_t id)
{
  std::lock_guard<std::mutex> lock(mMutex);

  auto it = mSnapshot.find(id);
  if(it == mSnapshot.end())
  {
    ElementInfo info{};
    info.id    = id;
    info.name  = "(not found)";
    info.role  = "UNKNOWN";
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

TreeNode NodeProxyQueryEngine::BuildTree(uint32_t rootId)
{
  std::lock_guard<std::mutex> lock(mMutex);

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

  auto& elem       = it->second;
  node.name        = elem.name;
  node.role        = elem.role;
  node.childCount  = elem.childCount;

  for(auto childId : elem.childIds)
  {
    // Temporarily release lock for recursive call — but since
    // snapshot is only modified in BuildSnapshot, this is safe
    // as long as BuildSnapshot is not called concurrently.
    // For simplicity, use non-locking internal helper.
    TreeNode childNode;
    childNode.id = childId;
    auto childIt = mSnapshot.find(childId);
    if(childIt != mSnapshot.end())
    {
      childNode.name       = childIt->second.name;
      childNode.role       = childIt->second.role;
      childNode.childCount = childIt->second.childCount;
      // Build children recursively (inline to avoid deadlock)
      std::function<void(TreeNode&)> buildChildren = [&](TreeNode& parent)
      {
        auto parentIt = mSnapshot.find(parent.id);
        if(parentIt == mSnapshot.end()) return;
        for(auto cid : parentIt->second.childIds)
        {
          TreeNode cn;
          cn.id = cid;
          auto cit = mSnapshot.find(cid);
          if(cit != mSnapshot.end())
          {
            cn.name       = cit->second.name;
            cn.role       = cit->second.role;
            cn.childCount = cit->second.childCount;
            buildChildren(cn);
          }
          parent.children.push_back(std::move(cn));
        }
      };
      buildChildren(childNode);
    }
    node.children.push_back(std::move(childNode));
  }

  return node;
}

uint32_t NodeProxyQueryEngine::Navigate(uint32_t currentId, bool forward)
{
  std::lock_guard<std::mutex> lock(mMutex);

  if(mHighlightableOrder.empty()) return currentId;

  auto it = std::find(mHighlightableOrder.begin(), mHighlightableOrder.end(), currentId);

  if(it == mHighlightableOrder.end())
  {
    return mHighlightableOrder.front();
  }

  if(forward)
  {
    ++it;
    if(it == mHighlightableOrder.end())
    {
      it = mHighlightableOrder.begin();
    }
  }
  else
  {
    if(it == mHighlightableOrder.begin())
    {
      it = mHighlightableOrder.end();
    }
    --it;
  }

  return *it;
}

uint32_t NodeProxyQueryEngine::NavigateChild(uint32_t currentId)
{
  std::lock_guard<std::mutex> lock(mMutex);

  auto it = mSnapshot.find(currentId);
  if(it == mSnapshot.end() || it->second.childIds.empty())
  {
    return currentId;
  }
  return it->second.childIds.front();
}

uint32_t NodeProxyQueryEngine::NavigateParent(uint32_t currentId)
{
  std::lock_guard<std::mutex> lock(mMutex);

  auto it = mSnapshot.find(currentId);
  if(it == mSnapshot.end() || it->second.parentId == 0)
  {
    return currentId;
  }
  return it->second.parentId;
}

size_t NodeProxyQueryEngine::GetSnapshotSize() const
{
  std::lock_guard<std::mutex> lock(mMutex);
  return mSnapshot.size();
}

} // namespace InspectorEngine
