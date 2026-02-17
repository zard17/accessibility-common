#ifndef ACCESSIBILITY_LOG_H
#define ACCESSIBILITY_LOG_H

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
#include <cstdarg>
#include <cstdio>
#include <functional>

// INTERNAL INCLUDES
#include <accessibility/public-api/accessibility-common.h>

namespace Accessibility
{
/**
 * @brief Log level enumeration.
 */
enum class LogLevel
{
  DEBUG,
  INFO,
  WARNING,
  ERROR
};

/**
 * @brief Log callback function type.
 *
 * @param[in] level The log level
 * @param[in] format printf-style format string
 * @param[in] args Variable argument list
 */
using LogFunction = std::function<void(LogLevel level, const char* format, va_list args)>;

/**
 * @brief Sets the log callback function.
 *
 * If not set, a default implementation using fprintf(stderr, ...) is used.
 *
 * @param[in] func The log callback
 */
ACCESSIBILITY_API void SetLogFunction(LogFunction func);

/**
 * @brief Logs a message at the given level.
 *
 * @param[in] level The log level
 * @param[in] format printf-style format string
 */
ACCESSIBILITY_API void LogMessage(LogLevel level, const char* format, ...) __attribute__((format(printf, 2, 3)));

} // namespace Accessibility

// Convenience macros
#define ACCESSIBILITY_LOG_ERROR(format, ...) \
  Accessibility::LogMessage(Accessibility::LogLevel::ERROR, format, ##__VA_ARGS__)

#define ACCESSIBILITY_LOG_INFO(format, ...) \
  Accessibility::LogMessage(Accessibility::LogLevel::INFO, format, ##__VA_ARGS__)

#define ACCESSIBILITY_LOG_DEBUG_INFO(format, ...) \
  Accessibility::LogMessage(Accessibility::LogLevel::DEBUG, format, ##__VA_ARGS__)

#endif // ACCESSIBILITY_LOG_H
