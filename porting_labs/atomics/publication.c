#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

enum { ITERATIONS = 20000 };
typedef struct {
    atomic_int ready;
    atomic_int abort_requested;
    /* Non-atomic data is safe only because both directions synchronize. */
    uint32_t sequence;
    uint32_t checksum;
    unsigned failures;
} channel;

static int wait_for(channel *state, int value) {
    while (atomic_load_explicit(&state->ready, memory_order_acquire) != value) {
        if (atomic_load_explicit(&state->abort_requested, memory_order_relaxed)) {
            return 0;
        }
        sched_yield();
    }
    return 1;
}

static void *producer(void *arg) {
    channel *state = arg;
    for (uint32_t i = 1; i <= ITERATIONS; ++i) {
        /* The consumer's release of ready=0 protects reuse of the payload. */
        if (!wait_for(state, 0)) {
            return NULL;
        }
        state->sequence = i;
        state->checksum = i ^ UINT32_C(0xa5a5a5a5);
        atomic_store_explicit(&state->ready, 1, memory_order_release);
    }
    return NULL;
}

static void *consumer(void *arg) {
    channel *state = arg;
    for (uint32_t i = 1; i <= ITERATIONS; ++i) {
        if (!wait_for(state, 1)) {
            return NULL;
        }
        if (state->sequence != i || state->checksum != (i ^ UINT32_C(0xa5a5a5a5))) {
            ++state->failures;
        }
        atomic_store_explicit(&state->ready, 0, memory_order_release);
    }
    return NULL;
}

int main(void) {
    channel state;
    atomic_init(&state.ready, 0);
    atomic_init(&state.abort_requested, 0);
    state.sequence = state.checksum = state.failures = 0;
    pthread_t reader, writer;
    if (pthread_create(&reader, NULL, consumer, &state)) {
        fputs("Couldn't create consumer\n", stderr);
        return 1;
    }
    if (pthread_create(&writer, NULL, producer, &state)) {
        atomic_store_explicit(&state.abort_requested, 1, memory_order_relaxed);
        pthread_join(reader, NULL);
        fputs("Couldn't create producer\n", stderr);
        return 1;
    }
    const int writer_join = pthread_join(writer, NULL);
    const int reader_join = pthread_join(reader, NULL);
    if (writer_join || reader_join) {
        return 1;
    }
    printf("Published %u messages with acquire/release; failures: %u\n",
           (unsigned)ITERATIONS, state.failures);
    return state.failures != 0;
}
