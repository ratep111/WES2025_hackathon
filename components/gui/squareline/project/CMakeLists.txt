SET(SOURCES screens/ui_scr1.c
    screens/ui_scr2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_1148992643.c
    images/ui_img_sun_rays_small_cloud_png.c
    images/ui_img_1040273307.c
    images/ui_img_play_png.c
    images/ui_img_forward_png.c
    images/ui_img_backward_png.c
    images/ui_img_car2_mini_png.c
    images/ui_img_474709716.c
    images/ui_img_temper_png.c
    images/ui_img_weather_1_png.c
    images/ui_img_360038530.c
    images/ui_img_settings_png.c
    images/ui_img_weather_2_png.c)

add_library(ui ${SOURCES})
