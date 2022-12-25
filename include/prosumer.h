#define NUM_SUBPARA 1024

#define INLINE_PRODUCER_F   static inline void 

typedef void (*fptr_t)(void);

typedef struct Producer {   
    fifo_s buffer;
    fptr_t fptr;
    struct async_sem *self_signalee, *consumer_signaler;
} producer;