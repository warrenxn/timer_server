/*
 * @Description: 
 * @Autor: warren xu
 * @Date: 2024-12-25 15:39:43
 * @LastEditors: warren xu
 * @LastEditTime: 2025-02-05 14:06:29
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

typedef uint32_t TIMER_TYPE;                

typedef struct TIMER_INSTANCE{		
    uint8_t timer_status;
    TIMER_TYPE dead_time;
    TIMER_TYPE set_time;
    
    struct TIMER_INSTANCE* next;
    void (*callback)(void*);
    void* arg;
}TIMER_INSTANCE;

struct timer_server_config{
    TIMER_TYPE (*get_tick)(void);
    
    void (*enter_critical)(void);
    void (*exit_critical)(void);
};

void timer_system_init(struct timer_server_config* config);
void init_timer(TIMER_INSTANCE* timer);
uint8_t create_timer(TIMER_INSTANCE* timer_ptr, TIMER_TYPE set_time);
uint8_t create_timer_ex(TIMER_INSTANCE* timer_ptr, TIMER_TYPE set_time, uint8_t always_run, void (*callback)(void*), void* arg);
void deinit_timer(TIMER_INSTANCE* timer_ptr);
uint8_t is_timer_done(TIMER_INSTANCE* timer_ptr);
uint8_t is_timer_act(TIMER_INSTANCE* timer_ptr);
uint8_t is_timer_idle(TIMER_INSTANCE* timer_ptr);
void timer_process(void);

#endif
