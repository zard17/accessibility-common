# Handover: pkg-config 이름 통일

## 이번 세션에서 한 일

### dali-adaptor pkg-config 참조명 변경: `dali2-accessibility-common` → `accessibility-common`

accessibility-common 라이브러리의 `.pc` 파일명은 `accessibility-common.pc`인데, dali-adaptor가 `dali2-accessibility-common`으로 참조하여 수동 symlink가 필요했던 문제 해결.

**수정 파일 (dali-adaptor repo):**

| File | Change |
|------|--------|
| `build/tizen/deps-check.cmake:150` | `CHECK_MODULE_AND_SET( DALI_ACCESSIBILITY accessibility-common [] )` (functional) |
| `build/tizen/dali2-adaptor.pc.in:10` | `Requires: dali2-core accessibility-common` (functional) |
| `build/tizen/CMakeLists.txt` | 4개 comment에서 `dali2-accessibility-common` → `accessibility-common` |
| `dali/internal/accessibility/file.list:64` | 1개 comment 업데이트 |

**검증 완료:**
- `dali2-accessibility-common.pc` symlink 제거 후 dali-adaptor cmake 성공 (`Found accessibility-common, version 0.1.0`)
- dali-adaptor, dali-toolkit 전체 빌드/설치 성공
- screen-reader-demo, screen-reader-tv-demo 빌드 성공
- dali-adaptor 내 `dali2-accessibility-common` 참조 0건 확인

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

# dali-adaptor (GLIB_X11 profile, no EFL needed)
cd ~/tizen/dali-adaptor/build/tizen && rm -f CMakeCache.txt
PKG_CONFIG_PATH=$DESKTOP_PREFIX/lib/pkgconfig cmake \
  -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DINSTALL_CMAKE_MODULES=ON \
  -DENABLE_PROFILE=GLIB_X11 -DPROFILE_LCASE=glib-x11 -DENABLE_LINK_TEST=OFF . && make install -j$(nproc)

# dali-toolkit
cd ~/tizen/dali-toolkit/build/tizen && rm -f CMakeCache.txt
PKG_CONFIG_PATH=$DESKTOP_PREFIX/lib/pkgconfig cmake \
  -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DINSTALL_CMAKE_MODULES=ON \
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

- Web inspector Ubuntu 빌드 지원
- macOS에서 `DoAction("activate")`가 `ClickedSignal`을 발생시키지 않는 근본 원인 조사
