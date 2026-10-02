#ifndef TIMER_DISPATCHER_H_INCLUDED
#define TIMER_DISPATCHER_H_INCLUDED

#include  "stm32f0xx.h"
#include <stdio.h>
#include <stdbool.h>
//-----------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------
#define	TASK_QUEUE_SIZE		    20
#define TIMER_QUEUE_SIZE	    20
#define SYSTIMER_TICK           1000

#define CRITICAL_ALLOC()     uint32_t __status_reg
#define ENTER_CRITICAL()      do { __status_reg = __get_PRIMASK(); __disable_irq(); } while(0)
#define EXIT_CRITICAL()       do { __set_PRIMASK(__status_reg); } while(0)


//-----------------------------------------------------------------------
// Type
//-----------------------------------------------------------------------
typedef void (*tptr)(void);

//-----------------------------------------------------------------------
// Prototype
//-----------------------------------------------------------------------
extern void td_idle(void);
extern void td_init_rtos_clock(void);

void td_init_rtos(void);
void td_set_task(tptr ts);
void td_set_timer_task(tptr ts, uint32_t new_time);
void td_task_manager(void);
void td_timer_service(void);
void td_time_handler(void);
bool timer_is_expired (const uint32_t timer);
uint32_t main_timer_set(const uint32_t add_time);
uint32_t main_timer_get_tick(void);

#endif /* TIMER_DISPATCHER_H_INCLUDED */
