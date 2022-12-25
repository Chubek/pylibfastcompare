#define NUM_ASYNC 1024

#define FUNC_PRO(n)     static inline async n(void *ptr) {\
                            printf("PRO");\
                        }

#define FUNC_CON(n)    static inline async n(void *ptr) {\
                            printf("CON");\
                        }

#define FUNC_DRV(n)     static inline async n(void *ptr) {\
                            printf("DRV");\
                        }     

#define FUNC_PRM(n)     static inline async n(void *ptr) {\
                            printf("PRM");\
                        }


typedef async (*fptr_t)(void);

typedef struct Driver {
    fptr_t func_producer, func_consumer, func_promise, func_driver;
    struct async_sem signal_producer, signal_consumer, signal_promise;
    struct async producer_pt, consumer_pt, driver_pt, promise_pt;
    fifo_s buffer;
 } driver_s;

void init_driver(driver_s *self);
void add_to_buffer(driver_s *self);

