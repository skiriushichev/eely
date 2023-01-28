project(eely_app)

set(SOURCE_FILES
  include/eely_app/app.h
  include/eely_app/asset_map.h
  include/eely_app/asset_material.h
  include/eely_app/asset_mesh.h
  include/eely_app/asset_uniform.h
  include/eely_app/component_anim_graph.h
  include/eely_app/component_camera.h
  include/eely_app/component_clip.h
  include/eely_app/component_transform.h
  include/eely_app/component_skeleton.h
  include/eely_app/filesystem_utils.h
  include/eely_app/inputs.h
  include/eely_app/matrix4x4.h
  include/eely_app/scene.h
  include/eely_app/system_camera.h
  include/eely_app/system_render.h
  include/eely_app/system_skeleton.h
  src/eely_app/app.cpp
  src/eely_app/asset_material.cpp
  src/eely_app/asset_mesh.cpp
  src/eely_app/asset_uniform.cpp
  src/eely_app/filesystem_utils.cpp
  src/eely_app/inputs.cpp
  src/eely_app/matrix4x4.cpp
  src/eely_app/scene.cpp
  src/eely_app/system_camera.cpp
  src/eely_app/system_render.cpp
  src/eely_app/system_skeleton.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include src)
target_link_libraries(${PROJECT_NAME} PUBLIC eely external_bgfx external_entt external_fmt external_gsl external_imgui external_imgui_bgfx external_imgui_node_editor external_sdl)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

# Create build rule for a shader
# Only for internal use by "eely_add_app"
function(eely_internal_add_shader)
  cmake_parse_arguments(
    PARSED_ARGS
    "VERTEX;FRAGMENT"
    "SOURCE;DESTINATION"
    ""
    ${ARGN})

  if ((PARSED_ARGS_VERTEX AND PARSED_ARGS_FRAGMENT) OR
      (NOT PARSED_ARGS_VERTEX AND NOT PARSED_ARGS_FRAGMENT))
    message(FATAL_ERROR "Either VERTEX or FRAGMENT should be given to `eely_add_shader`")
  endif()

  if (NOT PARSED_ARGS_SOURCE)
    message(FATAL_ERROR "SOURCE argument must be given to `eely_add_shader` (path to shader source code)")
  endif()

  if (NOT PARSED_ARGS_DESTINATION)
    message(FATAL_ERROR "DESTINATION argument must be given to `eely_add_shader` (absolute path to compiled shader)")
  endif()

  get_target_property(EXTERNAL_BGFX_SOURCE_DIR external_bgfx SOURCE_DIR)

  if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
    set(SHADER_PLATFORM "osx")
    set(SHADER_VERTEX_PROFILE "metal")
    set(SHADER_FRAGMENT_PROFILE "metal")
    set(SHADERC ${EXTERNAL_BGFX_SOURCE_DIR}/extras/${EELY_TARGET_SYSTEM_MACOS_ARM}/shaderc)
  else()
    message(FATAL_ERROR "Unsupported platform")
  endif()

  if (PARSED_ARGS_VERTEX)
    set(SHADER_TYPE vertex)
    set(SHADER_PROFILE ${SHADER_VERTEX_PROFILE})
  elseif(PARSED_ARGS_FRAGMENT)
    set(SHADER_TYPE fragment)
    set(SHADER_PROFILE ${SHADER_FRAGMENT_PROFILE})
  endif()

  add_custom_command(
    OUTPUT ${PARSED_ARGS_DESTINATION}
    COMMAND ${SHADERC} -f ${PARSED_ARGS_SOURCE} -o ${PARSED_ARGS_DESTINATION} --type ${SHADER_TYPE} --platform ${SHADER_PLATFORM} -i ./ -i ${EXTERNAL_BGFX_SOURCE_DIR}/include_shaders -p ${SHADER_PROFILE}
    DEPENDS ${PARSED_ARGS_SOURCE}
    COMMENT "Compiling shader: ${PARSED_ARGS_SOURCE} -> ${PARSED_ARGS_DESTINATION}")
endfunction(eely_internal_add_shader)

