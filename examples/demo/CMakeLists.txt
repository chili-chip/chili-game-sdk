# Chili Catch: a small game that uses every module of the game SDK.
blit_executable(chili-demo demo.cpp)
target_link_libraries(chili-demo chili-game-sdk)
blit_metadata(chili-demo metadata.yml)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set_target_properties(chili-demo PROPERTIES SUFFIX ".html")
endif()
