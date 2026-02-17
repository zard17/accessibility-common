#ifndef ACCESSIBILITY_COMMON_H
#define ACCESSIBILITY_COMMON_H

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

/*
 * Definitions for shared library support.
 *
 * If a library is configured with --enable-exportall or --enable-debug
 * then HIDE_ACCESSIBILITY_INTERNALS is not defined, and nothing is hidden.
 * If it is configured without these options (the default), visibility is
 * automatically hidden, and the explicit defines below come into use.
 */
#if __GNUC__ >= 4
#ifndef HIDE_ACCESSIBILITY_INTERNALS
#define ACCESSIBILITY_API
#else
#define ACCESSIBILITY_API __attribute__((visibility("default")))
#endif
#else
#ifdef WIN32
#ifdef BUILDING_ACCESSIBILITY_COMMON
/** Visibility attribute to export declarations */
#define ACCESSIBILITY_API __declspec(dllexport)
#else
/** Visibility attribute to import declarations */
#define ACCESSIBILITY_API __declspec(dllimport)
#endif
#else
/** Visibility attribute to show declarations */
#define ACCESSIBILITY_API
#endif
#endif

// Compiler hint macros
#ifdef __GNUC__
#define ACCESSIBILITY_LIKELY(x)   __builtin_expect(!!(x), 1)
#define ACCESSIBILITY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define ACCESSIBILITY_LIKELY(x)   (x)
#define ACCESSIBILITY_UNLIKELY(x) (x)
#endif

#endif // ACCESSIBILITY_COMMON_H
