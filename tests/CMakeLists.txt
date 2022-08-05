project(tests)

set(SOURCE_FILES
    src/tests/base_utils.cpp
    src/tests/bit_reader_and_bit_writer.cpp
    src/tests/float3.cpp
    src/tests/graph.cpp
    src/tests/math_utils.cpp
    src/tests/matrix4x4.cpp
    src/tests/quantization.cpp
    src/tests/quaternion.cpp
    src/tests/skeleton_and_clip.cpp
    src/tests/skeleton_pose.cpp
    src/tests/string_id.cpp
    src/tests/test_utils.h
    src/tests/transform.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE eely eely_app external_googletest)