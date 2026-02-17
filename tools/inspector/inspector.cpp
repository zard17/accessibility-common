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
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/api/accessible.h>
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <test/mock/mock-dbus-wrapper.h>
#include <test/test-accessible.h>
#include <tools/inspector/tts.h>

namespace
{
/**
 * @brief Makes a D-Bus object path for the given accessible ID.
 */
std::string MakeObjectPath(uint32_t id)
{
  return std::string{ATSPI_PREFIX_PATH} + std::to_string(id);
}

/**
 * @brief Creates a DBusClient targeting a specific accessible in the bridge.
 */
DBus::DBusClient CreateClient(const std::string& busName, uint32_t id,
                               const std::string& iface,
                               const DBusWrapper::ConnectionPtr& conn)
{
  return DBus::DBusClient{busName, MakeObjectPath(id), iface, conn};
}

/**
 * @brief Returns the role name string for the given Role enum.
 */
std::string RoleToString(Accessibility::Role role)
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

/**
 * @brief Recursively prints the accessibility tree.
 */
void PrintTree(const std::string& busName,
               uint32_t id,
               const DBusWrapper::ConnectionPtr& conn,
               int depth,
               uint32_t focusedId)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client = CreateClient(busName, id, accIface, conn);

  // Get name
  std::string name = "(unknown)";
  auto nameResult = client.property<std::string>("Name").get();
  if(nameResult)
  {
    name = std::get<0>(nameResult.getValues());
  }

  // Get role
  std::string roleName = "?";
  auto roleResult = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
  if(roleResult)
  {
    roleName = RoleToString(static_cast<Accessibility::Role>(std::get<0>(roleResult.getValues())));
  }

  // Get child count
  int childCount = 0;
  auto ccResult = client.property<int>("ChildCount").get();
  if(ccResult)
  {
    childCount = std::get<0>(ccResult.getValues());
  }

  // Print this node
  bool isFocused = (id == focusedId);
  for(int i = 0; i < depth; ++i) printf("  ");
  printf("%s[%s] \"%s\"", isFocused ? ">> " : "", roleName.c_str(), name.c_str());
  if(childCount > 0)
  {
    printf(" (%d children)", childCount);
  }
  printf("\n");

  // Recurse into children
  for(int i = 0; i < childCount; ++i)
  {
    auto childResult = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(i);
    if(childResult)
    {
      auto childAddr = std::get<0>(childResult.getValues());
      // Parse child ID from the path
      auto childPath = childAddr.GetPath();
      uint32_t childId = 0;
      try
      {
        childId = static_cast<uint32_t>(std::stoul(childPath));
      }
      catch(...)
      {
        continue;
      }
      PrintTree(busName, childId, conn, depth + 1, focusedId);
    }
  }
}

/**
 * @brief Reads and prints information about an accessible node.
 */
