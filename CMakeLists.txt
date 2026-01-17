cmake_minimum_required(VERSION 3.15)
project(barosavnc_vnc VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

file(GLOB SOURCES src/*.cpp)
add_executable(barosavnc ${SOURCES})

find_package(LibVNCServer REQUIRED)
find_package(WebP REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(ZLIB REQUIRED)
 find_package(OpenSSL REQUIRED)

if(MSVC)
    target_compile_options(barosavnc PRIVATE
        /std:c++latest /GR /utf-8
        $<$<CONFIG:Debug>:/Zi /Od /EHsc> 
        $<$<CONFIG:Release>:/O2 /DNDEBUG /EHs>
    )
else()
    target_compile_options(barosavnc PRIVATE
	-Wall -Wextra -fno-exceptions
	$<$<CONFIG:Debug>:-g -O1 -std=c++23>
	$<$<CONFIG:Release>:-O3 -DNDEBUG -fno-exceptions -std=c++23>
    )
endif()

target_link_libraries(barosavnc
    PRIVATE
        LibVNCServer::vncclient
	WebP::webp
	nlohmann_json::nlohmann_json
	CURL::libcurl
	Threads::Threads
        fmt::fmt-header-only
	ZLIB::ZLIB
	OpenSSL::SSL 
	OpenSSL::Crypto
)
