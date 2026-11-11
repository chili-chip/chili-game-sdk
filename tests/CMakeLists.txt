add_library(chili-fake-api OBJECT fake_api.cpp)
target_include_directories(chili-fake-api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chili-fake-api PUBLIC chili-game-sdk)

add_library(chili-test-support OBJECT main.cpp)
target_link_libraries(chili-test-support PUBLIC chili-fake-api)

foreach(name input menu screens save scores scene audio fx dialogue)
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_${name}.cpp)
    continue()
  endif()
  add_executable(test_${name} test_${name}.cpp)
  target_link_libraries(test_${name} PRIVATE chili-test-support chili-fake-api)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

# RPG tests also see the RPG sources, for the parts that are not public.
foreach(name rpg_json rpg_load rpg_events rpg_map rpg_turns rpg_stats rpg_combat rpg_ai rpg_items rpg_dialogue rpg_quests rpg_save rpg_demo)
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_${name}.cpp)
    continue()
  endif()
  add_executable(test_${name} test_${name}.cpp)
  target_include_directories(test_${name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(test_${name} PRIVATE chili-test-support chili-fake-api chili-game-sdk-rpg)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

target_compile_definitions(test_rpg_demo PRIVATE CHILI_DEMO_RPG="${PROJECT_SOURCE_DIR}/examples/rpg/assets/pepper.rpg")