void ReadElement(const std::string& busName, uint32_t id,
                 const DBusWrapper::ConnectionPtr& conn)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto compIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::COMPONENT);
  auto client = CreateClient(busName, id, accIface, conn);

  // Name
  std::string name = "(unknown)";
  auto nameResult = client.property<std::string>("Name").get();
  if(nameResult) name = std::get<0>(nameResult.getValues());

  // Description
  std::string desc;
  auto descResult = client.property<std::string>("Description").get();
  if(descResult) desc = std::get<0>(descResult.getValues());

  // Role
  std::string roleName = "?";
  auto roleResult = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
  if(roleResult) roleName = RoleToString(static_cast<Accessibility::Role>(std::get<0>(roleResult.getValues())));

  // States
  std::string stateStr;
  auto stateResult = client.method<DBus::ValueOrError<std::array<uint32_t, 2>>()>("GetState").call();
  if(stateResult)
  {
    auto stateData = std::get<0>(stateResult.getValues());
    Accessibility::States states{stateData};
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
      {Accessibility::State::HIGHLIGHTED, "HIGHLIGHTED"},
      {Accessibility::State::EDITABLE, "EDITABLE"},
      {Accessibility::State::READ_ONLY, "READ_ONLY"},
    };
    for(auto& [state, name] : stateNames)
    {
      if(states[state])
      {
        if(!stateStr.empty()) stateStr += ", ";
        stateStr += name;
      }
    }
  }
  if(stateStr.empty()) stateStr = "(none)";

  // Extents (via Component interface)
  auto compClient = CreateClient(busName, id, compIface, conn);
  std::string boundsStr = "(unavailable)";
  auto extResult = compClient.method<DBus::ValueOrError<std::tuple<int32_t, int32_t, int32_t, int32_t>>(uint32_t)>("GetExtents")
    .call(static_cast<uint32_t>(Accessibility::CoordinateType::SCREEN));
  if(extResult)
  {
    auto extents = std::get<0>(extResult.getValues());
    boundsStr = "(" + std::to_string(std::get<0>(extents)) + ", " +
                std::to_string(std::get<1>(extents)) + ", " +
                std::to_string(std::get<2>(extents)) + "x" +
                std::to_string(std::get<3>(extents)) + ")";
  }

  printf("\n");
  printf("  Name:        %s\n", name.c_str());
  printf("  Role:        %s\n", roleName.c_str());
  if(!desc.empty())
    printf("  Description: %s\n", desc.c_str());
  printf("  States:      %s\n", stateStr.c_str());
  printf("  Bounds:      %s\n", boundsStr.c_str());
  printf("\n");
}

/**
 * @brief Navigates to a neighbor in the given direction using GetNeighbor.
 *
 * @param[in] busName Bus name of the bridge
 * @param[in] currentId Current focused element ID
 * @param[in] rootId Root element ID (defines navigation scope)
 * @param[in] conn D-Bus connection
 * @param[in] forward True for next, false for previous
 * @return The new focused element ID, or currentId if navigation failed
 */
uint32_t Navigate(const std::string& busName, uint32_t currentId,
                  uint32_t rootId,
                  const DBusWrapper::ConnectionPtr& conn, bool forward)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client = CreateClient(busName, currentId, accIface, conn);

  // GetNeighbor(rootPath, direction, searchMode)
  // direction: 1 = forward, 0 = backward
  // searchMode: 1 = RECURSE_FROM_ROOT (allows full tree traversal)
  std::string rootPath = MakeObjectPath(rootId);
  auto result = client.method<DBus::ValueOrError<Accessibility::Address, uint8_t>(std::string, int32_t, int32_t)>("GetNeighbor")
    .call(rootPath, forward ? 1 : 0, 1);

  if(result)
  {
    auto addr = std::get<0>(result.getValues());
    auto path = addr.GetPath();
    try
    {
      return static_cast<uint32_t>(std::stoul(path));
    }
    catch(...)
    {
    }
  }
  return currentId;
}

/**
 * @brief Navigates to the first child of the current element.
 */
uint32_t NavigateChild(const std::string& busName, uint32_t currentId,
                       const DBusWrapper::ConnectionPtr& conn)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client = CreateClient(busName, currentId, accIface, conn);

  auto ccResult = client.property<int>("ChildCount").get();
  if(ccResult && std::get<0>(ccResult.getValues()) > 0)
  {
    auto childResult = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(0);
    if(childResult)
    {
      auto addr = std::get<0>(childResult.getValues());
      try
      {
        return static_cast<uint32_t>(std::stoul(addr.GetPath()));
      }
      catch(...)
      {
      }
    }
  }
  return currentId;
}

/**
 * @brief Navigates to the parent of the current element.
 */
