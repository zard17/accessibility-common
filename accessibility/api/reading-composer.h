#ifndef ACCESSIBILITY_API_READING_COMPOSER_H
#define ACCESSIBILITY_API_READING_COMPOSER_H

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

// INTERNAL INCLUDES
#include <accessibility/api/node-proxy.h>

namespace Accessibility
{
/**
 * @brief Configuration for the reading composer.
 *
 * Different profiles (mobile, TV, wearable) may use different settings.
 */
struct ReadingComposerConfig
{
  bool suppressTouchHints = false; ///< TV: true — suppress "double tap to activate" hints
  bool includeTvTraits    = false; ///< TV: true — include TV-specific role/state traits
};

/**
 * @brief Composes human-readable TTS strings from ReadingMaterial.
 *
 * Assembles the spoken output from a node's reading material by combining
 * the name, role trait, state trait, and description into a single string
 * suitable for TTS output.
 */
class ReadingComposer
{
public:
  /**
   * @brief Constructor.
   *
   * @param[in] config Configuration for the composer
   */
  explicit ReadingComposer(ReadingComposerConfig config = ReadingComposerConfig{});

  /**
   * @brief Composes the full TTS string from reading material.
   *
   * @param[in] rm The reading material to compose
   * @return The composed TTS string
   */
  std::string compose(const ReadingMaterial& rm) const;

  /**
   * @brief Composes the role trait portion of the reading.
   *
   * @param[in] rm The reading material
   * @return The role trait string (e.g. "button", "slider")
   */
  std::string composeRoleTrait(const ReadingMaterial& rm) const;

  /**
   * @brief Composes the state trait portion of the reading.
   *
   * @param[in] rm The reading material
   * @return The state trait string (e.g. "selected", "disabled")
   */
  std::string composeStateTrait(const ReadingMaterial& rm) const;

  /**
   * @brief Composes the description trait portion of the reading.
   *
   * @param[in] rm The reading material
   * @return The description trait string
   */
  std::string composeDescriptionTrait(const ReadingMaterial& rm) const;

private:
  ReadingComposerConfig mConfig;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_READING_COMPOSER_H
