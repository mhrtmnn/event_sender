#ifndef _event_sender
#define _event_sender

#include <stdbool.h>


/*******************************************************************************
* MACROS/DEFINES
*******************************************************************************/
#define JOY_NO_CHANGE -1
typedef enum _but_state_t{
	BUT_UP = 0,
	BUT_DOWN,
	BUT_KEEP
} but_state_t;

/*******************************************************************************
* DATA STRUCTURES
*******************************************************************************/
typedef struct
{
	int joy_x;
	int joy_y;
	but_state_t but_c;
	but_state_t but_z;
} nun_stat_t;


#endif /* _event_sender */