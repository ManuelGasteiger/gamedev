cmake_minimum_required(VERSION 3.20)
project(hoard_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(ages STATIC
  sim/ages/ages.cpp
  sim/ages/acts.cpp
  sim/ages/chronicle.cpp
  sim/ages/data.cpp
  sim/ages/data_io.cpp
  sim/ages/forecast.cpp
  sim/ages/hoard.cpp
  sim/ages/importance.cpp
  sim/ages/reaction_lines.cpp
  sim/ages/hunger.cpp
  sim/ages/names.cpp
  sim/ages/oaths.cpp
  sim/ages/offers.cpp
  sim/ages/briefing.cpp
  sim/ages/card_data.cpp
  sim/ages/policy.cpp
  sim/ages/pop.cpp
  sim/ages/serialise.cpp
  sim/ages/sleep.cpp
  sim/ages/spells.cpp
  sim/ages/step.cpp
  sim/ages/rules/modifiers.cpp
  sim/ages/rules/wealth.cpp
  sim/ages/rules/cohesion.cpp
  sim/ages/rules/era.cpp
  sim/ages/rules/war.cpp
  sim/ages/rules/coalitions.cpp
  sim/ages/rules/vassals.cpp
  sim/ages/rules/collapse.cpp
  sim/ages/rules/memory.cpp
  sim/ages/rules/dragon_rolls.cpp
  sim/ages/rules/fading.cpp
  sim/ages/rules/migration.cpp
  sim/ages/rules/remnants.cpp
  sim/ages/rules/regimes.cpp
  sim/ages/rules/hegemony.cpp
  sim/ages/rules/land.cpp
  sim/ages/rules/oaths.cpp
  sim/ages/rules/shocks.cpp
  sim/ages/rules/cards.cpp
  sim/ages/rules/card_match.cpp
  sim/ages/rules/card_effects.cpp
  sim/ages/rules/people.cpp
  sim/ages/rules/people_rise.cpp
  sim/ages/rules/hostages.cpp
  sim/ages/rules/threads.cpp
  sim/ages/rules/chronicle.cpp
  # v0.4 (S1)
  sim/ages/defences.cpp
  sim/ages/dragon_acts.cpp
  sim/ages/dragon_data.cpp
  sim/ages/dragon_json.cpp
  sim/ages/growth.cpp
  sim/ages/talents.cpp
  sim/ages/wounds.cpp
  sim/ages/rules/anti_dragon.cpp
  sim/ages/rules/slayers.cpp
  # v0.4 hooks (S3)
  sim/ages/hook_data.cpp
  sim/ages/hook_json.cpp
  sim/ages/hook_policy.cpp
  sim/ages/hook_resolve.cpp
  sim/ages/hooks.cpp
  # v0.4 world (S2)
  sim/ages/genesis_data.cpp
  sim/ages/genesis_map.cpp
  sim/ages/genesis_start.cpp
  sim/ages/genesis_json.cpp
  sim/ages/genesis_world.cpp
  sim/ages/legends.cpp
  sim/ages/rules/world.cpp
  sim/ages/rules/undying.cpp
  sim/ages/rules/extinction.cpp
  sim/ages/rules/magic.cpp
  sim/ages/rules/card_world.cpp
  sim/ages/rules/storyteller.cpp
)
target_include_directories(ages PUBLIC sim)
if(MSVC)
  target_compile_options(ages PRIVATE /W4 /bigobj)
else()
  target_compile_options(ages PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ages_runner sim/runner/main.cpp)
target_link_libraries(ages_runner PRIVATE ages)
set_target_properties(ages_runner PROPERTIES OUTPUT_NAME ages)

enable_testing()
set(AGES_DATA_DIR ${CMAKE_SOURCE_DIR}/data)
set(AGES_GOLDEN_DIR ${CMAKE_SOURCE_DIR}/sim/tests/golden)

function(ages_test name label)
  add_executable(test_${name} sim/tests/${name}.cpp)
  target_link_libraries(test_${name} PRIVATE ages)
  target_compile_definitions(test_${name} PRIVATE
    AGES_DATA_DIR="${AGES_DATA_DIR}"
    AGES_GOLDEN_DIR="${AGES_GOLDEN_DIR}"
    AGES_BINARY="$<TARGET_FILE:ages_runner>")
  add_test(NAME ${name} COMMAND test_${name})
  set_tests_properties(${name} PROPERTIES LABELS ${label})
endfunction()

ages_test(determinism fast)
ages_test(baseline slow)
ages_test(reactions fast)
ages_test(oath_breach_sleep fast)
ages_test(policies slow)
ages_test(golden fast)
