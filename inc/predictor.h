

typedef enum hint_e {
    NO_HINT,
    HINT_TAKEN,
    HINT_NOT_TAKEN
} hint_t;
typedef struct branch_s {
    uint64_t pc;
    bool taken;
    hint_t hint;
} branch_t;

typedef struct stats_s {
    uint64_t taken_count;
    uint64_t not_taken_count;
    uint64_t mispredict_count[2];
    uint64_t num_counters;
    uint8_t counter_size_in_bits;
    uint8_t history_length;
} stats_t;

typedef struct counter_s {
    uint8_t *counters;
    uint8_t max_value;
    uint8_t taken_threshold;
} counter_t;

typedef struct predictor_s {
    uint64_t pc_mask;
    uint64_t history_register;
    uint64_t history_mask;
    uint8_t pc_bits;
    uint8_t g_shared_bits;
    counter_t counters;
    stats_t stats;
} predictor_t;

void predictor__init(predictor_t *me, uint64_t num_counters, uint8_t counter_size_in_bits, uint8_t history_length, uint8_t g_shared_bits);

void predictor__reset(predictor_t* me);

void predictor__print_stats(predictor_t *me);

bool predictor__make_prediction(predictor_t *me, uint64_t pc, hint_t hint);

void predictor__update_stats(predictor_t *me, bool prediction, bool actual);

void predictor__update_predictor(predictor_t *me, uint64_t pc, bool taken);
