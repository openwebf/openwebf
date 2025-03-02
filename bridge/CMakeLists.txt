cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)
project(WebF)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)
# MSBuild are slower than Clang/GCC to support new CXX standards.
if(MSVC)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  set(BUILD_SHARED_LIBS TRUE)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

set(WEBF_JS_ENGINE $ENV{WEBF_JS_ENGINE})
if(NOT WEBF_JS_ENGINE)
    set(WEBF_JS_ENGINE "quickjs")
endif()

find_package(Threads REQUIRED)

if(MSVC)
  add_compile_options(/MP)
endif()

if (${ENABLE_PROFILE})
  add_definitions(-DENABLE_PROFILE=1)
else ()
  add_definitions(-DENABLE_PROFILE=0)
endif ()

if (${ENABLE_LOG})
  add_definitions(-DENABLE_LOG=1)
else()
  add_definitions(-DENABLE_LOG=0)
endif()

if(NOT MSVC)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if( supported )
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
      add_compile_options(-flto)
    endif()
  endif()
endif()

list(APPEND WEBF_PUBLIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/webf_bridge.h
)

set(QUICKJS_PUBLIC_HEADERS
  third_party/quickjs/cutils.h
  third_party/quickjs/libregexp.h
  third_party/quickjs/libregexp-opcode.h
  third_party/quickjs/libunicode.h
  third_party/quickjs/libunicode-table.h
  third_party/quickjs/list.h
  third_party/quickjs/quickjs.h
  third_party/quickjs/quickjs-atom.h
  third_party/quickjs/quickjs-opcode.h
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_compile_options(-fPIC)
endif()

if (ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address -fno-omit-frame-pointer)
endif ()

if (NOT MSVC)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    # Avoid quickjs stackoverflow.
    add_compile_options(-O1)
  endif()
endif()

if (DEFINED PLATFORM)
  if (${PLATFORM} STREQUAL "OS")
    add_compile_options(-fno-aligned-allocation)
  endif()
endif()

list(APPEND BRIDGE_SOURCE
  foundation/logging.cc
  foundation/native_string.cc
  foundation/ui_task_queue.cc
  foundation/shared_ui_command.cc
  foundation/inspector_task_queue.cc
  foundation/task_queue.cc
  foundation/string_view.cc
  foundation/native_value.cc
  foundation/native_type.cc
  foundation/stop_watch.cc
  foundation/profiler.cc
  foundation/dart_readable.cc
  foundation/rust_readable.cc
  foundation/ui_command_buffer.cc
  foundation/ui_command_strategy.cc
  polyfill/dist/polyfill.cc
  multiple_threading/dispatcher.cc
  multiple_threading/looper.cc
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dart/include/dart_api_dl.c
  )

list(APPEND GUMBO_PARSER
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/attribute.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/char_ref.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/error.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/string_buffer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/string_piece.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/tag.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/string_piece.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/tokenizer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/utf8.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/util.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gumbo-parser/src/vector.c
  )

list(APPEND BRIDGE_INCLUDE
  ${CMAKE_CURRENT_LIST_DIR}/foundation
  ${CMAKE_CURRENT_LIST_DIR}/out
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/polyfill/dist
  ${CMAKE_CURRENT_LIST_DIR}/core_rs/include
  ${CMAKE_CURRENT_LIST_DIR}/third_party/dart
  ${ADDITIONAL_INCLUDE_DIRS}
  )

if (${WEBF_JS_ENGINE} MATCHES "quickjs")
  add_compile_options(-DWEBF_QUICK_JS_ENGINE=1)

  execute_process(
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/VERSION
    OUTPUT_VARIABLE QUICKJS_VERSION
  )

  add_library(modb STATIC
    third_party/modp_b64/modp_b64.cc
  )

  if (NOT MSVC)
    list(APPEND QUICK_JS_SOURCE third_party/quickjs/src/libbf.c)
  endif()

  list(APPEND QUICK_JS_SOURCE
    third_party/quickjs/src/libbf.c
    third_party/quickjs/src/cutils.c
    third_party/quickjs/src/libregexp.c
    third_party/quickjs/src/libunicode.c
    third_party/quickjs/src/core/string.c
    third_party/quickjs/src/core/function.c
    third_party/quickjs/src/core/memory.c
    third_party/quickjs/src/core/bytecode.c
    third_party/quickjs/src/core/object.c
    third_party/quickjs/src/core/exception.c
    third_party/quickjs/src/core/gc.c
    third_party/quickjs/src/core/malloc.c
    third_party/quickjs/src/core/shape.c
    third_party/quickjs/src/core/parser.c
    third_party/quickjs/src/core/convertion.c
    third_party/quickjs/src/core/runtime.c
    third_party/quickjs/src/core/module.c
    third_party/quickjs/src/core/ic.c
    third_party/quickjs/src/core/builtins/js-array.c
    third_party/quickjs/src/core/builtins/js-async-function.c
    third_party/quickjs/src/core/builtins/js-async-generator.c
    third_party/quickjs/src/core/builtins/js-atomics.c
    third_party/quickjs/src/core/builtins/js-big-num.c
    third_party/quickjs/src/core/builtins/js-boolean.c
    third_party/quickjs/src/core/builtins/js-date.c
    third_party/quickjs/src/core/builtins/js-function.c
    third_party/quickjs/src/core/builtins/js-generator.c
    third_party/quickjs/src/core/builtins/js-json.c
    third_party/quickjs/src/core/builtins/js-map.c
    third_party/quickjs/src/core/builtins/js-math.c
    third_party/quickjs/src/core/builtins/js-number.c
    third_party/quickjs/src/core/builtins/js-object.c
    third_party/quickjs/src/core/builtins/js-closures.c
    third_party/quickjs/src/core/builtins/js-operator.c
    third_party/quickjs/src/core/builtins/js-promise.c
    third_party/quickjs/src/core/builtins/js-proxy.c
    third_party/quickjs/src/core/builtins/js-reflect.c
    third_party/quickjs/src/core/builtins/js-regexp.c
    third_party/quickjs/src/core/builtins/js-string.c
    third_party/quickjs/src/core/builtins/js-symbol.c
    third_party/quickjs/src/core/builtins/js-typed-array.c
  )

  if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions(-DDDEBUG)
  endif ()

  if(${STATIC_QUICKJS})
    add_library(quickjs STATIC ${QUICK_JS_SOURCE})
  else()
    add_library(quickjs SHARED ${QUICK_JS_SOURCE})
  endif()

  # PThread compact
  if(MSVC)
    # Using `add_subdirectory` will failed at cmake install starge due to unknown issues:
    # https://github.com/flutter/flutter/issues/95530
    set(PTHREADS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/compat/win32/pthread-win32")
    add_library(pthreadVC3 SHARED ${PTHREADS_ROOT}/pthread-JMP.c ${PTHREADS_ROOT}/version.rc)
    target_compile_definitions(pthreadVC3
      PRIVATE
      _UNICODE
      UNICODE
      WIN32
      _WINDOWS
      _WINDLL
      PTW32_CLEANUP_C
      UNICODE
      _UNICODE
      ENABLE_LOG=0
      PTW32_ARCHAMD64
      PTW32_BUILD_INLINED
      HAVE_CONFIG_H
      PTW32_RC_MSC
    )
    target_include_directories(pthreadVC3 PUBLIC ${PTHREADS_ROOT})
  endif()

  if(WIN32)
    target_link_libraries(quickjs pthreadVC3)
    target_include_directories(quickjs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/compat/win32/atomic)
    target_compile_definitions(quickjs PUBLIC HAVE_STRUCT_TIMESPEC=1 _HAS_EXCEPTIONS=1 -DCONFIG_BIGNUM=1)
  else()
    target_link_libraries(quickjs Threads::Threads)
  endif()

  set(MI_OVERRIDE OFF)
  if (NOT MSVC AND NOT DEFINED USE_SYSTEM_MALLOC)
    add_compile_definitions(ENABLE_MI_MALLOC=1)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/vendor/mimalloc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/vendor/mimalloc/include)
    target_link_libraries(quickjs mimalloc-static)
  endif()

  target_include_directories(quickjs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/include)

  if (MSVC)
    target_include_directories(quickjs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/compat/win32/pthread-win32)
    target_include_directories(quickjs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs/compat/win32/atomic)
  endif()

  list(APPEND BRIDGE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
  list(APPEND BRIDGE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/modp_b64/include)
  list(APPEND BRIDGE_LINK_LIBS quickjs)
  list(APPEND BRIDGE_LINK_LIBS modb)

  list(APPEND BRIDGE_SOURCE
    # Binding files
    bindings/qjs/dictionary_base.cc
    bindings/qjs/js_based_event_listener.cc
    bindings/qjs/js_event_handler.cc
    bindings/qjs/js_event_listener.cc
    bindings/qjs/binding_initializer.cc
    bindings/qjs/member_installer.cc
    bindings/qjs/source_location.cc
    bindings/qjs/cppgc/gc_visitor.cc
    bindings/qjs/cppgc/mutation_scope.cc
    bindings/qjs/script_wrappable.cc
    bindings/qjs/native_string_utils.cc
    bindings/qjs/qjs_engine_patch.cc
    bindings/qjs/qjs_function.cc
    bindings/qjs/script_value.cc
    bindings/qjs/script_promise.cc
    bindings/qjs/script_promise_resolver.cc
    bindings/qjs/atomic_string.cc
    bindings/qjs/exception_state.cc
    bindings/qjs/exception_message.cc
    bindings/qjs/rejected_promises.cc
    bindings/qjs/union_base.cc
    # Core sources
    webf_bridge.cc
    core/executing_context.cc
    core/script_forbidden_scope.cc
    core/script_state.cc
    core/page.cc
    core/dart_methods.cc
    core/api/exception_state.cc
    core/api/event_target.cc
    core/api/node.cc
    core/api/executing_context.cc
    core/api/container_node.cc
    core/api/document.cc
    core/api/element.cc
    core/api/html_element.cc
    core/api/html_image_element.cc
    core/api/html_canvas_element.cc
    core/api/document_fragment.cc
    core/api/window.cc
    core/api/text.cc
    core/api/comment.cc
    core/api/css_style_declaration.cc
    core/api/character_data.cc
    core/api/script_value_ref.cc
    core/dart_isolate_context.cc
    core/dart_context_data.cc
    core/executing_context_data.cc
    core/fileapi/blob.cc
    core/fileapi/blob_part.cc
    core/fileapi/blob_property_bag.cc
    core/frame/console.cc
    core/frame/dom_timer.cc
    core/frame/dom_timer_coordinator.cc
    core/frame/window_or_worker_global_scope.cc
    core/frame/module_listener.cc
    core/frame/module_listener_container.cc
    core/frame/module_manager.cc
    core/frame/module_callback.cc
    core/frame/module_context_coordinator.cc
    core/frame/window.cc
    core/frame/screen.cc
    core/frame/legacy/location.cc
    core/timing/performance.cc
    core/timing/performance_mark.cc
    core/timing/performance_entry.cc
    core/timing/performance_measure.cc
    core/css/css_style_declaration.cc
    core/css/inline_css_style_declaration.cc
    core/css/computed_css_style_declaration.cc
    core/dom/frame_request_callback_collection.cc
    core/dom/events/registered_eventListener.cc
    core/dom/events/event_listener_map.cc
    core/dom/events/event.cc
    core/dom/events/custom_event.cc
    core/dom/events/event_target.cc
    core/dom/events/event_listener_map.cc
    core/dom/events/event_target_impl.cc
    core/binding_object.cc
    core/dom/node.cc
    core/dom/node_list.cc
    core/dom/static_node_list.cc
    core/dom/node_traversal.cc
    core/dom/live_node_list_base.cc
    core/dom/character_data.cc
    core/dom/comment.cc
    core/dom/text.cc
    core/dom/tree_scope.cc
    core/dom/element.cc
    core/dom/parent_node.cc
    core/dom/element_data.cc
    core/dom/document.cc
    core/dom/dom_token_list.cc
    core/dom/dom_string_map.cc
    core/dom/space_split_string.cc
    core/dom/scripted_animation_controller.cc
    core/dom/node_data.cc
    core/dom/document_fragment.cc
    core/dom/child_node_list.cc
    core/dom/empty_node_list.cc
    core/dom/mutation_observer.cc
    core/dom/mutation_observer_registration.cc
    core/dom/mutation_observer_interest_group.cc
    core/dom/mutation_record.cc
    core/dom/child_list_mutation_scope.cc
    core/dom/container_node.cc
    core/html/custom/widget_element.cc
    core/events/error_event.cc
    core/events/message_event.cc
    core/events/animation_event.cc
    core/events/close_event.cc
    core/events/ui_event.cc
    core/events/focus_event.cc
    core/events/gesture_event.cc
    core/events/hashchange_event.cc
    core/events/input_event.cc
    core/events/touch_event.cc
    core/events/mouse_event.cc
    core/events/hybrid_router_change_event.cc
    core/events/pop_state_event.cc
    core/events/pointer_event.cc
    core/events/transition_event.cc
    core/events/intersection_change_event.cc
    core/events/keyboard_event.cc
    core/events/promise_rejection_event.cc
    core/html/parser/html_parser.cc
    core/html/html_element.cc
    core/html/html_div_element.cc
    core/html/html_head_element.cc
    core/html/html_body_element.cc
    core/html/html_html_element.cc
    core/html/html_template_element.cc
    core/html/html_all_collection.cc
    core/html/html_anchor_element.cc
    core/html/html_image_element.cc
    core/html/html_script_element.cc
    core/html/html_iframe_element.cc
    core/html/html_link_element.cc
    core/html/html_unknown_element.cc
    core/html/image.cc
    core/html/html_collection.cc
    core/html/canvas/html_canvas_element.cc
    core/html/canvas/canvas_rendering_context.cc
    core/html/canvas/canvas_rendering_context_2d.cc
    core/html/canvas/canvas_gradient.cc
    core/html/canvas/canvas_pattern.cc
    core/html/canvas/path_2d.cc
    core/geometry/dom_matrix.cc
    core/geometry/dom_matrix_read_only.cc
    core/geometry/dom_point.cc
    core/geometry/dom_point_read_only.cc
    core/html/forms/html_button_element.cc
    core/html/forms/html_input_element.cc
    core/html/forms/html_form_element.cc
    core/html/forms/html_textarea_element.cc

    core/native/native_loader.cc
    core/native/script_value_ref.cc

    # SVG files
    core/svg/svg_element.cc
    core/svg/svg_graphics_element.cc
    core/svg/svg_geometry_element.cc
    core/svg/svg_text_content_element.cc
    core/svg/svg_text_positioning_element.cc

    core/svg/svg_svg_element.cc
    core/svg/svg_path_element.cc
    core/svg/svg_rect_element.cc
    core/svg/svg_text_element.cc
    core/svg/svg_g_element.cc
    core/svg/svg_circle_element.cc
    core/svg/svg_ellipse_element.cc
    core/svg/svg_style_element.cc
    core/svg/svg_line_element.cc

    # Legacy implements, should remove them in the future.
    core/dom/legacy/element_attributes.cc
    core/dom/legacy/bounding_client_rect.cc
    core/input/touch.cc
    core/input/touch_list.cc
    )

  # Gen sources.
  list(APPEND BRIDGE_SOURCE
    out/names_installer.cc
    out/qjs_console.cc
    out/qjs_module_manager.cc
    out/qjs_window_or_worker_global_scope.cc
    out/qjs_window.cc
    out/qjs_location.cc
    out/qjs_blob.cc
    out/qjs_event.cc
    out/qjs_add_event_listener_options.cc
    out/qjs_event_listener_options.cc
    out/qjs_error_event.cc
    out/qjs_message_event.cc
    out/qjs_message_event_init.cc
    out/qjs_close_event.cc
    out/qjs_close_event_init.cc
    out/qjs_focus_event.cc
    out/qjs_focus_event_init.cc
    out/qjs_input_event.cc
    out/qjs_input_event_init.cc
    out/qjs_pop_state_event.cc
    out/qjs_pop_state_event_init.cc
    out/qjs_ui_event.cc
    out/qjs_ui_event_init.cc
    out/qjs_gesture_event.cc
    out/qjs_hashchange_event.cc
    out/qjs_hashchange_event_init.cc
    out/qjs_gesture_event_init.cc
    out/qjs_intersection_change_event.cc
    out/qjs_intersection_change_event_init.cc
    out/qjs_touch.cc
    out/qjs_touch_init.cc
    out/qjs_touch_list.cc
    out/qjs_mutation_record.cc
    out/qjs_mutation_observer.cc
    out/qjs_mutation_observer_init.cc
    out/qjs_mutation_observer_registration.cc
    out/qjs_touch_event.cc
    out/qjs_touch_event_init.cc
    out/qjs_hybrid_router_change_event.cc
    out/qjs_hybrid_router_change_event_init.cc
    out/qjs_pointer_event.cc
    out/qjs_pointer_event_init.cc
    out/qjs_mouse_event.cc
    out/qjs_mouse_event_init.cc
    out/qjs_transition_event.cc
    out/qjs_transition_event_init.cc
    out/event_factory.cc
    out/qjs_custom_event.cc
    out/qjs_custom_event_init.cc
    out/qjs_keyboard_event.cc
    out/qjs_keyboard_event_init.cc
    out/qjs_animation_event.cc
    out/qjs_animation_event_init.cc
    out/qjs_error_event_init.cc
    out/qjs_event_init.cc
    out/qjs_event_target.cc
    out/qjs_node.cc
    out/qjs_union_dom_stringnode.cc
    out/qjs_document.cc
    out/qjs_element.cc
    out/qjs_dom_token_list.cc
    out/qjs_dom_string_map.cc
    out/qjs_element_attributes.cc
    out/qjs_character_data.cc
    out/qjs_comment.cc
    out/qjs_document_fragment.cc
    out/qjs_bounding_client_rect.cc
    out/qjs_css_style_declaration.cc
    out/qjs_inline_css_style_declaration.cc
    out/qjs_computed_css_style_declaration.cc
    out/qjs_native_loader.cc
    out/qjs_text.cc
    out/qjs_screen.cc
    out/qjs_node_list.cc
    out/event_type_names.cc
    out/built_in_string.cc
    out/mutation_record_types.cc
    out/binding_call_methods.cc
    out/qjs_scroll_options.cc
    out/qjs_scroll_to_options.cc
    out/qjs_html_element.cc
    out/qjs_html_all_collection.cc
    out/qjs_html_collection.cc
    out/qjs_html_anchor_element.cc
    out/qjs_html_div_element.cc
    out/qjs_html_head_element.cc
    out/qjs_html_body_element.cc
    out/qjs_html_html_element.cc
    out/qjs_html_image_element.cc
    out/qjs_html_iframe_element.cc
    out/qjs_html_canvas_element.cc
    out/qjs_html_link_element.cc
    out/qjs_image.cc
    out/qjs_widget_element.cc
    out/qjs_canvas_rendering_context_2d.cc
    out/qjs_canvas_rendering_context.cc
    out/qjs_canvas_gradient.cc
    out/qjs_canvas_pattern.cc
    out/qjs_path_2d.cc
    out/qjs_dom_matrix_init.cc
    out/qjs_dom_matrix.cc
    out/qjs_dom_matrix_read_only.cc
    out/qjs_dom_point_init.cc
    out/qjs_dom_point.cc
    out/qjs_dom_point_read_only.cc
    out/qjs_union_double_sequencedouble.cc
    out/qjs_unionhtml_image_elementhtml_canvas_element.cc
    out/qjs_union_dom_stringcanvas_gradient.cc
    out/qjs_union_sequencedoubledom_matrix_init.cc
    out/qjs_union_doubledom_point_init.cc
    out/canvas_types.cc
    out/qjs_html_button_element.cc
    out/qjs_html_input_element.cc
    out/qjs_html_form_element.cc
    out/qjs_html_textarea_element.cc
    out/qjs_html_script_element.cc
    out/qjs_promise_rejection_event.cc
    out/qjs_promise_rejection_event_init.cc
    out/qjs_unionevent_listener_options_boolean.cc
    out/qjs_unionadd_event_listener_options_boolean.cc
    out/qjs_unionpath_2_d_dom_string.cc
    out/qjs_html_template_element.cc
    out/qjs_html_unknown_element.cc
    out/qjs_performance.cc
    out/qjs_performance_entry.cc
    out/qjs_performance_mark.cc
    out/qjs_performance_measure.cc
    out/performance_entry_names.cc
    out/qjs_performance_measure_options.cc
    out/qjs_performance_mark_options.cc
    out/performance_mark_constants.cc
    out/html_element_factory.cc
    out/html_names.cc
    out/script_type_names.cc
    out/defined_properties.cc
    out/element_attribute_names.cc
    out/element_namespace_uris.cc

    # SVG generated
    out/svg_names.cc
    out/svg_element_factory.cc

    # SVG files
    out/qjs_svg_element.cc
    out/qjs_svg_graphics_element.cc
    out/qjs_svg_geometry_element.cc
    out/qjs_svg_text_content_element.cc
    out/qjs_svg_text_positioning_element.cc

    out/qjs_svg_svg_element.cc
    out/qjs_svg_path_element.cc
    out/qjs_svg_rect_element.cc
    out/qjs_svg_text_element.cc
    out/qjs_svg_g_element.cc
    out/qjs_svg_circle_element.cc
    out/qjs_svg_ellipse_element.cc
    out/qjs_svg_style_element.cc
    out/qjs_svg_line_element.cc

    # plugin api
    out/plugin_api_event.cc
    out/plugin_api_animation_event.cc
    out/plugin_api_close_event.cc
    out/plugin_api_custom_event.cc
    out/plugin_api_focus_event.cc
    out/plugin_api_gesture_event.cc
    out/plugin_api_hashchange_event.cc
    out/plugin_api_input_event.cc
    out/plugin_api_intersection_change_event.cc
    out/plugin_api_mouse_event.cc
    out/plugin_api_pointer_event.cc
    out/plugin_api_transition_event.cc
    out/plugin_api_ui_event.cc
    )


  if (NOT MSVC)
  # Quickjs use __builtin_frame_address() to get stack pointer, we should add follow options to get it work with -O2
  # https://stackoverflow.com/questions/14735010/how-do-you-get-gccs-builtin-frame-address-to-work-with-o2
  add_compile_options(-fno-optimize-sibling-calls -fno-omit-frame-pointer)
  endif()
  target_compile_options(quickjs PUBLIC -DCONFIG_VERSION=${\"QUICKJS_VERSION\"} -DCONFIG_BIGNUM=1)

endif ()

list(APPEND PUBLIC_HEADER
  include/webf_bridge.h
)

add_library(webf SHARED ${BRIDGE_SOURCE})

if(MSVC)
  target_compile_options(webf PRIVATE /JMC)
endif()

target_compile_definitions(webf PUBLIC -DFLUTTER_BACKEND=1)

add_library(gumbo_parse_static STATIC ${GUMBO_PARSER})
list(APPEND BRIDGE_LINK_LIBS gumbo_parse_static)

if (${IS_ANDROID})
  find_library(log-lib log)

  if (${ANDROID_ABI} MATCHES "armeabi-v7a" OR ${ANDROID_ABI} MATCHES "x86")
    add_definitions(-DANDROID_32_BIT=1)
  endif()

  add_definitions(-DIS_ANDROID=1)
  list(APPEND BRIDGE_LINK_LIBS ${log-lib})
elseif(${IS_IOS})
  add_definitions(-DIS_IOS=1)
endif()


## webf
target_include_directories(webf PRIVATE
  ${BRIDGE_INCLUDE}
  ${CMAKE_CURRENT_SOURCE_DIR} PUBLIC ./include)
target_link_libraries(webf PRIVATE ${BRIDGE_LINK_LIBS})

if ($ENV{WEBF_JS_ENGINE} MATCHES "quickjs" AND NOT MSVC)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    target_compile_options(webf PRIVATE -fno-exceptions -fvisibility=hidden -fno-rtti -fdata-sections -ffunction-sections)
    target_compile_options(quickjs PRIVATE -fno-exceptions -fno-rtti -fdata-sections -ffunction-sections)
  else ()
    ### remove dynamic_cast and exceptions
    target_compile_options(webf PRIVATE -fno-exceptions -fno-rtti)
  endif ()
endif ()

execute_process(
  COMMAND node get_app_ver.js
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/scripts
  OUTPUT_VARIABLE APP_VER
)
string(REPLACE \n "" APP_VER ${APP_VER}) # Remove last \n
add_definitions(-DAPP_VERSION="${APP_VER}") # Read from dartfm version
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HEAD
)
string(REPLACE \n "" GIT_HEAD ${GIT_HEAD}) # Remove last \n
add_definitions(-DAPP_REV="${GIT_HEAD}") # Read from git head sha1

if (${ENABLE_TEST})
  add_compile_definitions(IS_TEST=true)
  include(./test/test.cmake)
endif ()

if (DEFINED ENV{LIBRARY_OUTPUT_DIR})
  set_target_properties(webf
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
  )
  if ($ENV{WEBF_JS_ENGINE} MATCHES "quickjs")
    set_target_properties(quickjs
      PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
    )
    if(MSVC)
      set_target_properties(pthreadVC3
        PROPERTIES
          LIBRARY_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
          RUNTIME_OUTPUT_DIRECTORY "$ENV{LIBRARY_OUTPUT_DIR}"
      )
    endif()
  endif ()
elseif (IS_ANDROID)
  # android do nothing
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")

  set_target_properties(webf PROPERTIES
    OUTPUT_NAME webf_bridge
    FRAMEWORK TRUE
    FRAMEWORK_VERSION C
    MACOSX_FRAMEWORK_IDENTIFIER com.openwebf.webf-bridge
    MACOSX_FRAMEWORK_BUNDLE_VERSION 1.0
    MACOSX_FRAMEWORK_SHORT_VERSION_STRING 1.0
    PUBLIC_HEADER ${WEBF_PUBLIC_HEADERS}
    )

  # If quickjs is static, there will be no quickjs.framework any more.
  if(NOT DEFINED STATIC_QUICKJS)
    set_target_properties(quickjs PROPERTIES
      OUTPUT_NAME quickjs
      FRAMEWORK TRUE
      FRAMEWORK_VERSION C
      MACOSX_FRAMEWORK_IDENTIFIER com.openwebf.quickjs
      MACOSX_FRAMEWORK_BUNDLE_VERSION 1.0
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING 1.0
      PUBLIC_HEADER ${QUICKJS_PUBLIC_HEADERS}
      )
  endif()
endif ()
