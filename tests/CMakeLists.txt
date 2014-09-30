macro(link_test_libs project_name)
    target_link_libraries(${project_name} ppconsul)
    target_link_libraries(${project_name} json11)

    target_link_libraries(${project_name} ${Boost_LIBRARIES})
    target_link_libraries(${project_name} ${CPPNETLIB_LIBRARIES})
endmacro()

include_directories(../include)
include_directories(${JSON11_INCLUDE_DIRS})
include_directories(${CATCH_INCLUDE_DIRS})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CPPNETLIB_INCLUDE_DIRS})

project(consul-get)
add_executable(${PROJECT_NAME} consul_get_app.cpp)
link_test_libs(${PROJECT_NAME})

project(unittests)
add_executable(${PROJECT_NAME} main.cpp
    consul.cpp
    parameters.cpp
)
link_test_libs(${PROJECT_NAME})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})


project(tests-kv)
add_executable(${PROJECT_NAME}
    kv.cpp
)
link_test_libs(${PROJECT_NAME})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})