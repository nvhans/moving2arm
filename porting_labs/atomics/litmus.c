/* A race-free message-passing litmus. Every shared payload access is atomic.
 * Relaxed mode permits flag=1/data=0; it needn't reproduce that outcome.
 */
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    atomic_int start;
    atomic_int data;
    atomic_int flag;
    int synchronized;
    int seen_flag;
    int seen_data;
} trial;

static void wait_start(trial *state) {
    while (!atomic_load_explicit(&state->start, memory_order_acquire)) {
        sched_yield();
    }
}

static void *publish(void *arg) {
    trial *state = arg;
    wait_start(state);
    atomic_store_explicit(&state->data, 1, memory_order_relaxed);
    if (state->synchronized) {
        atomic_store_explicit(&state->flag, 1, memory_order_release);
    } else {
        atomic_store_explicit(&state->flag, 1, memory_order_relaxed);
    }
    return NULL;
}

static void *observe(void *arg) {
    trial *state = arg;
    wait_start(state);
    if (state->synchronized) {
        state->seen_flag = atomic_load_explicit(&state->flag, memory_order_acquire);
    } else {
        state->seen_flag = atomic_load_explicit(&state->flag, memory_order_relaxed);
    }
    state->seen_data = atomic_load_explicit(&state->data, memory_order_relaxed);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "relaxed") && strcmp(argv[1], "acquire-release"))) {
        fprintf(stderr, "Usage: %s relaxed|acquire-release\n", argv[0]);
        return 1;
    }
    const int synchronized = !strcmp(argv[1], "acquire-release");
    unsigned stale = 0;
    enum { TRIALS = 500 };
    for (unsigned i = 0; i < TRIALS; ++i) {
        trial state;
        atomic_init(&state.start, 0);
        atomic_init(&state.data, 0);
        atomic_init(&state.flag, 0);
        state.synchronized = synchronized;
        state.seen_flag = state.seen_data = -1;
        pthread_t writer, reader;
        if (pthread_create(&writer, NULL, publish, &state)) {
            return 1;
        }
        if (pthread_create(&reader, NULL, observe, &state)) {
            atomic_store_explicit(&state.start, 1, memory_order_release);
            pthread_join(writer, NULL);
            return 1;
        }
        atomic_store_explicit(&state.start, 1, memory_order_release);
        const int writer_join = pthread_join(writer, NULL);
        const int reader_join = pthread_join(reader, NULL);
        if (writer_join || reader_join) {
            return 1;
        }
        if ((state.seen_flag != 0 && state.seen_flag != 1) ||
            (state.seen_data != 0 && state.seen_data != 1)) {
            return 1;
        }
        stale += state.seen_flag == 1 && state.seen_data == 0;
    }
    printf("%s: flag=1/data=0 occurred %u times in %u trials.\n",
           argv[1], stale, (unsigned)TRIALS);
    if (!synchronized) {
        puts("Zero observations doesn't prove relaxed publication is correct.");
    }
    return synchronized && stale != 0;
}
