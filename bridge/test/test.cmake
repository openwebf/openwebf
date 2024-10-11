list(APPEND WEBF_TEST_SOURCE
  include/webf_bridge_test.h
  webf_bridge_test.cc
  polyfill/dist/testframework.cc
)

set(gtest_disable_pthreads ON)

add_subdirectory(./third_party/googletest)
add_subdirectory(./third_party/benchmark)

list(APPEND WEBF_TEST_SOURCE
        test/webf_test_context.cc
        test/webf_test_context.h
        )
list(APPEND WEBF_UNIT_TEST_SOURCEURCE
  ./test/webf_test_env.cc
  ./test/webf_test_env.h
  ./bindings/qjs/atomic_string_test.cc
  ./bindings/qjs/script_value_test.cc
  ./bindings/qjs/qjs_engine_patch_test.cc
  ./core/dom/events/custom_event_test.cc
  ./core/executing_context_test.cc
  ./core/frame/console_test.cc
  ./core/frame/module_manager_test.cc
  ./core/dom/events/event_target_test.cc
  ./core/dom/document_test.cc
  ./core/dom/legacy/element_attribute_test.cc
  ./core/dom/node_test.cc
  ./core/html/html_collection_test.cc
  ./core/dom/element_test.cc
  ./core/frame/dom_timer_test.cc
  ./core/frame/window_test.cc
  ./core/css/inline_css_style_declaration_test.cc
  ./core/html/html_element_test.cc
  ./core/html/custom/widget_element_test.cc
  ./core/timing/performance_test.cc
)

### webf_unit_test executable
add_executable(webf_unit_test
  ${WEBF_UNIT_TEST_SOURCEURCE}
  ${WEBF_TEST_SOURCE}
  ${BRIDGE_SOURCE}
)

target_include_directories(webf_unit_test PUBLIC ./third_party/googletest/googletest/include ${BRIDGE_INCLUDE} ./test)
target_link_libraries(webf_unit_test gtest gtest_main ${BRIDGE_LINK_LIBS})

target_compile_options(quickjs PUBLIC -DDUMP_LEAKS=1)
target_compile_options(webf PUBLIC -DDUMP_LEAKS=1)

target_compile_definitions(webf_unit_test PUBLIC -DFLUTTER_BACKEND=0)
target_compile_definitions(webf_unit_test PUBLIC -DSPEC_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(webf_unit_test PUBLIC -DUNIT_TEST=1)

if (DEFINED ENV{LIBRARY_OUTPUT_DIR})
  set_target_properties(webf_unit_test
          PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
          )
endif()

# Run webf integration without flutter.
add_executable(webf_integration_test
  ${WEBF_TEST_SOURCE}
  ${BRIDGE_SOURCE}
  ./test/webf_test_env.cc
  ./test/webf_test_env.h
  ./test/run_integration_test.cc
  )
target_include_directories(webf_integration_test PUBLIC ./third_party/googletest/googletest/include ${BRIDGE_INCLUDE} ./test)
target_link_libraries(webf_integration_test gtest gtest_main ${BRIDGE_LINK_LIBS})
target_compile_definitions(webf_integration_test PUBLIC -DFLUTTER_BACKEND=0)
target_compile_definitions(webf_integration_test PUBLIC -DUNIT_TEST=1)
target_compile_definitions(webf_integration_test PUBLIC -DSPEC_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

# Benchmark test
add_executable(webf_benchmark
  ${WEBF_TEST_SOURCE}
  ${BRIDGE_SOURCE}
  ./test/webf_test_env.cc
  ./test/webf_test_env.h
  ./test/benchmark/create_element.cc
)
target_include_directories(webf_benchmark PUBLIC
  ./third_party/googletest/googletest/include
  ./third_party/benchmark/include/
  ${BRIDGE_INCLUDE}
  ./test)
target_link_libraries(webf_benchmark gtest gtest_main benchmark::benchmark  ${BRIDGE_LINK_LIBS})
target_compile_definitions(webf_benchmark PUBLIC -DFLUTTER_BACKEND=0)
target_compile_definitions(webf_benchmark PUBLIC -DUNIT_TEST=1)

# Built libwebf_test.dylib library for integration test with flutter.
add_library(webf_test SHARED ${WEBF_TEST_SOURCE})
target_link_libraries(webf_test PRIVATE ${BRIDGE_LINK_LIBS} webf)
target_include_directories(webf_test PRIVATE
  ${BRIDGE_INCLUDE}
  ./test
  ${CMAKE_CURRENT_SOURCE_DIR} PUBLIC ./include)

if (DEFINED ENV{LIBRARY_OUTPUT_DIR})
  set_target_properties(webf_test
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
    )
endif()
