#include <stdio.h>
#include <stdlib.h>
#include "p2_simulator.h"
#include "inputs_part2.h"

int main() {
    // Define a struct locally to hold the test case data and its properties.
    // This avoids modifying any header files.
    typedef struct {
        int (*program_data)[20];
        int num_rows;
        const char* name;
    } TestCase;

    // Manually define all 12 test cases. This removes the need for NUM_TEST_CASES.
    TestCase all_tests[] = {
        {input00,  8, "00"},
        {input01,  6, "01"},
        {input02,  5, "02"},
        {input03,  6, "03"},
        {input04,  6, "04"},
        {input05,  6, "05"},
        {input06,  5, "06"},
        {input07, 12, "07"},
        {input08, 12, "08"},
        {input09, 12, "09"},
        {input10, 12, "10"},
        {input11, 12, "11"}
    };
    int num_tests = sizeof(all_tests) / sizeof(all_tests[0]);

    printf("Starting OS Simulation (Assignment 2, Part 2)...\n");

    for (int i = 0; i < num_tests; i++) {
        SimulationSystem system;
        char filename[30];
        sprintf(filename, "output2T%s.out", all_tests[i].name);

        // Redirect stdout to the output file
        FILE* output_file = freopen(filename, "w", stdout);
        if (output_file == NULL) {
            perror("Error: Could not open output file");
            continue;
        }

        // Run the simulation for the current test case
        initialize_system(&system, all_tests[i].program_data, all_tests[i].num_rows);
        run_simulation(&system);
        cleanup_system(&system);

        // Close the file
        fclose(output_file);
    }
    
    // Restore stdout for the final message
    #ifdef _WIN32
        freopen("CONOUT$", "w", stdout);
    #else
        freopen("/dev/tty", "w", stdout);
    #endif

    printf("Simulation complete. All %d test case files generated.\n", num_tests);

    return 0;
}