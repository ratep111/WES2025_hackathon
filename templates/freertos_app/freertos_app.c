/**
 * @file freertos_app.c
 *
 * @brief Example FreeRTOS application task using event queue.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "freertos_app.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

//---------------------------------- MACROS -----------------------------------
#define TAG "FREERTOS_APP"

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static void _freertos_app_task(void *params);
static void _handle_event(app_event_t event);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static QueueHandle_t s_app_event_queue;
static TaskHandle_t s_app_task_handle;

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------
void freertos_app_init(void) {
    s_app_event_queue = xQueueCreate(10, sizeof(app_event_t));
    xTaskCreate(_freertos_app_task, "freertos_app_task", 2048, NULL, 5, &s_app_task_handle);
}

void freertos_app_send_event(app_event_t event) {
    if(s_app_event_queue != NULL) {
        xQueueSend(s_app_event_queue, &event, 0);
    }
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------
static void _freertos_app_task(void *params) {
    app_event_t event;
    while(1) {
        if(xQueueReceive(s_app_event_queue, &event, portMAX_DELAY)) {
            _handle_event(event);
        }
    }
}

static void _handle_event(app_event_t event) {
    switch(event) {
        case APP_EVENT_EXAMPLE:
            printf("Handled example event\n");
            break;
        default:
            break;
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------