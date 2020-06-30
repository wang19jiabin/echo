cmake_minimum_required(VERSION 3.10)
project(server)
add_compile_options(-Wall -g)

aux_source_directory(. SRC)
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} pthread)

#aux_source_directory(client CLI)
#add_executable(client ${CLI})
