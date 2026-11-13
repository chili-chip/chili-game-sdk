cmake_minimum_required(VERSION 3.15...3.31)

project(chili-game-sdk VERSION 0.1.0 LANGUAGES C CXX ASM)

# A game adds the 32blit SDK first, so BlitEngine already exists. Built on its
# own (for the tests and the demo), find the SDK the same way games do.
if(NOT TARGET BlitEngine)
  find_package(32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})
endif()

add_library(chili-game-sdk STATIC
  src/audio.cpp
  src/dialogue.cpp
  src/fx.cpp
  src/input.cpp
  src/menu.cpp
  src/scene.cpp
  src/screens.cpp
  src/ui.cpp
)
add_library(chili::game-sdk ALIAS chili-game-sdk)

target_include_directories(chili-game-sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(chili-game-sdk PUBLIC BlitEngine)

# The RPG parts: game files, maps, turns, combat, inventory, dialogue and
# quests. A separate target, so arcade games do not carry it.
add_library(chili-game-sdk-rpg STATIC
  src/rpg/json.cpp
  src/rpg/load.cpp
  src/rpg/world.cpp
  src/rpg/combat.cpp
  src/rpg/ai.cpp
  src/rpg/items.cpp
  src/rpg/save.cpp
)
add_library(chili::game-sdk-rpg ALIAS chili-game-sdk-rpg)

target_include_directories(chili-game-sdk-rpg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(chili-game-sdk-rpg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(chili-game-sdk-rpg PUBLIC chili-game-sdk)

if(NOT MSVC)
  target_compile_options(chili-game-sdk PRIVATE -Wall -Wextra -Wdouble-promotion -Wno-unused-parameter)
  target_compile_options(chili-game-sdk-rpg PRIVATE -Wall -Wextra -Wdouble-promotion -Wno-unused-parameter)
endif()

# Tests and the demo only build when this repo is the top-level project.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CHILI_DESKTOP OFF)
  if(NOT 32BLIT_HW AND NOT PICO_SDK_PATH AND NOT EMSCRIPTEN)
    set(CHILI_DESKTOP ON)
  endif()

  option(CHILI_GAME_SDK_TESTS "Build the unit tests" ${CHILI_DESKTOP})
  option(CHILI_GAME_SDK_DEMO "Build the demo game" ON)

  if(CHILI_GAME_SDK_TESTS)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(tools)
  endif()

  if(CHILI_GAME_SDK_DEMO)
    add_subdirectory(examples/demo)
    add_subdirectory(examples/rpg)
  endif()
endif()
