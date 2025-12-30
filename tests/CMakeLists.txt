include(FetchContent)

# GoogleTest
set(GTEST_FORCE_SHARED_CRT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

# Core object library (no UI deps)
# 我们只测试核心逻辑，不涉及UI组件
set(CORE_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/../src/core/Entities.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/core/JsonStorage.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../src/core/LedgerService.cpp
)

add_library(core_objects OBJECT ${CORE_SOURCES})
target_include_directories(core_objects PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../src)
target_link_libraries(core_objects PUBLIC Qt5::Core)
if (MSVC)
  target_compile_options(core_objects PRIVATE /utf-8)
else()
  target_compile_options(core_objects PRIVATE -Wall -Wextra -pedantic)
endif()

enable_testing()

# Unit tests
# 单元测试
add_executable(unit_tests)
target_link_libraries(unit_tests PRIVATE core_objects gtest_main Qt5::Core)
if (MSVC)
  target_compile_options(unit_tests PRIVATE /utf-8)
endif()
target_sources(unit_tests PRIVATE
  unit/smoke_tests.cpp
  unit/json_storage_tests.cpp
  unit/ledger_bill_tests.cpp
  unit/reminder_tests.cpp
  unit/category_tests.cpp
  unit/auth_social_tests.cpp
)
add_test(NAME unit COMMAND unit_tests)

# Integration tests
# 集成测试
add_executable(integration_tests
  integration/basic_flow_tests.cpp
  integration/end_to_end_tests.cpp
)
target_link_libraries(integration_tests PRIVATE core_objects gtest_main Qt5::Core)
if (MSVC)
  target_compile_options(integration_tests PRIVATE /utf-8)
endif()
add_test(NAME integration COMMAND integration_tests)

# Optional: build libFuzzer harness (Clang only)
option(ENABLE_FUZZ "Build fuzz target with libFuzzer" OFF)
if(ENABLE_FUZZ)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_executable(fuzz_entities
      fuzz/fuzz_entities.cpp
      $<TARGET_OBJECTS:core_objects>
    )
    target_include_directories(fuzz_entities PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)
    target_link_libraries(fuzz_entities PRIVATE Qt5::Core)
    target_compile_options(fuzz_entities PRIVATE -fsanitize=fuzzer)
    target_link_options(fuzz_entities PRIVATE -fsanitize=fuzzer)
  else()
    message(WARNING "ENABLE_FUZZ=ON 需要使用 Clang 编译器，当前编译器不支持，跳过 fuzz_entities 目标。")
  endif()
endif()

# Optional: coverage HTML via OpenCppCoverage on Windows
find_program(OPENCPPCOVERAGE_EXECUTABLE
  NAMES OpenCppCoverage.exe OpenCppCoverage
  PATHS "C:/Program Files/OpenCppCoverage" "C:/Program Files (x86)/OpenCppCoverage"
)
if(OPENCPPCOVERAGE_EXECUTABLE)
  # Normalize paths to Windows separators for OpenCppCoverage.
  file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/src/core" OPENCPPCOVERAGE_SOURCE_NATIVE)
  file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/coverage_html" OPENCPPCOVERAGE_EXPORT_NATIVE)
  add_custom_target(coverage_html
    COMMAND "${OPENCPPCOVERAGE_EXECUTABLE}"
      --sources "${OPENCPPCOVERAGE_SOURCE_NATIVE}"
      --export_type "html:${OPENCPPCOVERAGE_EXPORT_NATIVE}"
      -- "$<TARGET_FILE:unit_tests>"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    COMMENT "Generating coverage report with OpenCppCoverage"
  )
endif()
