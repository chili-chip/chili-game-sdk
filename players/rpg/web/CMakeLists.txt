# The RPG web player. Build with Emscripten:
#   emcmake cmake -S players/rpg/web -B build-web && cmake --build build-web
#   node players/rpg/web/test.mjs build-web
cmake_minimum_required(VERSION 3.20)
project(chili-rpg-web LANGUAGES C CXX)

# The SDK as a library, with the 32blit engine (no SDL host is linked).
add_subdirectory(../../.. ${CMAKE_BINARY_DIR}/sdk)

add_executable(chili-rpg main.cpp)
target_link_libraries(chili-rpg PRIVATE chili-game-sdk-rpg)
target_compile_options(chili-rpg PRIVATE -O2)
target_link_options(chili-rpg PRIVATE
  -O2
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createChiliRpg
  -sENVIRONMENT=web,worker,node
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_FUNCTIONS=_rpg_load,_rpg_error,_rpg_step,_rpg_restart,_rpg_rgba,_rpg_audio,_rpg_debug,_malloc,_free
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,HEAPU8,HEAPF32
)
set_target_properties(chili-rpg PROPERTIES SUFFIX ".js")

# Ship the player wrapper, a demo page and the template game next to the build.
configure_file(player.js ${CMAKE_BINARY_DIR}/player.js COPYONLY)
configure_file(index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
configure_file(../../../examples/rpg/assets/pepper-village.rpg ${CMAKE_BINARY_DIR}/pepper-village.rpg COPYONLY)
