# Plays the demo game with scripted input and saves screenshots for the docs.
# Run tools/screenshots.py rather than this directly.
add_executable(chili-shots shots.cpp)
target_link_libraries(chili-shots PRIVATE chili-fake-api)

# The same for Pepper Village, the RPG demo.
if(CHILI_GAME_SDK_DEMO)
  add_executable(chili-rpg-shots rpg_shots.cpp)
  target_link_libraries(chili-rpg-shots PRIVATE chili-fake-api chili-game-sdk-rpg chili-rpg-assets)
  target_compile_definitions(chili-rpg-shots PRIVATE
    CHILI_TEMPLATE_RPG="${PROJECT_SOURCE_DIR}/examples/rpg/assets/pepper-village.rpg")
endif()

# The same for the RPG player app.
if(CHILI_GAME_SDK_PLAYERS)
  add_executable(chili-player-shots player_shots.cpp)
  target_link_libraries(chili-player-shots PRIVATE chili-fake-api chili-game-sdk-rpg chili-rpg-player-assets)
endif()
