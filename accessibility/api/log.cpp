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
#include <accessibility/api/log.h>

namespace
{
Accessibility::LogFunction gLogFunction;

void DefaultLogFunction(Accessibility::LogLevel level, const char* format, va_list args)
{
  const char* prefix = "";
  switch(level)
  {
    case Accessibility::LogLevel::DEBUG:
      prefix = "DEBUG";
      break;
    case Accessibility::LogLevel::INFO:
      prefix = "INFO";
      break;
    case Accessibility::LogLevel::WARNING:
      prefix = "WARNING";
      break;
    case Accessibility::LogLevel::ERROR:
      prefix = "ERROR";
      break;
  }
  fprintf(stderr, "ACCESSIBILITY %s: ", prefix);
  vfprintf(stderr, format, args);
}
} // namespace

namespace Accessibility
{
void SetLogFunction(LogFunction func)
{
  gLogFunction = std::move(func);
}

void LogMessage(LogLevel level, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  if(gLogFunction)
  {
    gLogFunction(level, format, args);
  }
  else
  {
    DefaultLogFunction(level, format, args);
  }
  va_end(args);
}

} // namespace Accessibility
