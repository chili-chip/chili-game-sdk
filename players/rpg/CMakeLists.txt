# The RPG player app: a handler .blit that plays any .rpg file from the
# console's drive. With no file, it plays Pepper Village.

# The template game, built into the app (and into its test, see tests/).
add_library(chili-rpg-player-assets OBJECT)
blit_assets_yaml(chili-rpg-player-assets assets.yml)
target_include_directories(chili-rpg-player-assets PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

blit_executable(chili-rpg main.cpp)
target_link_libraries(chili-rpg chili-game-sdk-rpg chili-rpg-player-assets)
blit_metadata(chili-rpg metadata.yml)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set_target_properties(chili-rpg PROPERTIES SUFFIX ".html")
endif()
