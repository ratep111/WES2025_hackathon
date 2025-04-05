SET(SOURCES screens/ui_scr1.c
    screens/ui_scr2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c)

add_library(ui ${SOURCES})
