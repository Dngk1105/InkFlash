# Domain: C thuần (C11 + thư viện chuẩn). KHÔNG include header ESP-IDF/FreeRTOS.
# ESP-IDF tự thêm các component chung vào mọi component nên CMake không chặn được việc này;
# rào chắn thật là host test (test/host) biên dịch domain bằng gcc trên PC: include header ESP-IDF là lỗi.
idf_component_register(
    SRCS "crc32.c" "review_log.c"
    INCLUDE_DIRS "include"
)
