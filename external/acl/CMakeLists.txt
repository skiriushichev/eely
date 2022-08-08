project(external_acl)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<CONFIG:DEBUG>:ACL_ON_ASSERT_ABORT ACL_ALLOCATOR_TRACK_ALL_ALLOCATIONS>)