/* Compile this launcher for baseline AArch64, without SVE-specific flags. */
#include <errno.h>
#include <linux/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <vector-bits> <program> [arguments...]\n", argv[0]);
        return 1;
    }
    char *end;
    const long bits = strtol(argv[1], &end, 10);
    if (*end || bits < 128 || bits > 2048 || bits % 128) {
        fprintf(stderr, "Vector length must be a multiple of 128 in 128..2048\n");
        return 1;
    }
    const unsigned long bytes = (unsigned long)bits / 8;
    const int selected = prctl(PR_SVE_SET_VL, bytes | PR_SVE_VL_INHERIT, 0UL, 0UL, 0UL);
    if (selected < 0) {
        if (errno == EINVAL) {
            fprintf(stderr, "Skipping: SVE vector length selection unavailable\n");
            return 77;
        }
        perror("PR_SVE_SET_VL");
        return 1;
    }
    if (((unsigned long)selected & PR_SVE_VL_LEN_MASK) != bytes) {
        fprintf(stderr, "Skipping: %ld-bit SVE vector length unsupported\n", bits);
        return 77;
    }
    execv(argv[2], argv + 2);
    perror("execv");
    return 1;
}
