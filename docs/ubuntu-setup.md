# Ubuntu Build & Demo Guide

accessibility-common + DALi demos on Ubuntu.

## 1. Prerequisites

```bash
# DALi build deps (if not already installed)
sudo apt install cmake g++ pkg-config \
  libglib2.0-dev libgles2-mesa-dev libpng-dev libjpeg-dev \
  libfreetype-dev libharfbuzz-dev libfontconfig-dev libcurl4-openssl-dev \
  libxi-dev libxfixes-dev libxdamage-dev libxrandr-dev libxtst-dev \
  libxkbcommon-dev libwayland-dev wayland-protocols

# GDBus (already part of libglib2.0-dev)
# espeak-ng for TTS
sudo apt install libespeak-ng-dev espeak-ng
```

## 2. DALi Build

DALi 패키지가 이미 빌드되어 있다고 가정 (`~/tizen/dali-env/`).
안 되어 있으면 `dali-core → dali-adaptor → dali-toolkit` 순서로 빌드.

## 3. accessibility-common Build

```bash
cd ~/tizen/accessibility-common/build/tizen
mkdir -p build && cd build

# Tests + demos
cmake .. \
  -DENABLE_ATSPI=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_SERVICE_TESTS=ON \
  -DBUILD_SCREEN_READER_TESTS=ON \
  -DBUILD_SCREEN_READER_DEMO=ON \
  -DBUILD_SCREEN_READER_TV_DEMO=ON \
  -DBUILD_WEB_INSPECTOR_DIRECT=ON \
  -DENABLE_PKG_CONFIGURE=OFF

make -j$(nproc)
```

## 4. TTS: MacTtsEngine → EspeakTtsEngine

데모 소스에서 `MacTtsEngine` 대신 `EspeakTtsEngine`을 사용해야 함.

### 4.1 새 파일: `tools/screen-reader/espeak-tts-engine.h`

```cpp
#ifndef ACCESSIBILITY_TOOLS_SCREEN_READER_ESPEAK_TTS_ENGINE_H
#define ACCESSIBILITY_TOOLS_SCREEN_READER_ESPEAK_TTS_ENGINE_H

#include <accessibility/api/tts-engine.h>

class EspeakTtsEngine : public Accessibility::TtsEngine
{
public:
  EspeakTtsEngine();
  ~EspeakTtsEngine() override;

  Accessibility::CommandId speak(const std::string& text, const Accessibility::SpeakOptions& options) override;
  void stop() override;
  bool pause() override;
  bool resume() override;
  bool isPaused() const override;
  void purge(bool onlyDiscardable) override;
  void onUtteranceStarted(std::function<void(Accessibility::CommandId)> callback) override;
  void onUtteranceCompleted(std::function<void(Accessibility::CommandId)> callback) override;

private:
  Accessibility::CommandId mNextId{1};
  Accessibility::CommandId mCurrentId{0};
  bool                     mPaused{false};
  std::function<void(Accessibility::CommandId)> mStartedCallback;
  std::function<void(Accessibility::CommandId)> mCompletedCallback;
};

#endif
```

### 4.2 새 파일: `tools/screen-reader/espeak-tts-engine.cpp`

```cpp
#include <tools/screen-reader/espeak-tts-engine.h>
#include <espeak-ng/speak_lib.h>
#include <cstdio>

EspeakTtsEngine::EspeakTtsEngine()
{
  espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, nullptr, 0);
}

EspeakTtsEngine::~EspeakTtsEngine()
{
  espeak_Terminate();
}

Accessibility::CommandId EspeakTtsEngine::speak(const std::string& text, const Accessibility::SpeakOptions& options)
{
  if(options.interrupt)
  {
    espeak_Cancel();
  }

  mCurrentId = mNextId++;
  mPaused    = false;

  auto id = mCurrentId;
  if(mStartedCallback) mStartedCallback(id);

  espeak_Synth(text.c_str(), text.size() + 1, 0, POS_CHARACTER, 0,
               espeakCHARS_UTF8, nullptr, nullptr);

  // Note: espeak_Synth is async with AUDIO_OUTPUT_PLAYBACK.
  // For proper completion callback, use espeak_SetSynthCallback.
  // Simplified here — fires immediately.
  if(mCompletedCallback) mCompletedCallback(id);

  return id;
}

void EspeakTtsEngine::stop()
{
  mPaused = false;
  espeak_Cancel();
}

bool EspeakTtsEngine::pause()  { return false; } // espeak-ng has no pause API
bool EspeakTtsEngine::resume() { return false; }
bool EspeakTtsEngine::isPaused() const { return false; }

void EspeakTtsEngine::purge(bool)
{
  espeak_Cancel();
}

void EspeakTtsEngine::onUtteranceStarted(std::function<void(Accessibility::CommandId)> cb)
{
  mStartedCallback = std::move(cb);
}

void EspeakTtsEngine::onUtteranceCompleted(std::function<void(Accessibility::CommandId)> cb)
{
  mCompletedCallback = std::move(cb);
}
```

### 4.3 CMakeLists.txt 수정

`IF(APPLE)` 블록을 `IF/ELSE`로 변경 (두 군데 — demo, tv-demo):

```cmake
  IF( APPLE )
    TARGET_SOURCES( accessibility-screen-reader-demo PRIVATE
      ${accessibility_common_root}/tools/screen-reader/mac-tts-engine.mm
    )
    TARGET_LINK_LIBRARIES( accessibility-screen-reader-demo
      "-framework AVFoundation" "-framework Foundation" objc
    )
  ELSE()
    TARGET_SOURCES( accessibility-screen-reader-demo PRIVATE
      ${accessibility_common_root}/tools/screen-reader/espeak-tts-engine.cpp
    )
    TARGET_LINK_LIBRARIES( accessibility-screen-reader-demo espeak-ng )
  ENDIF()
```

### 4.4 데모 소스 수정

`screen-reader-demo.cpp`, `screen-reader-tv-demo.cpp` 둘 다:

```cpp
// Before
#include <tools/screen-reader/mac-tts-engine.h>
// ...
auto tts = std::make_unique<MacTtsEngine>();

// After
#ifdef __APPLE__
#include <tools/screen-reader/mac-tts-engine.h>
using PlatformTtsEngine = MacTtsEngine;
#else
#include <tools/screen-reader/espeak-tts-engine.h>
using PlatformTtsEngine = EspeakTtsEngine;
#endif
// ...
auto tts = std::make_unique<PlatformTtsEngine>();
```

## 5. Run

```bash
export LD_LIBRARY_PATH=$HOME/tizen/dali-env/lib

# Tests
./accessibility-test                  # 56 passed
./accessibility-service-test          # 55 passed
./accessibility-screen-reader-test    # 120 passed

# Inspector (no TTS needed)
./accessibility-web-inspector-direct  # http://localhost:8080

# Screen reader demos
./accessibility-screen-reader-demo
./accessibility-screen-reader-tv-demo
```

## 6. GDBus Mode Inspector

Ubuntu는 D-Bus daemon이 기본 실행 중이므로 GDBus inspector도 바로 사용 가능:

```bash
cmake .. -DBUILD_WEB_INSPECTOR_GDBUS=ON ...
make -j$(nproc)
./accessibility-web-inspector-gdbus   # real D-Bus IPC round-trip
```
