# Design: docs/design/figma-to-code.md
idf_component_register(SRCS "ink_app_foundations.c" "views/foundations_portrait_view.gen.c"
                            "views/foundations_landscape_view.gen.c"
                            "views/foundations_2_portrait_view.gen.c"
                            "views/foundations_2_landscape_view.gen.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_system
                       PRIV_REQUIRES ink_display ink_platform ink_ui)
