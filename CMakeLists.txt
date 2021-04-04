cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" 
  CACHE STRING "Minimum OS X deployment version")
project(Dice)

include_directories(CQSDK Dice Lua QQAPI)

aux_source_directory(Dice Dice_SRC)
aux_source_directory(Lua Lua_SRC)
aux_source_directory(CQSDKCPP CQSDK_SRC)
aux_source_directory(QQAPI QQAPI_SRC)

add_library(w4123.Dice SHARED ${Dice_SRC} ${Lua_SRC} ${CQSDK_SRC} ${QQAPI_SRC})

set_property(TARGET w4123.Dice PROPERTY CXX_STANDARD 17)
set_property(TARGET w4123.Dice PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET w4123.Dice PROPERTY PREFIX "")
set_property(TARGET w4123.Dice PROPERTY SUFFIX ".dll")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-fpermissive ${CMAKE_CXX_FLAGS}")
    target_link_libraries(w4123.Dice PRIVATE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>:stdc++fs>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-invalid-source-encoding ${CMAKE_CXX_FLAGS}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-fpermissive -Wno-invalid-source-encoding ${CMAKE_CXX_FLAGS}")
    target_link_libraries(w4123.Dice PRIVATE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>:c++fs>)
    if(CMAKE_SYSTEM_NAME STREQUAL Android)
    	set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++ ${CMAKE_SHARED_LINKER_FLAGS}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(REGEX MATCH static IS_STATIC ${VCPKG_TARGET_TRIPLET})
    if(IS_STATIC STREQUAL static)
        set_property(TARGET w4123.Dice PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

include(CheckCSourceCompiles)

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

if (BSYMBOLIC_WORKS)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic-functions -Wl,-Bsymbolic")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT supported)

if(supported)
    set_property(TARGET w4123.Dice PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(CMAKE_SYSTEM_NAME STREQUAL Android)
    	STRING(REGEX REPLACE "-fuse-ld=gold" "" CMAKE_CXX_LINK_OPTIONS_IPO ${CMAKE_CXX_LINK_OPTIONS_IPO})
    endif()
endif()


if(NOT ((CMAKE_SYSTEM_NAME STREQUAL Windows) OR (CMAKE_SYSTEM_NAME STREQUAL Android)))
    find_package(CURL REQUIRED)
    target_link_libraries(w4123.Dice PRIVATE CURL::libcurl)
    find_package(Iconv REQUIRED)
    target_link_libraries(w4123.Dice PRIVATE Iconv::Iconv)
endif()

find_package(OpenSSL REQUIRED)
target_link_libraries(w4123.Dice PRIVATE OpenSSL::SSL OpenSSL::Crypto)
find_package(AWSSDK CONFIG COMPONENTS core s3 REQUIRED)
target_include_directories(w4123.Dice PRIVATE ${AWSSDK_INCLUDE_DIRS})
target_link_libraries(w4123.Dice PRIVATE ${AWSSDK_LIBRARIES})

set_target_properties(w4123.Dice PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(w4123.Dice PROPERTIES C_VISIBILITY_PRESET hidden)

if(CMAKE_SYSTEM_NAME STREQUAL Android)
    target_include_directories(w4123.Dice PRIVATE ${CMAKE_SOURCE_DIR}/libiconv/${ANDROID_ABI}/include)
    target_link_libraries(w4123.Dice PRIVATE ${CMAKE_SOURCE_DIR}/libiconv/${ANDROID_ABI}/lib/libiconv.a)
endif()

if(NOT ((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")))
    set_target_properties(w4123.Dice PROPERTIES LINK_FLAGS_RELEASE "-s -Wl,--exclude-libs,ALL")
endif()
