project(eely)

set(SOURCE_FILES
    include/eely/axis_system.h
    include/eely/base_utils.h   
    include/eely/bit_reader.h
    include/eely/bit_writer.h
    include/eely/clip_compression.h
    include/eely/clip_player_base.h
    include/eely/clip_player_uncompressed.h
    include/eely/clip_uncooked.h
    include/eely/clip.h
    include/eely/float3.h
    include/eely/float4.h
    include/eely/graph.h
    include/eely/math_utils.h
    include/eely/measurement_unit.h
    include/eely/project_uncooked.h
    include/eely/project.h
    include/eely/quaternion.h
    include/eely/resource_base.h
    include/eely/skeleton_pose.h
    include/eely/skeleton_uncooked.h
    include/eely/skeleton.h
    include/eely/time_utils.h
    include/eely/transform.h
    src/eely/bit_reader.cpp
    src/eely/bit_writer.cpp
    src/eely/clip_player_base.cpp
    src/eely/clip_player_uncompressed.cpp
    src/eely/clip_uncooked.cpp
    src/eely/clip.cpp
    src/eely/float3.cpp
    src/eely/math_utils.cpp
    src/eely/project_uncooked.cpp
    src/eely/project.cpp
    src/eely/quaternion.cpp
    src/eely/resource_base.cpp
    src/eely/resource_uncooked.cpp
    src/eely/resource.cpp
    src/eely/skeleton_pose.cpp
    src/eely/skeleton_uncooked.cpp
    src/eely/skeleton.cpp
    src/eely/string_id.cpp
    src/eely/transform.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC external_fmt external_gsl)