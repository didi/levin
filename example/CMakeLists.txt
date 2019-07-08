set(CMAKE_CXX_FLAGS "-g -O2 -pipe -W -Wall -fPIC -std=c++0x")

file(GLOB_RECURSE EXAMPLE_FILES "${PROJECT_DIR}/example/*.cpp")
foreach (file_path ${EXAMPLE_FILES})
    file(RELATIVE_PATH file_name ${PROJECT_DIR}/example ${file_path})
    string(REGEX MATCH "[A-Za-z0-9_]*" exec_name ${file_name}) 
    add_executable(${exec_name} ${file_path})
    target_link_libraries(${exec_name}
        levin
        ${BOOST_MODULE_LIB}
        ${OPENSSL_MODULE_LIB}
        ${GTEST_MODULE_LIB}
        ${GMOCK_MODULE_LIB}
        pthread
        crypto
        ssl
        rt)
endforeach(file_path)
