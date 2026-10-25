#include <stdio.h>
#include <stdint.h>
/* Faithful transcription of 0x99aabc..0x99ac44 (FUN_0099a76c popcount-style chain). */
uint64_t chain(uint32_t w22, uint32_t sp64) {
    uint32_t w9, w8, w10, w11, w12;
    uint64_t x8, x9;
    __asm__ volatile(
        "ror    w9, %w2, #31\n\t"
        "lsr    w8, %w3, #1\n\t"
        "lsr    w10, %w2, #30\n\t"
        "bfi    w10, w9, #1, #31\n\t"
        "mvn    w11, %w2\n\t"
        "sub    w9, w11, w9\n\t"
        "lsr    w11, %w2, #29\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #28\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #27\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #26\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #25\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #24\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #23\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #22\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #21\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #20\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #19\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #18\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #17\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #16\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #15\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #14\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #13\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #12\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #11\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #10\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #9\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #8\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #7\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #6\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #5\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "lsr    w10, %w2, #4\n\t"
        "bfi    w10, w11, #1, #31\n\t"
        "sub    w9, w9, w11\n\t"
        "lsr    w11, %w2, #3\n\t"
        "bfi    w11, w10, #1, #31\n\t"
        "lsr    w12, %w2, #2\n\t"
        "bfi    w12, w11, #1, #31\n\t"
        "sub    w9, w9, w10\n\t"
        "sub    w9, w9, w11\n\t"
        "sub    w9, w9, w12\n\t"
        "bfi    w8, w12, #1, #31\n\t"
        "mvn    w8, w8\n\t"
        "add    w8, w9, w8\n\t"
        "sub    w8, w8, #14\n\t"
        "mov    x9, #1\n\t"
        "sub    x8, x9, x8\n\t"
        "mov    %0, x8\n\t"
        : "=&r"(x8)
        : "r"((uint64_t)0), "r"(w22), "r"(sp64)
        : "w9", "w8", "w10", "w11", "w12", "x9", "x8", "cc");
    return x8;
}
int main(void) {
    uint32_t tests[] = {0u, 1u, 3u, 0xFu, 0x7FFFFFFFu, 0xFFFFFFFFu,
                        0x12345678u, 0xDEADBEEFu, 0x80000000u};
    for (int i = 0; i < 9; i++) {
        uint64_t r = chain(tests[i], 0);
        printf("w22=%08x popc=%2d -> s64=%lld\n", tests[i], __builtin_popcount(tests[i]), (long long)r);
    }
    /* sp64 dependence, w22 fixed */
    uint32_t sp[] = {0u, 1u, 2u, 4u, 0xFFFFFFFFu};
    for (int i = 0; i < 5; i++)
        printf("sp64=%08x -> s64=%lld\n", sp[i], (long long)chain(0x12345678u, sp[i]));
    return 0;
}