uint32_t NavigateParent(const std::string& busName, uint32_t currentId,
                        const DBusWrapper::ConnectionPtr& conn)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client = CreateClient(busName, currentId, accIface, conn);

  auto parentResult = client.property<Accessibility::Address>("Parent").get();
  if(parentResult)
  {
    auto addr = std::get<0>(parentResult.getValues());
    auto path = addr.GetPath();
    try
    {
      return static_cast<uint32_t>(std::stoul(path));
    }
    catch(...)
    {
      // "root" or other non-numeric path — stay where we are
    }
  }
  return currentId;
}

/**
 * @brief Builds a demo accessible tree resembling a typical DALi app.
 */
struct DemoTree
{
  std::shared_ptr<TestAccessible> window;
  std::shared_ptr<TestAccessible> header;
  std::shared_ptr<TestAccessible> menuBtn;
  std::shared_ptr<TestAccessible> titleLabel;
  std::shared_ptr<TestAccessible> content;
  std::shared_ptr<TestAccessible> playBtn;
  std::shared_ptr<TestAccessible> volumeSlider;
  std::shared_ptr<TestAccessible> nowPlayingLabel;
  std::shared_ptr<TestAccessible> footer;
  std::shared_ptr<TestAccessible> prevBtn;
  std::shared_ptr<TestAccessible> nextBtn;

  std::vector<std::shared_ptr<TestAccessible>> all;
};

DemoTree BuildDemoTree()
{
  DemoTree t;

  auto makeStates = [](bool focusable = false, bool active = false) {
    Accessibility::States s;
    s[Accessibility::State::ENABLED]   = true;
    s[Accessibility::State::VISIBLE]   = true;
    s[Accessibility::State::SHOWING]   = true;
    s[Accessibility::State::SENSITIVE] = true;
    if(focusable)
    {
      s[Accessibility::State::FOCUSABLE]     = true;
      s[Accessibility::State::HIGHLIGHTABLE] = true;
    }
    if(active) s[Accessibility::State::ACTIVE] = true;
    return s;
  };

  // Window
  t.window = std::make_shared<TestAccessible>("Main Window", Accessibility::Role::WINDOW);
  t.window->SetStates(makeStates(false, true));
  t.window->SetExtents({0.0f, 0.0f, 480.0f, 800.0f});

  // Header panel
  t.header = std::make_shared<TestAccessible>("Header", Accessibility::Role::PANEL);
  t.header->SetStates(makeStates());
  t.header->SetExtents({0.0f, 0.0f, 480.0f, 60.0f});

  t.menuBtn = std::make_shared<TestAccessible>("Menu", Accessibility::Role::PUSH_BUTTON);
  t.menuBtn->SetStates(makeStates(true));
  t.menuBtn->SetExtents({10.0f, 10.0f, 40.0f, 40.0f});

  t.titleLabel = std::make_shared<TestAccessible>("My DALi App", Accessibility::Role::LABEL);
  t.titleLabel->SetStates(makeStates());
  t.titleLabel->SetExtents({60.0f, 10.0f, 360.0f, 40.0f});

  // Content panel
  t.content = std::make_shared<TestAccessible>("Content", Accessibility::Role::PANEL);
  t.content->SetStates(makeStates());
  t.content->SetExtents({0.0f, 60.0f, 480.0f, 680.0f});

  t.playBtn = std::make_shared<TestAccessible>("Play", Accessibility::Role::PUSH_BUTTON);
  t.playBtn->SetStates(makeStates(true));
  t.playBtn->SetExtents({200.0f, 300.0f, 80.0f, 80.0f});

  t.volumeSlider = std::make_shared<TestAccessible>("Volume", Accessibility::Role::SLIDER);
  t.volumeSlider->SetStates(makeStates(true));
  t.volumeSlider->SetExtents({40.0f, 420.0f, 400.0f, 40.0f});

  t.nowPlayingLabel = std::make_shared<TestAccessible>("Now Playing: Bohemian Rhapsody", Accessibility::Role::LABEL);
  t.nowPlayingLabel->SetStates(makeStates());
  t.nowPlayingLabel->SetExtents({40.0f, 480.0f, 400.0f, 30.0f});

  // Footer panel
  t.footer = std::make_shared<TestAccessible>("Footer", Accessibility::Role::PANEL);
  t.footer->SetStates(makeStates());
  t.footer->SetExtents({0.0f, 740.0f, 480.0f, 60.0f});

  t.prevBtn = std::make_shared<TestAccessible>("Previous", Accessibility::Role::PUSH_BUTTON);
  t.prevBtn->SetStates(makeStates(true));
  t.prevBtn->SetExtents({100.0f, 750.0f, 80.0f, 40.0f});

  t.nextBtn = std::make_shared<TestAccessible>("Next", Accessibility::Role::PUSH_BUTTON);
  t.nextBtn->SetStates(makeStates(true));
  t.nextBtn->SetExtents({300.0f, 750.0f, 80.0f, 40.0f});

  // Build tree
  t.header->AddChild(t.menuBtn);
  t.header->AddChild(t.titleLabel);
  t.content->AddChild(t.playBtn);
  t.content->AddChild(t.volumeSlider);
  t.content->AddChild(t.nowPlayingLabel);
  t.footer->AddChild(t.prevBtn);
  t.footer->AddChild(t.nextBtn);
  t.window->AddChild(t.header);
  t.window->AddChild(t.content);
  t.window->AddChild(t.footer);

  t.all = {t.window, t.header, t.menuBtn, t.titleLabel, t.content,
           t.playBtn, t.volumeSlider, t.nowPlayingLabel, t.footer,
           t.prevBtn, t.nextBtn};

  return t;
}

