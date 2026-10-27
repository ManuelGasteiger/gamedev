cmake_minimum_required(VERSION 3.20)
project(hoard_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# determinism policy (docs/research-determinism-ci.md section 1): no contraction, no fast-math, anywhere in sim/
foreach(v CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_EXE_LINKER_FLAGS)
  if("${${v}}" MATCHES "-ffast-math|-Ofast|/fp:fast")
    message(FATAL_ERROR "${v} contains a fast-math flag; the sim must stay deterministic")
  endif()
endforeach()
function(hoard_sim_flags target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /fp:strict)
  else()
    target_compile_options(${target} PRIVATE -ffp-contract=off -fno-fast-math)
  endif()
endfunction()

add_library(ages STATIC
  sim/core/detmath.cpp
  sim/ages/state_hash.cpp
  sim/ages/record.cpp
  sim/ages/recorded.cpp
  sim/ages/save.cpp
  sim/ages/ages.cpp
  sim/ages/acts.cpp
  sim/ages/chronicle.cpp
  sim/ages/data.cpp
  sim/ages/data_io.cpp
  sim/ages/forecast.cpp
  sim/ages/hoard.cpp
  sim/ages/importance.cpp
  sim/ages/memory_tiers.cpp
  sim/ages/memory_data.cpp
  sim/ages/reaction_lines.cpp
  sim/ages/hunger.cpp
  sim/ages/names.cpp
  sim/ages/oaths.cpp
  sim/ages/offers.cpp
  sim/ages/briefing.cpp
  sim/sift/acceptor.cpp
  sim/sift/bank_data.cpp
  sim/sift/because.cpp
  sim/sift/line_text.cpp
  sim/sift/score.cpp
  sim/sift/select.cpp
  sim/sift/sift_json.cpp
  sim/sift/sift_util.cpp
  sim/sift/source.cpp
  sim/ages/card_data.cpp
  sim/ages/policy.cpp
  sim/ages/policy_json.cpp
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
  # item 06
  sim/ages/arrival_data.cpp
  sim/ages/arrival_gates.cpp
  sim/ages/arrivals.cpp
  sim/ages/arrival_acts.cpp
  sim/ages/rules/arrivals.cpp
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
  # item 19: text
  sim/text/tokens.cpp
  sim/text/lexicon.cpp
  sim/text/names.cpp
  sim/text/numbers.cpp
  sim/text/render.cpp
  sim/text/bundle.cpp
  sim/text/bundle_io.cpp
  sim/text/batch.cpp
  sim/text/reason.cpp
)
target_include_directories(ages PUBLIC sim)
hoard_sim_flags(ages)
execute_process(COMMAND git rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE HOARD_BUILD OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(HOARD_BUILD)
  set_source_files_properties(sim/ages/record.cpp PROPERTIES COMPILE_DEFINITIONS HOARD_BUILD="${HOARD_BUILD}")
endif()
if(MSVC)
  target_compile_options(ages PRIVATE /W4 /bigobj)
else()
  target_compile_options(ages PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ages_runner sim/runner/main.cpp sim/runner/analyse.cpp sim/runner/text_cmd.cpp)
target_link_libraries(ages_runner PRIVATE ages)
target_include_directories(ages_runner PRIVATE sim/runner)
hoard_sim_flags(ages_runner)
set_target_properties(ages_runner PROPERTIES OUTPUT_NAME hoard-sim)

enable_testing()
set(AGES_DATA_DIR ${CMAKE_SOURCE_DIR}/data)
set(AGES_GOLDEN_DIR ${CMAKE_SOURCE_DIR}/tests/golden)

function(ages_test name label)
  add_executable(test_${name} sim/tests/${name}.cpp)
  target_link_libraries(test_${name} PRIVATE ages)
  hoard_sim_flags(test_${name})
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
ages_test(detmath_bounds fast)
ages_test(rng_sequence fast)
ages_test(hash_canonical fast)
ages_test(save_roundtrip fast)
ages_test(replay_verify fast)
ages_test(dread_awe_update fast)
ages_test(memory_tiers fast)
ages_test(arrival_gating fast)
ages_test(arrival_cooldown fast)
ages_test(sift_acceptor fast)
ages_test(sift_budget fast)

# item 19: schema, lint and forced-state renders of the seed scenarios in every language (needs jsonschema)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  file(GLOB HOARD_SEEDS ${CMAKE_SOURCE_DIR}/docs/scenarios/seed-*.json)
  add_test(NAME text_lint_seeds COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/scenario_check.py
    ${CMAKE_SOURCE_DIR}/docs/scenario.schema.json ${CMAKE_SOURCE_DIR}/docs/act-api.schema.json ${CMAKE_SOURCE_DIR}/docs/scenarios/vocab.json
    ${HOARD_SEEDS} --text ${AGES_DATA_DIR} --bin $<TARGET_FILE:ages_runner>)
  set_tests_properties(text_lint_seeds PROPERTIES LABELS fast)
endif()

find_program(BASH bash)
if(BASH AND NOT WIN32)
  add_test(NAME banned_ops COMMAND ${BASH} ${CMAKE_SOURCE_DIR}/tools/check_banned.sh)
  set_tests_properties(banned_ops PROPERTIES LABELS fast)
endif()
