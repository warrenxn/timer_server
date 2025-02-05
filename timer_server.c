/*
 * @Description: 
 * @Autor: warren xu
 * @Date: 2024-12-25 15:39:43
 * @LastEditors: warren xu
 * @LastEditTime: 2025-02-05 16:57:14
 */

#include "timer_server.h"
#include <stdio.h>

#define TIMER_STATUS_IDLE                  0x00
#define TIMER_STATUS_ACT                   0x01
#define TIMER_STATUS_DONE_ONCE             0x02
#define TIMER_STATUS_RUN_MASK              0x0F
#define TIMER_STATUS_WAIT_OVER             0x10
#define TIMER_STATUS_ALWAYS_RUN            0x20

#define NUM_TIMER_CALLBACK                  5  

static TIMER_INSTANCE* timer_ptr = NULL;
static struct timer_server_config timer_api = {
    .get_tick = NULL,
    .enter_critical = NULL,
    .exit_critical = NULL
};

// static void remove_timer(TIMER_INSTANCE* timer) {
//     TIMER_INSTANCE* current = timer_ptr;
//     while(current){
//         if(current == timer){
//             current = timer->next;
//             break;
//         }
//         current = current->next;
//     }
// }

static uint8_t is_timer_contain(TIMER_INSTANCE* timer){
    TIMER_INSTANCE* current = timer_ptr;
    uint8_t is_contain = 0;
    if (timer){
        while(current){
            if(current == timer){
                is_contain = 1;
                break;
            }
            current = current->next;
        }
    }
    return is_contain;
}

static uint8_t add_timer(TIMER_INSTANCE* new_timer){
    uint8_t add_success = 0;
    if(new_timer){
        if(!is_timer_contain(new_timer)){
            TIMER_INSTANCE* current = timer_ptr;
            if(!current){
                timer_ptr = new_timer;
            }else{
                while (current->next){
                    current = current->next;
                }
                current->next = new_timer;
            }
            new_timer->next = NULL;
            add_success = 1;
        }
    }
    return add_success;
}

uint8_t create_timer(TIMER_INSTANCE* timer, TIMER_TYPE set_time){
    uint8_t creat_result = 0;
    if(timer && timer_api.get_tick){
        TIMER_TYPE current_tick = timer_api.get_tick();
        timer->dead_time = current_tick + set_time;
        timer->set_time = set_time;
        timer->timer_status = TIMER_STATUS_ACT;
        if (current_tick > timer->dead_time){
            timer->timer_status |= TIMER_STATUS_WAIT_OVER;
        }
        if (timer_api.enter_critical){
            timer_api.enter_critical();
        }
        add_timer(timer);
        if (timer_api.exit_critical){
            timer_api.exit_critical();
        }
        creat_result = 1;
    }
    return creat_result;
}

uint8_t create_timer_ex(TIMER_INSTANCE* timer, TIMER_TYPE set_time, uint8_t always_run, void (*callback)(void*), void* arg){
    uint8_t creat_result = 0;
    if(timer && timer_api.get_tick){
        TIMER_TYPE current_tick = timer_api.get_tick();
        timer->dead_time = current_tick + set_time;
        timer->set_time = set_time;
        timer->timer_status = TIMER_STATUS_ACT;
        timer->callback = callback;
        timer->arg = arg;
        if (current_tick > timer->dead_time){
            timer->timer_status |= TIMER_STATUS_WAIT_OVER;
        }
        if (always_run){
            timer->timer_status |= TIMER_STATUS_ALWAYS_RUN;
        }
        if (timer_api.enter_critical){
            timer_api.enter_critical();
        }
        add_timer(timer);
        if (timer_api.exit_critical){
            timer_api.exit_critical();
        }
        creat_result = 1;
    }
    return creat_result;
}

void init_timer(TIMER_INSTANCE* timer){
    if(timer){
        timer->timer_status = TIMER_STATUS_IDLE;
    }
}

void deinit_timer(TIMER_INSTANCE* timer){
    if(timer){
        timer->timer_status = TIMER_STATUS_IDLE;
    }
}

uint8_t is_timer_done(TIMER_INSTANCE* timer){
    uint8_t is_done = 0;
    if(timer){
        if(TIMER_STATUS_DONE_ONCE & (timer->timer_status)){
            is_done = 1;
        }
    }
    return is_done;
}

uint8_t is_timer_act(TIMER_INSTANCE* timer){
    uint8_t is_act = 0;
    if(timer){
        if(is_timer_contain(timer)){
            if(TIMER_STATUS_ACT & timer->timer_status){
                is_act = 1;
            }
        }
    }
    return is_act;
}

uint8_t is_timer_idle(TIMER_INSTANCE* timer){
    uint8_t is_idle = 1;
    if(timer){
        if(is_timer_contain(timer)){
            if(TIMER_STATUS_RUN_MASK & timer->timer_status){
                is_idle = 0;
            }
        }
    }
    return is_idle;
}

void timer_system_init(struct timer_server_config* config){
    timer_api = *config;
}

void timer_process(void){
    if (timer_api.get_tick){
        static TIMER_TYPE last_tick = 0;
        uint8_t is_tick_over = 0;
        TIMER_INSTANCE* current = timer_ptr;
        static struct{
            void (*callback)(void*);
            void* arg;
        }call_buf[NUM_TIMER_CALLBACK];
        uint8_t call_buf_index = 0;
        if (last_tick > timer_api.get_tick()){
            is_tick_over = 1;
        }
        last_tick = timer_api.get_tick();
        if (timer_api.enter_critical){
            timer_api.enter_critical();
        }
        while (current){
            if(TIMER_STATUS_ACT & current->timer_status){
                if (is_tick_over){
                    current->timer_status &= ~TIMER_STATUS_WAIT_OVER;
                }
                if(0 == (TIMER_STATUS_WAIT_OVER & current->timer_status)){
                    if (current->dead_time <= last_tick){
                        current->timer_status |= TIMER_STATUS_DONE_ONCE;
                        current->timer_status &= ~TIMER_STATUS_ACT;
                        if (current->callback){
                            if(call_buf_index < NUM_TIMER_CALLBACK){
                                call_buf[call_buf_index].callback = current->callback;
                                call_buf[call_buf_index].arg = current->arg;
                                call_buf_index++;
                            }
                            if (current->timer_status & TIMER_STATUS_ALWAYS_RUN){
                                current->timer_status |= TIMER_STATUS_ACT;
                                current->dead_time = last_tick + current->set_time;
                                if (last_tick > current->dead_time){
                                    current->timer_status |= TIMER_STATUS_WAIT_OVER;
                                }
                            }
                        }
                    }
                }
            }
            current = current->next;
        }
        if (timer_api.exit_critical){
            timer_api.exit_critical();
        }
        for (size_t i = 0; i < call_buf_index; i++){
            call_buf[i].callback(call_buf[i].arg);
        }
    }
}
