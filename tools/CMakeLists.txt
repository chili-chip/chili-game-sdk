# Plays the demo game with scripted input and saves screenshots for the docs.
# Run tools/screenshots.py rather than this directly.
add_executable(chili-shots shots.cpp)
target_link_libraries(chili-shots PRIVATE chili-fake-api)
