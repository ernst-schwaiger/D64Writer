cmake_minimum_required(VERSION 3.11)
project(D64Writer VERSION 0.0.1 LANGUAGES CXX)

#
# For getting Catch2 sources
#
Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Writer binary
#
add_executable(D64Writer
    src/main.cpp
    src/Writer.cpp
    )    

#
# Tests
#
add_executable(D64WriterTest 
    test/WriterTest.cpp
    )

target_include_directories(D64WriterTest PRIVATE 
    ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(D64WriterTest PRIVATE Catch2::Catch2WithMain)
add_test(NAME D64WriterTest COMMAND D64WriterTest)
