cmake_minimum_required(VERSION 3.9)

project(db-proxy)

find_package(Boost 1.66 REQUIRED COMPONENTS system)

set(SOURCES main.cpp
    debug.hpp
    parser.cpp
    parser.hpp
    logger.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_NO_DEPRECATED BOOST_ASIO_STANDALONE)
target_link_libraries(${PROJECT_NAME} Boost::boost Boost::system)

# Rewrite options for static builds
if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "static")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE /MT)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /MTd)
    endif()
endif()
