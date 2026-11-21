# PS Vita (VitaSDK). Genera bolitas_edu.vpk:  cmake -S platform/vita -B build-vita && cmake --build build-vita
cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
  else()
    message(FATAL_ERROR "Define VITASDK (o usa la imagen vitasdk/vitasdk)")
  endif()
endif()

project(bolitas_edu C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Bolitas Edu")
set(VITA_TITLEID  "BOLI00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O2 -std=gnu99 -Wall")

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${ROOT}/core ${ROOT}/platform/pc)

add_executable(${PROJECT_NAME}
  ${ROOT}/platform/pc/main.c
  ${ROOT}/platform/pc/gfx.c
  ${ROOT}/platform/pc/audio.c
  ${ROOT}/platform/pc/font_data.c
  ${ROOT}/core/edu.c
  ${ROOT}/core/rhythm.c
  ${ROOT}/core/paint.c
  ${ROOT}/core/hang.c
)

# SDL2 de VitaSDK: si tiene configuración CMake se usa; si no, se enlaza a mano
find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND AND TARGET SDL2::SDL2-static)
  set(SDL_LINK SDL2::SDL2-static)
else()
  set(SDL_LINK SDL2)
endif()

target_link_libraries(${PROJECT_NAME}
  ${SDL_LINK}
  SceDisplay_stub SceCtrl_stub SceAudio_stub SceAudioIn_stub SceSysmodule_stub
  SceGxm_stub SceCommonDialog_stub SceAppMgr_stub SceTouch_stub SceHid_stub
  SceMotion_stub SceIofilemgr_stub SceKernelModulemgr_stub
  m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg0.png sce_sys/livearea/contents/bg0.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
