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
#include <iostream>
#include <string>

// INTERNAL INCLUDES
#include <tools/inspector/query-engine.h>
#include <tools/inspector/tts.h>

namespace
{
/**
 * @brief Recursively prints a tree node with indentation.
 */
void PrintTree(const InspectorEngine::TreeNode& node, int depth, uint32_t focusedId)
{
  bool isFocused = (node.id == focusedId);
  for(int i = 0; i < depth; ++i) printf("  ");
  printf("%s[%s] \"%s\"", isFocused ? ">> " : "", node.role.c_str(), node.name.c_str());
  if(node.childCount > 0)
  {
    printf(" (%d children)", node.childCount);
  }
  printf("\n");

  for(auto& child : node.children)
  {
    PrintTree(child, depth + 1, focusedId);
  }
}

/**
 * @brief Prints element details to stdout.
 */
void PrintElement(const InspectorEngine::ElementInfo& info)
{
  printf("\n");
  printf("  Name:        %s\n", info.name.c_str());
  printf("  Role:        %s\n", info.role.c_str());
  if(!info.description.empty())
    printf("  Description: %s\n", info.description.c_str());
  printf("  States:      %s\n", info.states.c_str());
  printf("  Bounds:      (%.0f, %.0f, %.0fx%.0f)\n",
         info.boundsX, info.boundsY, info.boundsWidth, info.boundsHeight);
  printf("\n");
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
  printf("=== Tizen Accessibility Inspector ===\n\n");

  InspectorEngine::AccessibilityQueryEngine engine;
  if(!engine.Initialize())
  {
    fprintf(stderr, "Failed to initialize accessibility engine.\n");
    return 1;
  }

  printf("Bridge is up.\n");

  // Print initial tree
  printf("\nAccessibility Tree:\n");
  auto tree = engine.BuildTree(engine.GetRootId());
  PrintTree(tree, 0, engine.GetFocusedId());

  PrintHelp();

  // Print initial focus
  auto focusInfo = engine.GetElementInfo(engine.GetFocusedId());
  printf("Focus: [%s] \"%s\"\n", focusInfo.role.c_str(), focusInfo.name.c_str());
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
        auto t = engine.BuildTree(engine.GetRootId());
        PrintTree(t, 0, engine.GetFocusedId());
        break;
      }
      case 'n':
      {
        uint32_t oldId = engine.GetFocusedId();
        uint32_t newId = engine.Navigate(oldId, true);
        if(newId != oldId)
        {
          engine.SetFocusedId(newId);
          printf("Navigated forward.\n");
          PrintElement(engine.GetElementInfo(newId));
        }
        else
        {
          printf("(No next element)\n");
        }
        break;
      }
      case 'b':
      {
        uint32_t oldId = engine.GetFocusedId();
        uint32_t newId = engine.Navigate(oldId, false);
        if(newId != oldId)
        {
          engine.SetFocusedId(newId);
          printf("Navigated backward.\n");
          PrintElement(engine.GetElementInfo(newId));
        }
        else
        {
          printf("(No previous element)\n");
        }
        break;
      }
      case 'c':
      {
        uint32_t oldId = engine.GetFocusedId();
        uint32_t newId = engine.NavigateChild(oldId);
        if(newId != oldId)
        {
          engine.SetFocusedId(newId);
          printf("Navigated to child.\n");
          PrintElement(engine.GetElementInfo(newId));
        }
        else
        {
          printf("(No children)\n");
        }
        break;
      }
      case 'u':
      {
        uint32_t oldId = engine.GetFocusedId();
        uint32_t newId = engine.NavigateParent(oldId);
        if(newId != oldId)
        {
          engine.SetFocusedId(newId);
          printf("Navigated to parent.\n");
          PrintElement(engine.GetElementInfo(newId));
        }
        else
        {
          printf("(Already at root)\n");
        }
        break;
      }
      case 'r':
      {
        PrintElement(engine.GetElementInfo(engine.GetFocusedId()));
        break;
      }
      case 's':
      {
        auto info = engine.GetElementInfo(engine.GetFocusedId());
        std::string speech = info.role + ". " + info.name;
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
        engine.Shutdown();
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

  engine.Shutdown();
  return 0;
}
