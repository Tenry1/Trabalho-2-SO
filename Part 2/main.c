#include "p2_simulator.h"
#include "inputs_part2.h"

int main() {
    // Array of pointers to all test cases
    int (*test_cases[])[20] = {
        input00, input01, input02, input03, input04,
        input05, input06, input07, input08, input09,
        input10, input11
    };

    // Number of test cases
    int num_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    // Run each test case
    for (int test_num = 0; test_num < num_test_cases; test_num++) {
        printf("\nRunning test case %d\n", test_num);

        // Initialize memory and scheduler
        MemoryManager mm;
        Scheduler sched;
        initialize_memory(&mm);
        initialize_scheduler(&sched);

        // Determine number of processes in this test case
        int num_processes = 0;
        while (test_cases[test_num][num_processes][0] != 0 && num_processes < 20) {
            num_processes++;
        }

        // Create processes from the test case
        for (int i = 0; i < num_processes; i++) {
            int address_space = test_cases[test_num][i][0];
            int program[MAX_PROGRAM_SIZE];
            int program_size = 0;

            // Extract program (skip the first element which is address space)
            for (int j = 1; j < 20 && test_cases[test_num][i][j] != 0; j++) {
                program[j-1] = test_cases[test_num][i][j];
                program_size++;
            }

            PCB* proc = create_process(i+1, address_space, program, program_size);
            sched.processes[sched.process_count++] = proc;
        }

        // Run simulation
        simulate(&sched, &mm);

        // Cleanup
        for (int i = 0; i < sched.process_count; i++) {
            free_process(sched.processes[i]);
        }
    }

    return 0;
}