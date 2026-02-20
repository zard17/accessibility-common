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

// CLASS HEADER
#include <accessibility/internal/service/screen-reader/symbol-table.h>

// EXTERNAL INCLUDES
#include <unordered_map>

namespace Accessibility
{
const std::string& SymbolTable::lookup(const std::string& symbol)
{
  static const std::unordered_map<std::string, std::string> table = {
    {".", "dot"},
    {",", "comma"},
    {"!", "exclamation mark"},
    {"?", "question mark"},
    {"@", "at sign"},
    {"#", "hash"},
    {"$", "dollar sign"},
    {"%", "percent"},
    {"^", "caret"},
    {"&", "ampersand"},
    {"*", "asterisk"},
    {"(", "left parenthesis"},
    {")", "right parenthesis"},
    {"-", "hyphen"},
    {"_", "underscore"},
    {"+", "plus"},
    {"=", "equals"},
    {"{", "left brace"},
    {"}", "right brace"},
    {"[", "left bracket"},
    {"]", "right bracket"},
    {"|", "vertical bar"},
    {"\\", "backslash"},
    {"/", "slash"},
    {":", "colon"},
    {";", "semicolon"},
    {"\"", "quotation mark"},
    {"'", "apostrophe"},
    {"<", "less than"},
    {">", "greater than"},
    {"~", "tilde"},
    {"`", "grave accent"},
    {"\n", "new line"},
    {"\t", "tab"},
    {" ", "space"},
    {"\xC2\xA9", "copyright"},
    {"\xC2\xAE", "registered"},
    {"\xE2\x84\xA2", "trademark"},
    {"\xC2\xB0", "degree"},
    {"\xC2\xA3", "pound sign"},
    {"\xC2\xA5", "yen sign"},
    {"\xE2\x82\xAC", "euro sign"},
    {"\xC2\xA2", "cent sign"},
    {"\xC2\xB1", "plus minus"},
    {"\xC3\x97", "multiplication sign"},
    {"\xC3\xB7", "division sign"},
    {"\xE2\x88\x9E", "infinity"},
    {"\xE2\x89\xA0", "not equal"},
    {"\xE2\x89\xA4", "less than or equal"},
    {"\xE2\x89\xA5", "greater than or equal"},
    {"\xE2\x80\xA6", "ellipsis"},
    {"\xE2\x80\x93", "en dash"},
    {"\xE2\x80\x94", "em dash"},
    {"\xE2\x80\x98", "left single quotation mark"},
    {"\xE2\x80\x99", "right single quotation mark"},
    {"\xE2\x80\x9C", "left double quotation mark"},
    {"\xE2\x80\x9D", "right double quotation mark"},
  };

  static const std::string empty;

  auto it = table.find(symbol);
  if(it != table.end())
  {
    return it->second;
  }
  return empty;
}

} // namespace Accessibility
