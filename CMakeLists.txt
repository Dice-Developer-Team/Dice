cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" 
  CACHE STRING "Minimum OS X deployment version")
project(Dice)
set(PROJECT_NAME w4123.Dice)

include_directories(Dice QQAPI quickjspp)

aux_source_directory(Dice Dice_SRC)
aux_source_directory(QQAPI QQAPI_SRC)
aux_source_directory(quickjspp JS_SRC)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    include_directories(quickjspp/win)
    add_library(${PROJECT_NAME} SHARED ${Dice_SRC} ${QQAPI_SRC} ${JS_SRC} Dice/Resource.rc)
else()
    add_library(${PROJECT_NAME} SHARED ${Dice_SRC} ${QQAPI_SRC} ${JS_SRC})
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY PREFIX "")
set_property(TARGET ${PROJECT_NAME} PROPERTY SUFFIX ".dll")
set_property(TARGET ${PROJECT_NAME} PROPERTY RELEASE_POSTFIX "${DLL_SUFFIX}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-fpermissive ${CMAKE_CXX_FLAGS}")
    target_link_libraries(${PROJECT_NAME} PRIVATE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>:stdc++fs>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-invalid-source-encoding ${CMAKE_CXX_FLAGS}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-invalid-source-encoding ${CMAKE_CXX_FLAGS}")
    target_link_libraries(${PROJECT_NAME} PRIVATE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>:c++fs>)
    if(CMAKE_SYSTEM_NAME STREQUAL Android)
    	set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++ ${CMAKE_SHARED_LINKER_FLAGS}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/source-charset:utf-8 /execution-charset:utf-8 ${CMAKE_CXX_FLAGS} /EHsc-")
    string(REGEX MATCH static IS_STATIC ${VCPKG_TARGET_TRIPLET})
    if(IS_STATIC STREQUAL static)
        set_property(TARGET ${PROJECT_NAME} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

include(CheckCSourceCompiles)
add_compile_definitions(CONFIG_BIGNUM)
add_compile_definitions(JS_STRICT_NAN_BOXING)

list(APPEND CMAKE_REQUIRED_LIBRARIES "-Wl,-Bsymbolic-functions -Wl,-Bsymbolic")
	
check_c_source_compiles(
[=[
int main ()
{
    return 0;
}
]=]
BSYMBOLIC_WORKS
)
list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES "-Wl,-Bsymbolic-functions -Wl,-Bsymbolic")

if (NOT(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") AND BSYMBOLIC_WORKS)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic-functions -Wl,-Bsymbolic")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT supported)

if(supported)
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(CMAKE_SYSTEM_NAME STREQUAL Android)
    	STRING(REGEX REPLACE "-fuse-ld=gold" "" CMAKE_CXX_LINK_OPTIONS_IPO "${CMAKE_CXX_LINK_OPTIONS_IPO}")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    find_package(AWSSDK CONFIG COMPONENTS core s3 REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${AWSSDK_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${AWSSDK_LIBRARIES})
else()
    find_package(CURL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
    if(NOT (CMAKE_SYSTEM_NAME STREQUAL Android))
        find_package(Iconv REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE Iconv::Iconv)
    endif()
endif()

find_package(Lua REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LUA_LIBRARIES})
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp::yaml-cpp)
find_package(tinyxml2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml2::tinyxml2)
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
find_package(civetweb CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE civetweb::civetweb civetweb::civetweb-cpp)
find_package(libzip CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libzip::zip)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(${PROJECT_NAME} PROPERTIES C_VISIBILITY_PRESET hidden)

if(CMAKE_SYSTEM_NAME STREQUAL Android)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/libiconv/${ANDROID_ABI}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/libiconv/${ANDROID_ABI}/lib/libiconv.a)
else()
    find_package(unofficial-libgit2 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::libgit2::libgit2)
    find_package(Python3 COMPONENTS Development REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Python)
endif()

if(NOT ((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")))
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "-s -Wl,--exclude-libs,ALL")
endif()
