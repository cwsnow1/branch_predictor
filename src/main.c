#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "predictor.h"

#define FILE_LINE_LENGTH_IN_BYTES   (30)
#define PC_TO_TAKEN                 (8)
#define TAKEN_TO_HINT               (8)

static uint8_t * read_in_file (const char* filename, uint64_t *length) {
    assert(length);

    uint8_t *buffer = NULL;

    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        goto error;
    }

    if (fseek(f, 0, SEEK_END)) {
        fprintf(stderr, "Error in file seek of %s\n", filename);
        goto error;
    }
    long m = ftell(f);
    if (m < 0) goto error;
    buffer = (uint8_t *) malloc(m);
    if (buffer == NULL)                 goto error;
    if (fseek(f, 0, SEEK_SET))          goto error;
    if (fread(buffer, 1, m, f) != m)    goto error;
    fclose(f);

    *length = (uint64_t) m;
    return buffer;

error:
    if (f) fclose(f);
    if (buffer) free(buffer);
    fprintf(stderr, "Error in reading file %s\n", filename);
    exit(1);
}

void parse_line (uint8_t *line, uint64_t *pc, bool *taken, hint_t *hint) {
    char *end = NULL;
    *pc = strtoul((char*) line, &end, 16);
    assert(*end == ':');
    line = (uint8_t*) end + PC_TO_TAKEN;
    *taken = *line == 'T';
    line += TAKEN_TO_HINT;
    *hint = (hint_t) (*line - '0');
}

/**
 *  File content format is as follows:
 *  PC: taken T/F
 */

branch_t * parse_file_contents(uint8_t * buffer, uint64_t num_branches) {
    branch_t *branches = (branch_t*) malloc(sizeof(branch_t) * num_branches);
    if (branches == NULL) {
        fprintf(stderr, "malloc failed. %luB may be too much memory\n", num_branches * FILE_LINE_LENGTH_IN_BYTES);
    }
    uint8_t * buffer_start = buffer;
    for (uint64_t i = 0; i < num_branches; i++, buffer += FILE_LINE_LENGTH_IN_BYTES) {
        parse_line(buffer, &branches[i].pc, &branches[i].taken, &branches[i].hint);
    }
    free(buffer_start);
    return branches;
}


void usage (void) {
    fprintf(stderr, "Usage: ./predictor tracefile\n");
}


int main (int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        usage();
    }
    uint64_t file_length = 0;
    clock_t t = clock();
    uint8_t *buffer = read_in_file(argv[1], &file_length);
    assert(file_length);
    uint64_t num_branches = file_length / FILE_LINE_LENGTH_IN_BYTES;
    branch_t *branches = parse_file_contents(buffer, num_branches);
    t = clock() - t;
    double elapsed_time = (double) t / CLOCKS_PER_SEC;
    printf("File reading and parsing took %.4f seconds\n", elapsed_time);
    predictor_t predictor;
    t = clock();
    predictor__init(&predictor, 512, 2, 2, 2);
    for (uint64_t i = 0; i < num_branches; i++) {
        bool taken = predictor__make_prediction(&predictor, branches[i].pc, branches[i].hint);
        predictor__update_stats(&predictor, taken, branches[i].taken);
        predictor__update_predictor(&predictor, branches[i].pc, branches[i].taken);
    }
    t = clock() - t;
    predictor__print_stats(&predictor);
    predictor__reset(&predictor);
    elapsed_time = (double) t / CLOCKS_PER_SEC;
    printf("Simulation took %.4f seconds\n", elapsed_time);

    free(branches);
    return 0;
}