void PrintHelp()
{
  printf("\n");
  printf("Accessibility Inspector Commands:\n");
  printf("  p  - Print accessibility tree\n");
  printf("  n  - Navigate to next element\n");
  printf("  b  - Navigate to previous element\n");
  printf("  c  - Navigate to first child\n");
  printf("  u  - Navigate to parent\n");
  printf("  r  - Read current element (name, role, states, bounds)\n");
  printf("  s  - Speak current element name (TTS)\n");
  printf("  h  - Show this help\n");
  printf("  q  - Quit\n");
  printf("\n");
}

} // anonymous namespace

int main(int argc, char** argv)
{
  printf("=== DALi Accessibility Inspector ===\n\n");

  // Step 1: Install MockDBusWrapper
  auto mockWrapper = std::make_unique<MockDBusWrapper>();
  DBusWrapper::Install(std::move(mockWrapper));

  // Step 2: Set PlatformCallbacks
  Accessibility::PlatformCallbacks callbacks;
  callbacks.addIdle = [](std::function<bool()> cb) -> uint32_t {
    if(cb) cb();
    return 1;
  };
  callbacks.removeIdle           = [](uint32_t) {};
  callbacks.getToolkitVersion    = []() -> std::string { return "inspector-1.0.0"; };
  callbacks.getAppName           = []() -> std::string { return "MyDaliApp"; };
  callbacks.isAdaptorAvailable   = []() -> bool { return true; };
  callbacks.onEnableAutoInit     = []() {};
  callbacks.createTimer          = [](uint32_t, std::function<bool()> cb) -> uint32_t {
    if(cb) cb();
    return 1;
  };
  callbacks.cancelTimer          = [](uint32_t) {};
  callbacks.isTimerRunning       = [](uint32_t) -> bool { return false; };
  Accessibility::SetPlatformCallbacks(callbacks);

  // Step 3: Build demo accessible tree
  auto demo = BuildDemoTree();
  printf("Demo tree built: %zu elements\n", demo.all.size());

  // Step 4: Get bridge and configure
  auto bridge = Accessibility::Bridge::GetCurrentBridge();
  if(!bridge)
  {
    fprintf(stderr, "FATAL: Bridge is null.\n");
    return 1;
  }

  bridge->SetApplicationName("MyDaliApp");
  bridge->SetToolkitName("dali");

  // Register all test accessibles with the bridge
  for(auto& acc : demo.all)
  {
    bridge->AddAccessible(acc->GetId(), acc);
  }

  bridge->AddTopLevelWindow(demo.window.get());

  // Step 5: Initialize and force up the bridge
  bridge->Initialize();
  bridge->ApplicationResumed();

  if(!bridge->IsUp())
  {
    fprintf(stderr, "FATAL: Bridge failed to start.\n");
    return 1;
  }

  std::string busName = bridge->GetBusName();
  auto conn = DBusWrapper::Installed()->eldbus_address_connection_get_impl("unix:path=/tmp/mock-atspi");

  printf("Bridge is up. Bus: %s\n", busName.c_str());

  // Start with the first focusable element (Menu button)
  uint32_t focusedId = demo.menuBtn->GetId();

  // Print initial tree
  printf("\nAccessibility Tree:\n");
  PrintTree(busName, demo.window->GetId(), conn, 0, focusedId);

  PrintHelp();

  // Step 6: Interactive loop
  printf("Focus: [%s] \"%s\"\n", "PUSH_BUTTON", "Menu");
  printf("> ");
  fflush(stdout);

  std::string line;
  while(std::getline(std::cin, line))
  {
    if(line.empty())
    {
      printf("> ");
      fflush(stdout);
      continue;
    }

    char cmd = line[0];

    switch(cmd)
    {
      case 'p':
      {
        printf("\nAccessibility Tree:\n");
        PrintTree(busName, demo.window->GetId(), conn, 0, focusedId);
        break;
      }
      case 'n':
      {
        uint32_t newId = Navigate(busName, focusedId, demo.window->GetId(), conn, true);
        if(newId != focusedId)
        {
          focusedId = newId;
          printf("Navigated forward.\n");
          ReadElement(busName, focusedId, conn);
        }
        else
        {
          printf("(No next element)\n");
        }
        break;
      }
      case 'b':
      {
        uint32_t newId = Navigate(busName, focusedId, demo.window->GetId(), conn, false);
        if(newId != focusedId)
        {
          focusedId = newId;
          printf("Navigated backward.\n");
          ReadElement(busName, focusedId, conn);
        }
        else
        {
          printf("(No previous element)\n");
        }
        break;
      }
      case 'c':
      {
        uint32_t newId = NavigateChild(busName, focusedId, conn);
        if(newId != focusedId)
        {
          focusedId = newId;
          printf("Navigated to child.\n");
          ReadElement(busName, focusedId, conn);
        }
        else
        {
          printf("(No children)\n");
        }
        break;
      }
      case 'u':
      {
        uint32_t newId = NavigateParent(busName, focusedId, conn);
        if(newId != focusedId)
        {
          focusedId = newId;
          printf("Navigated to parent.\n");
          ReadElement(busName, focusedId, conn);
        }
        else
        {
          printf("(Already at root)\n");
        }
        break;
      }
      case 'r':
      {
        ReadElement(busName, focusedId, conn);
        break;
      }
      case 's':
      {
        auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
        auto client = CreateClient(busName, focusedId, accIface, conn);

        std::string name;
        auto nameResult = client.property<std::string>("Name").get();
        if(nameResult) name = std::get<0>(nameResult.getValues());

        std::string roleName;
        auto roleResult = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
        if(roleResult) roleName = RoleToString(static_cast<Accessibility::Role>(std::get<0>(roleResult.getValues())));

        std::string speech = roleName + ". " + name;
        printf("Speaking: \"%s\"\n", speech.c_str());
        Speak(speech);
        break;
      }
      case 'h':
      {
        PrintHelp();
        break;
      }
      case 'q':
      {
        printf("Goodbye.\n");
        bridge->Terminate();
        return 0;
      }
      default:
      {
        printf("Unknown command '%c'. Press 'h' for help.\n", cmd);
        break;
      }
    }

    printf("> ");
    fflush(stdout);
  }

  bridge->Terminate();
  return 0;
}
