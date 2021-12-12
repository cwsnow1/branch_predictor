#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "predictor.h"

void predictor__init(predictor_t *me, uint64_t num_counters, uint8_t counter_size_in_bits, uint8_t history_length, uint8_t g_shared_bits) {
    // Assert all values that need be non-zero are so
    assert(me);
    assert(counter_size_in_bits);
    assert(num_counters);
    assert(history_length >= g_shared_bits && "g shared bits cannot be greater than total history length");
    memset(me, 0, sizeof(predictor_t));
    me->g_shared_bits = g_shared_bits;
    // Calculate number of bits to index into num_counters
    uint64_t tmp = num_counters;
    uint8_t num_counters_bits = 0;
    for (; (tmp & 1) == 0; tmp >>= 1) {
        num_counters_bits++;
    }
    assert(tmp == 1 && "Number of counters must be a power of 2");
    me->stats.num_counters = num_counters;
    // Assert that gshare values make sense for given config
    assert(num_counters_bits >= g_shared_bits && "Number of g shared bits is higher than the number of counters supports");
    assert(num_counters_bits >= history_length && "g history length is longer than the number of counters supports");
    me->stats.history_length = history_length;
    // Calculate bitmasks
    me->pc_bits = num_counters_bits - (history_length - g_shared_bits);
    me->pc_mask = (1 << me->pc_bits) - 1;
    me->history_mask = (1 << history_length) - 1;
    // Allocate and initialize counters
    me->counters.counters = (uint8_t*) malloc(sizeof(uint8_t) * num_counters);
    if (me->counters.counters == NULL) {
        fprintf(stderr, "Unable to allocate memory for %lu counters\n", num_counters);
        exit(1);
    }
    memset(me->counters.counters, 0, sizeof(uint8_t) * num_counters);
    // Calculate and save counter info
    assert(counter_size_in_bits <= 8 && "Counters are too large to fit in uint8_t");
    me->stats.counter_size_in_bits = counter_size_in_bits;
    me->counters.max_value = (1 << counter_size_in_bits) - 1;
    me->counters.taken_threshold = 1 << (counter_size_in_bits - 1);
}

void predictor__reset(predictor_t *me) {
    if (me) {
        if (me->counters.counters) {
            free(me->counters.counters);
        }
        memset(me, 0, sizeof(predictor_t));
    }
}

void predictor__print_stats(predictor_t *me) {
    printf("Number of counters: %lu, number of bits in counter: %hhu\n", me->stats.num_counters, me->stats.counter_size_in_bits);
    printf("Global history register length: %hhu bit(s), %hhu shared bit(s)\n", me->stats.history_length, me->g_shared_bits);
    float mispredict_rate_not_taken = (float) me->stats.mispredict_count[0] / (float) me->stats.not_taken_count;
    float mispredict_rate_taken = (float) me->stats.mispredict_count[1] / (float) me->stats.taken_count;
    float total_mispredict_rate = (float) (me->stats.mispredict_count[0] + me->stats.mispredict_count[1]) / (float) (me->stats.taken_count + me->stats.not_taken_count);
    printf("Total mispredict rate = %02.1f%%\n", total_mispredict_rate * 100.0f);
    printf("Mispredict rate when branch is not taken = %02.1f%%\n", mispredict_rate_not_taken * 100.0f);
    printf("Mispredict rate when branch is taken = %02.1f%%\n", mispredict_rate_taken * 100.0f);
}

/**
 * @brief       Updates the global history of branch outcomes
 * 
 * @param me    Instance of predictor
 * @param taken Outcome of the last branch
 */
static inline void update_history(predictor_t *me, bool taken) {
    uint64_t new_value = taken ? 1 : 0; // unnecessary if bools are only 1 or 0, but I think true is any non zero value
    // shift left, OR in new value, cut off the MSB by ANDing the mask
    me->history_register = ((me->history_register << 1) | new_value) & me->history_mask;
}

/**
 * @brief       Converts a given PC to an index into the counters
 * 
 * @param me    Predictor instance
 * @param pc    Program counter of branch instruction
 * @return      Index into predictor's counters
 */
static inline uint64_t calculate_counter_index(predictor_t *me, uint64_t pc) {
    uint64_t g_history_component = me->history_register << me->pc_bits;
    uint64_t pc_component = pc & me->pc_mask;
    return g_history_component ^ pc_component;
}

bool predictor__make_prediction(predictor_t *me, uint64_t pc, hint_t hint) {
    if (hint == NO_HINT) {
        bool taken = false;
        uint64_t index = calculate_counter_index(me, pc);
        if (me->counters.counters[index] >= me->counters.taken_threshold) {
            taken = true;
        }
        return taken;
    } else {
        return hint == HINT_TAKEN;
    }
}

void predictor__update_stats(predictor_t *me, bool prediction, bool actual) {
    uint8_t index_to_stats = 0;
    if (actual) {
        index_to_stats = 1;
        me->stats.taken_count++;
    } else {
        me->stats.not_taken_count++;
    }
    if (prediction != actual) {
        me->stats.mispredict_count[index_to_stats]++;
    }
}

void predictor__update_predictor(predictor_t *me, uint64_t pc, bool taken) {
    uint64_t index = calculate_counter_index(me, pc); // NOTE: this assumes that the calculated index will be the same as when the prediction was made
    if (taken) {
        if (me->counters.counters[index] != me->counters.max_value) {
            me->counters.counters[index]++;
        }
    } else {
        if (me->counters.counters[index] != 0) {
            me->counters.counters[index]--;
        }
    }
    update_history(me, taken);
}
