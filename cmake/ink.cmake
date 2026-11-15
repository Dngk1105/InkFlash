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

# Component bọc mã nguồn ngoài nằm trong third_party/ (docs/design/code-structure.md, mục Luật phụ thuộc).
#   ink_third_party_component(SRCS ink_ten.c
#                             THIRD_PARTY_DIR third_party/ten/src
#                             THIRD_PARTY_SRCS a.c b.c
#                             [DEFINITIONS TEN=1] [REQUIRES ink_khac])
# SRCS là code của repo, biên dịch như mọi khối. THIRD_PARTY_SRCS là tên file trong THIRD_PARTY_DIR (tính từ gốc repo):
# biên dịch theo gnu11 và tắt cảnh báo, vì mã ngoài không viết theo chuẩn của repo. DEFINITIONS áp cho cả khối dùng
# header của mã ngoài. Trên PC, THIRD_PARTY_DIR là thư mục include dạng SYSTEM để -pedantic của khối gọi không soi vào.
macro(ink_third_party_component)
    cmake_parse_arguments(INK_TP "" "THIRD_PARTY_DIR" "SRCS;THIRD_PARTY_SRCS;DEFINITIONS;REQUIRES" ${ARGN})
    set(INK_TP_DIR "${INK_ROOT_DIR}/${INK_TP_THIRD_PARTY_DIR}")
    if(NOT EXISTS "${INK_TP_DIR}")
        message(FATAL_ERROR "Thiếu ${INK_TP_DIR}: chạy git submodule update --init")
    endif()
    set(INK_TP_PATHS "")
    foreach(src ${INK_TP_THIRD_PARTY_SRCS})
        list(APPEND INK_TP_PATHS "${INK_TP_DIR}/${src}")
    endforeach()
    if(ESP_PLATFORM)
        idf_component_register(SRCS ${INK_TP_SRCS} ${INK_TP_PATHS}
                               INCLUDE_DIRS "include" "${INK_TP_DIR}"
                               REQUIRES ${INK_TP_REQUIRES})
        target_compile_definitions(${COMPONENT_LIB} PUBLIC ${INK_TP_DEFINITIONS})
    else()
        get_filename_component(INK_TP_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)
        add_library(${INK_TP_NAME} STATIC ${INK_TP_SRCS} ${INK_TP_PATHS})
        target_include_directories(${INK_TP_NAME} PUBLIC include)
        target_include_directories(${INK_TP_NAME} SYSTEM PUBLIC "${INK_TP_DIR}")
        target_compile_definitions(${INK_TP_NAME} PUBLIC ${INK_TP_DEFINITIONS})
        target_link_libraries(${INK_TP_NAME} PUBLIC ${INK_TP_REQUIRES})
        target_compile_options(${INK_TP_NAME} PRIVATE -Wall -Wextra -Werror)
    endif()
    # Sau idf_component_register: lúc ESP-IDF đọc CMakeLists ở chế độ script để lấy REQUIRES, lệnh này không chạy được.
    set_source_files_properties(${INK_TP_PATHS} PROPERTIES COMPILE_OPTIONS "-std=gnu11;-w")
endmacro()
