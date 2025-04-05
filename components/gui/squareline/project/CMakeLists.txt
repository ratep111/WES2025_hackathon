SET(SOURCES screens/ui_scr1.c
    screens/ui_scr2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_1148992643.c
    images/ui_img_sun_rays_small_cloud_png.c
    images/ui_img_car2_mini_png.c)

add_library(ui ${SOURCES})
