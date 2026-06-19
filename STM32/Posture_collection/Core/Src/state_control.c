#include "state_control.h"


void init_state(
		system_state_t *system_state,
		system_input_t *system_input
)
{
	*system_state = STATE_INIT;

	system_input->SW3 = 0;
	system_input->data_flag = 0;
}

void change_state(
		system_state_t *system_state,
		system_input_t *system_input
)
{

	switch(*system_state)
	{

		case STATE_INIT:

			if(system_input->SW3)
			{
				*system_state = STATE_START;
			}

			break;
		case STATE_START:
			*system_state = STATE_IDLE;
			break;
		case STATE_IDLE:
			if(system_input->SW3 == 0)
			{
				*system_state = STATE_STOP;
			}
			else if(system_input->data_flag)
			{
				*system_state = STATE_DATA;
			}
			break;


		case STATE_DATA:
			system_input->data_flag = 0;
			*system_state = STATE_WRITE_SD;
			break;


		case STATE_WRITE_SD:
			*system_state = STATE_IDLE;
			break;


		case STATE_STOP:
			break;


		default:
			*system_state = STATE_INIT;
			break;
	}
}
