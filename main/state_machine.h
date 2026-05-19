#ifndef STATE_MACHINE_TASK_H //prevent double includes
#define STATE_MACHINE_TASK_H

#include "esp_log.h"

// Pin assignments for state machine

#define END_PAUSE_INPUT    GPIO_NUM_20  // board TP 33
#define FLOATING_PIN       1
#define GROUNDED_PIN       0


// State Machine Task  states
typedef enum {
    SM_State_Paused,
    SM_State_Acquiring,
    SM_State_Uploading
} SM_state_t;


void state_machine_init();
void state_machine_task(void*);

static volatile SM_state_t SM_state = SM_State_Paused;

#endif
