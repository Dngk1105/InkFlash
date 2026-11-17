# Design: docs/design/app-framework.md
idf_component_register(SRCS "ink_app_example.c" "views/example_view.gen.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_system
                       PRIV_REQUIRES ink_display ink_platform ink_ui)
