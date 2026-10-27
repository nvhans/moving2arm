/* Deliberately invalid C: opt-in sanitizer demonstration, never a normal test. */
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    alignas(16) unsigned char storage[16] = {0, 1, 2, 3, 4};
    /* A processor accepting this access doesn't establish C alignment or an
     * object of the required effective type. UBSan should stop at this load.
     */
    const uint32_t *wrong = (const uint32_t *)(storage + 1);
    printf("Unsafe value: %u\n", (unsigned)*wrong);
    return 0;
}
