# UI: C thuần, không biết phần cứng (canvas 1bpp đúng định dạng HAL màn). Test trên PC ở test/host.
idf_component_register(
    SRCS "ink_canvas.c"
    INCLUDE_DIRS "include"
)
