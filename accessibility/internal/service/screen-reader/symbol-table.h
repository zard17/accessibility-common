#ifndef ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_SYMBOL_TABLE_H
#define ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_SYMBOL_TABLE_H

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
#include <string>

namespace Accessibility
{
/**
 * @brief Provides symbol-to-spoken-text mappings for TTS.
 *
 * Maps punctuation and special characters to their spoken equivalents
 * (e.g., "." → "dot", "@" → "at sign"). Pure logic, no platform dependency.
 */
class SymbolTable
{
public:
  /**
   * @brief Looks up the spoken form of a symbol.
   *
   * @param[in] symbol The symbol character(s) to look up
   * @return The spoken text, or empty string if not found
   */
  static const std::string& lookup(const std::string& symbol);
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_SCREEN_READER_SYMBOL_TABLE_H
