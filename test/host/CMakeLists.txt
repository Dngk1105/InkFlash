# Host test: biên dịch các tầng C thuần (domain, ui) bằng gcc trên PC và chạy bằng Unity.
# Đây cũng là rào chắn: domain/ui include header ESP-IDF hay FreeRTOS thì build ở đây sẽ lỗi.
#
# Chạy (từ thư mục gốc project):
#   cmake -S test/host -B build-host -G Ninja -DCMAKE_C_COMPILER=gcc
#   cmake --build build-host && ctest --test-dir build-host --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(inkflash_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

# Unity lấy từ ESP-IDF (biến môi trường IDF_PATH, mặc định bản cài v6.1)
if(DEFINED ENV{IDF_PATH})
    set(IDF_DIR $ENV{IDF_PATH})
else()
    set(IDF_DIR "C:/esp/v6.1/esp-idf")
endif()
set(UNITY_DIR ${IDF_DIR}/components/unity/unity/src)
add_library(unity STATIC ${UNITY_DIR}/unity.c)
target_include_directories(unity PUBLIC ${UNITY_DIR})
target_compile_options(unity PRIVATE -w)

set(COMP ${CMAKE_CURRENT_SOURCE_DIR}/../../components)

add_library(domain STATIC ${COMP}/domain/crc32.c ${COMP}/domain/review_log.c)
target_include_directories(domain PUBLIC ${COMP}/domain/include)

add_library(ui STATIC ${COMP}/ui/ink_canvas.c)
target_include_directories(ui PUBLIC ${COMP}/ui/include)

enable_testing()
foreach(t test_review_log test_canvas)
    add_executable(${t} ${t}.c)
    target_link_libraries(${t} unity domain ui m)
    add_test(NAME ${t} COMMAND ${t})
endforeach()
