#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "predictor.h"

void predictor__init(predictor_t *me, uint64_t num_counters, uint8_t counter_size_in_bits) {
    assert(me);
    assert(counter_size_in_bits);
    assert(num_counters);
    memset(me, 0, sizeof(predictor_t));
    uint64_t tmp = num_counters;
    for (; (tmp & 1) == 0; tmp >>= 1)
        ;
    assert(tmp == 1 && "Number of counters must be a power of 2");
    me->counter_index_mask = num_counters - 1;
    me->counters.counters = (uint8_t*) malloc(sizeof(uint8_t) * num_counters);
    if (me->counters.counters == NULL) {
        fprintf(stderr, "Unable to allocate memory for %lu counters\n", num_counters);
        exit(1);
    }
    memset(me->counters.counters, 0, sizeof(uint8_t) * num_counters);
    me->counters.max_value = (1 << counter_size_in_bits) - 1;
    me->counters.taken_threshold = 1 << (counter_size_in_bits - 1);
    assert(counter_size_in_bits <= 8 && "Counters are too large to fit in uint8_t");
}

void predictor__print_stats(predictor_t *me) {
    float mispredict_rate_not_taken = (float) me->stats.mispredict_count[0] / (float) me->stats.not_taken_count;
    float mispredict_rate_taken = (float) me->stats.mispredict_count[1] / (float) me->stats.taken_count;
    float total_mispredict_rate = (float) (me->stats.mispredict_count[0] + me->stats.mispredict_count[1]) / (float) (me->stats.taken_count + me->stats.not_taken_count);
    printf("Total mispredict rate = %02.1f%%\n", total_mispredict_rate * 100.0f);
    printf("mispredict rate when branch is not taken = %02.1f%%\n", mispredict_rate_not_taken * 100.0f);
    printf("mispredict rate when branch is taken = %02.1f%%\n", mispredict_rate_taken * 100.0f);
}

bool predictor__make_prediction(predictor_t *me, uint64_t pc, hint_t hint) {
    if (hint == NO_HINT) {
        bool taken = false;
        uint64_t index = pc & me->counter_index_mask;
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
    uint64_t index = pc & me->counter_index_mask;
    if (taken) {
        if (me->counters.counters[index] != me->counters.max_value) {
            me->counters.counters[index]++;
        }
    } else {
        if (me->counters.counters[index] != 0) {
            me->counters.counters[index]--;
        }
    }
}
