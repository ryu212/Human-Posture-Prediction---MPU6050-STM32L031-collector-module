#ifndef INC_STATE_CONTROL_H_
#define INC_STATE_CONTROL_H_

#include "main.h"


/* =========================
 * STATE DEFINE
 * ========================= */
typedef enum
{
    STATE_INIT = 0,
    STATE_START,
    STATE_IDLE,
    STATE_DATA,
    STATE_WRITE_SD,
    STATE_STOP

} system_state_t;

typedef struct system_input_t
{
	int8_t SW3;
	int8_t data_flag;
}system_input_t;
void init_state(system_state_t *system_state, system_input_t *system_input);
void change_state(system_state_t *system_state, system_input_t *system_input);
#endif /* INC_STATE_CONTROL_H_ */
