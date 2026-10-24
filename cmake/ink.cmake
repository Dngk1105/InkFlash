# Thiết kế: docs/design/code-structure.md
#
# Hàm CMake chung cho firmware, templates/ và host test. Mỗi luật viết một lần ở đây; CMakeLists của từng khối chỉ gọi lại.

# Gốc repo. Đặt ở đây (không phải trong macro) vì CMAKE_CURRENT_LIST_DIR trong macro là thư mục của nơi gọi.
get_filename_component(INK_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Cờ build chung, gọi sau include(project.cmake) và trước project().
# API deprecated và #warning thành lỗi build (docs/design/esp-idf-api.md, lớp 2).
macro(ink_project_flags)
    idf_build_set_property(COMPILE_OPTIONS "-Werror=deprecated-declarations" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "-Werror=cpp" APPEND)
endmacro()

# Gọi sau project(). Sau mỗi lần build, xoá cờ -m*/-f* khỏi compile_commands.json cho clangd
# (tools/fix_compile_commands.py giải thích lý do).
macro(ink_fix_clangd)
    add_custom_target(ink_fix_clangd ALL
        COMMAND ${PYTHON} ${INK_ROOT_DIR}/tools/fix_compile_commands.py ${CMAKE_BINARY_DIR}/compile_commands.json
        COMMENT "Cleaning compile_commands.json for clangd"
        VERBATIM)
endmacro()

# Thêm các thư mục chứa component vào EXTRA_COMPONENT_DIRS, bỏ qua thư mục chưa có.
macro(ink_extra_component_dirs)
    foreach(dir ${ARGN})
        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${dir}")
            list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/${dir}")
        endif()
    endforeach()
endmacro()

# Khối C thuần.
#   ink_core_component(SRCS a.c b.c [REQUIRES ink_khac])
# Trong ESP-IDF: đăng ký component như bình thường.
# Trên PC (host test): tạo thư viện tĩnh, biên dịch bằng gcc theo C23 (docs/design/code-structure.md, mục Chuẩn C).
# Include nhầm header ESP-IDF hay dùng phần mở rộng GNU là lỗi build.
# Là macro (không phải function) vì idf_component_register phải chạy trong scope của component.
macro(ink_core_component)
    cmake_parse_arguments(INK_CORE "" "" "SRCS;REQUIRES" ${ARGN})
    if(ESP_PLATFORM)
        idf_component_register(SRCS ${INK_CORE_SRCS} INCLUDE_DIRS "include" REQUIRES ${INK_CORE_REQUIRES})
    else()
        get_filename_component(INK_CORE_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)
        add_library(${INK_CORE_NAME} STATIC ${INK_CORE_SRCS})
        target_include_directories(${INK_CORE_NAME} PUBLIC include)
        target_link_libraries(${INK_CORE_NAME} PUBLIC ${INK_CORE_REQUIRES})
        target_compile_options(${INK_CORE_NAME} PRIVATE -Wall -Wextra -Werror -pedantic)
    endif()
endmacro()
