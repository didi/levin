cmake_minimum_required (VERSION 2.6)
project (levin)

file(GLOB LEVIN_LIB_SOURCES "src/*.cpp")
file(GLOB LEVIN_LIB_HEADERS "src/*.h" "src/*.hpp")
file(GLOB LEVIN_LIB_HEADERS_DETAILS "src/details/*.h" "src/details/*.hpp")

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "-g -pipe -W -Wall -fPIC -std=c++0x")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(PROJECT_DIR  ${CMAKE_CURRENT_SOURCE_DIR})

# Thirdparty dependency dir
set(THIRD_PARTY /usr/include/)

set(BOOST_MODULE ${THIRD_PARTY}/boost)
set(BOOST_MODULE_INCLUDE ${BOOST_MODULE}/include)
file(GLOB_RECURSE BOOST_MODULE_LIB "${BOOST_MODULE}/lib/*.a")
message( STATUS "Boost include dir: " ${BOOST_MODULE_INCLUDE})
set(BOOST_ROOT ${BOOST_MODULE_INCLUDE})
find_package(Boost 1.56.0 REQUIRED)

set(GTEST_MODULE ${THIRD_PARTY}/gtest)
set(GTEST_MODULE_INCLUDE ${GTEST_MODULE}/include)
file(GLOB_RECURSE GTEST_MODULE_LIB "${GTEST_MODULE}/lib/*.a")

set(OPENSSL_MODULE ${THIRD_PARTY}/openssl)
set(OPENSSL_MODULE_INCLUDE ${OPENSSL_MODULE}/include)
file(GLOB_RECURSE OPENSSL_MODULE_LIB "${OPENSSL_MODULE}/lib/*.a")

include_directories(src/
    ${BOOST_MODULE_INCLUDE}
    ${GTEST_MODULE_INCLUDE}
    ${OPENSSL_MODULE_INCLUDE})

  add_library(levin STATIC ${LEVIN_LIB_SOURCES})

target_link_libraries(levin
    ${BOOST_MODULE_LIB}
    ${OPENSSL_MODULE_LIB}
    pthread
    crypto
    ssl
    rt)

install(TARGETS levin DESTINATION ${PROJECT_DIR}/output/lib)

install(FILES ${LEVIN_LIB_HEADERS} DESTINATION ${PROJECT_DIR}/output/include)
install(FILES ${LEVIN_LIB_HEADERS_DETAILS} DESTINATION ${PROJECT_DIR}/output/include/details)

add_subdirectory(test)
add_subdirectory(example)