# Add executable to a project with default eely app functionality
# Arguments:
#   - SOURCE_FILES: list of C++ sources
#   - RESOURCE_FILES: list of resources to be built and copied to the executable folder, relative to the target's source directory
function(eely_add_app)
  cmake_parse_arguments(
    PARSED_ARGS
    ""
    "TARGET"
    "SOURCE_FILES;RESOURCE_FILES"
    ${ARGN})
  
  add_executable(${PARSED_ARGS_TARGET} ${PARSED_ARGS_SOURCE_FILES})
  target_include_directories(${PARSED_ARGS_TARGET} PUBLIC include)
  target_include_directories(${PARSED_ARGS_TARGET} PRIVATE src)
  target_link_libraries(${PARSED_ARGS_TARGET} PRIVATE eely_app)

  # Process resource files
  # Create commands to build some of them, others stay the same
  # Custom targets build them and copy to executable directory
  # Also, shared resources from `eely_app` are always added to the target

  set(BUILD_AND_COPY_RESOURCES_TARGET ${PARSED_ARGS_TARGET}_resources_build_and_copy)
  set(BUILD_AND_COPY_RESOURCES_DIR ${CMAKE_BINARY_DIR}/${BUILD_AND_COPY_RESOURCES_TARGET})

  get_target_property(EELY_APP_SOURCE_DIR eely_app SOURCE_DIR)
  set(
    RESOURCES_DEFAULT
    ${EELY_APP_SOURCE_DIR}/res/color.fs
    ${EELY_APP_SOURCE_DIR}/res/color.vs
    ${EELY_APP_SOURCE_DIR}/res/solid.fs
    ${EELY_APP_SOURCE_DIR}/res/solid.vs)
  list(APPEND PARSED_ARGS_RESOURCE_FILES ${RESOURCES_DEFAULT})

  set(RESOURCE_OUTPUTS "")
  foreach(RESOURCE_FILE ${PARSED_ARGS_RESOURCE_FILES})
    get_filename_component(RESOURCE_FILE_EXT ${RESOURCE_FILE} EXT)
    get_filename_component(RESOURCE_FILE_ABSOLUTE ${RESOURCE_FILE} ABSOLUTE)

    if (${RESOURCE_FILE} IN_LIST RESOURCES_DEFAULT)
      # Default resources already have absolute paths,
      # copy with path relative to the `eely_app` source folder
      # E.g., custom resource has `res/custom_shader.vs` value,
      # and default shader has `/Users/me/eely/libs/eely_app/res/default_shared.vs`.
      # This default shader should still be copied to target's ${BUILD_AND_COPY_RESOURCES_DIR}
      # as `res/default_shared.vs`
      # TODO: IN_LIST is probably not very efficient
      file(
        RELATIVE_PATH
        RESOURCE_FILE_RELATIVE
        ${EELY_APP_SOURCE_DIR}
        ${RESOURCE_FILE})
      set(RESOURCE_OUTPUT ${BUILD_AND_COPY_RESOURCES_DIR}/${RESOURCE_FILE_RELATIVE})
    else()
      set(RESOURCE_OUTPUT ${BUILD_AND_COPY_RESOURCES_DIR}/${RESOURCE_FILE})
    endif()


    # Create build rule
    if(${RESOURCE_FILE_EXT} STREQUAL ".vs")
      eely_internal_add_shader(
        VERTEX
        SOURCE ${RESOURCE_FILE_ABSOLUTE}
        DESTINATION ${RESOURCE_OUTPUT})
    elseif(${RESOURCE_FILE_EXT} STREQUAL ".fs")
      eely_internal_add_shader(
        FRAGMENT
        SOURCE ${RESOURCE_FILE_ABSOLUTE}
        DESTINATION ${RESOURCE_OUTPUT})
    else()
      add_custom_command(
        OUTPUT ${RESOURCE_OUTPUT}
        DEPENDS ${RESOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE_FILE_ABSOLUTE} ${RESOURCE_OUTPUT})
    endif()

    list(APPEND RESOURCE_OUTPUTS ${RESOURCE_OUTPUT})
  endforeach()

  add_custom_target(
    ${BUILD_AND_COPY_RESOURCES_TARGET}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${BUILD_AND_COPY_RESOURCES_DIR} $<TARGET_FILE_DIR:${PARSED_ARGS_TARGET}>
    DEPENDS ${RESOURCE_OUTPUTS})
  add_dependencies(${PARSED_ARGS_TARGET} ${BUILD_AND_COPY_RESOURCES_TARGET})

endfunction(eely_add_app)