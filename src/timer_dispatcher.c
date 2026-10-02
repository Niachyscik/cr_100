#include "timer_dispatcher.h"
#include <stdio.h>
#include <stdbool.h>
//-----------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------
volatile static tptr	task_queue[TASK_QUEUE_SIZE];
volatile static struct
{
    tptr go_to_task;
    uint32_t time;
}
main_timer[TIMER_QUEUE_SIZE];
volatile static uint32_t tick=0;

//-----------------------------------------------------------------------
// Interrupt
//-----------------------------------------------------------------------
void td_time_handler()
{
    tick++;
    td_timer_service();
}

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
uint32_t main_timer_get_tick(void)
{
    return tick;
}

//-------------------------------------------------------------------------
bool timer_is_expired (const uint32_t timer)
{
    uint32_t time_tick;
    time_tick = tick;
    return ((time_tick - timer) < (1UL << 31));
}

//-------------------------------------------------------------------------
uint32_t main_timer_set(const uint32_t add_time)
{
    uint32_t time_tick;
    time_tick = tick;
    return time_tick + add_time;
}

//-----------------------------------------------------------------------
void td_init_rtos(void)
{
    uint32_t	index;
    for(index=0; index<TASK_QUEUE_SIZE; index++)
    {
        task_queue[index] = td_idle;
    }
    for(index=0; index<TIMER_QUEUE_SIZE; index++)
    {
        main_timer[index].go_to_task = td_idle;
        main_timer[index].time = 0;
    }
    td_init_rtos_clock();

}

//-----------------------------------------------------------------------
__weak void td_idle(void)
{

}

//-----------------------------------------------------------------------
void td_set_task(tptr ts)
{
    uint32_t index = 0;

    CRITICAL_ALLOC();
    ENTER_CRITICAL();
    while (index < TASK_QUEUE_SIZE)
    {
        if (task_queue[index] == td_idle)
        {
            task_queue[index] = ts;
            break;
        }
        index++;
    }
    EXIT_CRITICAL();
}

//-----------------------------------------------------------------------
void td_set_timer_task(tptr ts, uint32_t new_time)
{
    uint32_t index = 0;

    CRITICAL_ALLOC();
    ENTER_CRITICAL();

    for(index = 0; index < TIMER_QUEUE_SIZE; ++index)
    {
        if(main_timer[index].go_to_task == ts)
        {
            main_timer[index].time = new_time;
            EXIT_CRITICAL();
            return;
        }
    }
    for(index = 0; index < TIMER_QUEUE_SIZE; ++index)
    {
        if (main_timer[index].go_to_task == td_idle)
        {
            main_timer[index].go_to_task = ts;
            main_timer[index].time = new_time;
            EXIT_CRITICAL();
            return;
        }
    }
    EXIT_CRITICAL();
}

//-----------------------------------------------------------------------
void td_task_manager(void)
{
    uint32_t index = 0;
    tptr go_to_task = td_idle;

    CRITICAL_ALLOC();
    ENTER_CRITICAL();
    go_to_task = task_queue[0];
    if (go_to_task == td_idle)
    {
        EXIT_CRITICAL();
        (td_idle)();
    }
    else
    {
        for(index = 0; index < (TASK_QUEUE_SIZE - 1); index++)
        {
            task_queue[index] = task_queue[index + 1];
        }
        task_queue[TASK_QUEUE_SIZE - 1] = td_idle;
        EXIT_CRITICAL();
        (go_to_task)();
    }
}

//-----------------------------------------------------------------------
void td_timer_service(void)
{
    uint32_t index;

    for(index = 0; index < TIMER_QUEUE_SIZE; index++)
    {
        if(main_timer[index].go_to_task == td_idle) continue;
        if(main_timer[index].time > 0)
        {
            main_timer[index].time--;
            if(main_timer[index].time == 0)
            {
                td_set_task(main_timer[index].go_to_task);
                main_timer[index].go_to_task = td_idle;
            }
        }
    }
}

