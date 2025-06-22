#include <stdio.h>
#include "p1_simulator.h"
#include "inputs_part1.h"

int main() {
    struct TestCase {
        int num_procs;
        int* mem_sizes;
        int* exec_trace;
        int trace_len;
    };

    struct TestCase all_tests[] = {
        {5,  inputP1Mem00, inputP1Exec00, 12},
        {5,  inputP1Mem01, inputP1Exec01, 6},
        {5,  inputP1Mem02, inputP1Exec02, 9},
        {10, inputP1Mem03, inputP1Exec03, 9},
        {20, inputP1Mem04, inputP1Exec04, 35},
        {3,  inputP1Mem05, inputP1Exec05, 18}
    };
    int num_tests = sizeof(all_tests) / sizeof(all_tests[0]);

    for (int i = 0; i < num_tests; i++) {
        char filename[20];
        struct TestCase current_test = all_tests[i];

        sprintf(filename, "fifo%02d.out", i);
        freopen(filename, "w", stdout);
        print_header(current_test.num_procs);
        run_simulation_logic(FIFO, current_test.num_procs, current_test.mem_sizes, current_test.exec_trace, current_test.trace_len);
        fclose(stdout);

        sprintf(filename, "lru%02d.out", i);
        freopen(filename, "w", stdout);
        print_header(current_test.num_procs);
        run_simulation_logic(LRU, current_test.num_procs, current_test.mem_sizes, current_test.exec_trace, current_test.trace_len);
        fclose(stdout);
    }
    
    #ifdef _WIN32
        freopen("CONOUT$", "w", stdout);
    #else
        freopen("/dev/tty", "w", stdout);
    #endif
    printf("Generated output files for %d test cases.\n", num_tests);
    
    return 0;
}