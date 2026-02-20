# Handover: EspeakTtsEngine + Ubuntu DALi Build

## 이번 세션에서 한 일

### 1. EspeakTtsEngine 구현 (Ubuntu/Linux TTS backend)

Screen reader demos가 macOS의 `MacTtsEngine`만 지원하던 것을 Ubuntu/Linux에서도 동작하도록 `EspeakTtsEngine` 추가.

**새 파일:**
- `tools/screen-reader/espeak-tts-engine.h` — `Accessibility::TtsEngine` 구현
- `tools/screen-reader/espeak-tts-engine.cpp` — `espeak-ng/speak_lib.h` 사용 (`AUDIO_OUTPUT_PLAYBACK` 모드, pause/resume 미지원)

**수정 파일:**
- `build/tizen/CMakeLists.txt` — `IF(APPLE) ... ELSE()` 블록 추가, espeak-ng 소스/링크 설정
- `tools/screen-reader/screen-reader-demo.cpp` — `#ifdef __APPLE__` 분기로 `PlatformTtsEngine` typedef
- `tools/screen-reader/screen-reader-tv-demo.cpp` — 동일한 platform-conditional 처리

### 2. ProgressBar 제거 대응

dali-toolkit upstream에서 `ProgressBar` 클래스가 제거됨. 양쪽 demo에서 `ProgressBar`를 `TextLabel` ("Volume: 50%")로 교체. Up/Down 키로 값 조절 기능 유지.

### 3. CheckBox 활성화 버그 수정 (Linux)

`screen-reader-demo.cpp`에서 Enter 키 누르면 `ActivateCurrentNode()` + `ONE_FINGER_DOUBLE_TAP` gesture가 동시에 발생하여 checkbox가 double-toggle (net zero change) 되는 버그.

- **원인**: macOS에서는 `DoAction("activate")`가 `ClickedSignal`을 발생시키지 않아 manual toggle이 필요했으나, Linux에서는 `DoAction`이 정상 동작하여 이중 토글 발생
- **수정**: `ActivateCurrentNode()` 호출을 `#ifdef __APPLE__`로 가드

### 4. Ubuntu DALi 스택 빌드 성공

| Component | Profile/Option | Install Prefix |
|-----------|---------------|----------------|
| dali-core | default | `~/tizen/dali-env` |
| accessibility-common | `ENABLE_ACCESSIBILITY=ON` | `~/tizen/dali-env` |
| dali-adaptor | `GLIB_X11` (EFL 불필요) | `~/tizen/dali-env` |
| dali-toolkit | default | `~/tizen/dali-env` |

**참고:** Ubuntu에서는 `UBUNTU` profile 대신 `GLIB_X11` profile 사용 (UBUNTU profile은 EFL/Ecore 필요).

### 5. dali-toolkit rebase 충돌 해결

`youngsus/250220-phase5-accessibility-common` 브랜치를 `devel/master`로 rebase 시 충돌 해결:
- `utc-Dali-Accessibility-Controls.cpp` — `ToolBar`, `Alignment` 테스트 함수 유지 (upstream에서 삭제된 것을 `ActorAccessible::Get` API로 추가한 커밋)
- `utc-Dali-Accessibility-Value.cpp` — `ProgressBar` Value 테스트 함수 유지

## Ubuntu 빌드 필요 패키지

```bash
sudo apt-get install -y cmake g++ pkg-config libssl-dev libespeak-ng-dev \
  libfreetype-dev libfontconfig-dev libharfbuzz-dev libpng-dev \
  libcurl4-openssl-dev libfribidi-dev libcairo2-dev libgif-dev \
  libturbojpeg0-dev libjpeg-dev libhyphen-dev libexif-dev \
  libdrm-dev libwebp-dev libx11-dev libxdamage-dev libxfixes-dev \
  libxi-dev libxrender-dev libgles-dev libegl-dev libx11-xcb-dev \
  libxrandr-dev libxcursor-dev libxcomposite-dev libxtst-dev
```

## Ubuntu 빌드 명령 (순서대로)

```bash
export DESKTOP_PREFIX=$HOME/tizen/dali-env
export PKG_CONFIG_PATH=$DESKTOP_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH

# dali-core
cd ~/tizen/dali-core/build/tizen
cmake -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DINSTALL_CMAKE_MODULES=ON . && make install -j$(nproc)

# accessibility-common
cd ~/tizen/accessibility-common/build/tizen && mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DENABLE_ACCESSIBILITY=ON \
  -DINCLUDE_DIR=$DESKTOP_PREFIX/include -DLIB_DIR=$DESKTOP_PREFIX/lib && make install -j$(nproc)
# symlink for pkg-config (adaptor expects dali2-accessibility-common)
ln -sf accessibility-common.pc $DESKTOP_PREFIX/lib/pkgconfig/dali2-accessibility-common.pc

# dali-adaptor (GLIB_X11 profile, no EFL needed)
cd ~/tizen/dali-adaptor/build/tizen
cmake -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DINSTALL_CMAKE_MODULES=ON \
  -DENABLE_PROFILE=GLIB_X11 -DPROFILE_LCASE=glib-x11 -DENABLE_LINK_TEST=OFF . && make install -j$(nproc)

# dali-toolkit
cd ~/tizen/dali-toolkit/build/tizen
cmake -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DINSTALL_CMAKE_MODULES=ON \
  -DENABLE_LINK_TEST=OFF . && make install -j$(nproc)

# Screen reader demos
cd ~/tizen/accessibility-common/build/tizen/build
cmake .. -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DENABLE_ACCESSIBILITY=ON \
  -DINCLUDE_DIR=$DESKTOP_PREFIX/include -DLIB_DIR=$DESKTOP_PREFIX/lib \
  -DBUILD_SCREEN_READER_DEMO=ON -DBUILD_SCREEN_READER_TV_DEMO=ON && make -j$(nproc)

# Run
export LD_LIBRARY_PATH=$DESKTOP_PREFIX/lib
./accessibility-screen-reader-demo
./accessibility-screen-reader-tv-demo
```

## 다음 세션 TODO

- dali-adaptor의 `deps-check.cmake`에서 `dali2-accessibility-common` 대신 `accessibility-common`으로 참조하도록 변경 (pkg-config 이름 통일)
- macOS에서 `DoAction("activate")`가 `ClickedSignal`을 발생시키지 않는 근본 원인 조사
- Ubuntu setup guide (`docs/ubuntu-setup.md`) 업데이트: 실제 빌드 경험 반영
