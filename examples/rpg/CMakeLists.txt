# Pepper Village: a small RPG that uses every RPG part of the game SDK.

# The sprite sheet and the game file, built into the game (and into the
# screenshot tool, see tools/).
add_library(chili-rpg-assets OBJECT)
blit_assets_yaml(chili-rpg-assets assets.yml)
target_include_directories(chili-rpg-assets PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

blit_executable(chili-rpg-demo rpg.cpp)
target_link_libraries(chili-rpg-demo chili-game-sdk-rpg chili-rpg-assets)
blit_metadata(chili-rpg-demo metadata.yml)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set_target_properties(chili-rpg-demo PROPERTIES SUFFIX ".html")
endif()
