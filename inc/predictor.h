
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
    uint8_t  counter_size_in_bits;
    uint8_t  history_length;
} stats_t;

typedef struct counter_s {
    uint8_t *counters;
    uint8_t  max_value;
    uint8_t  taken_threshold;
} counter_t;

typedef struct predictor_s {
    uint64_t    pc_mask;
    uint64_t    history_register;
    uint64_t    history_mask;
    uint8_t     pc_bits;
    uint8_t     g_shared_bits;
    counter_t   counters;
    stats_t     stats;
} predictor_t;

/**
 * @brief Initializes the branch predictor structure
 * 
 * @param me                    Instance of a branch predictor
 * @param num_counters          Number of x bit counters that will give the branch prediction
 * @param counter_size_in_bits  Number of bits in a single counter
 * @param history_length        Number of previous branches to keep track of for making predictions
 * @param g_shared_bits         Number of the history bits to XOR into the PC to index into the counters
 */
void predictor__init(predictor_t *me, uint64_t num_counters, uint8_t counter_size_in_bits, uint8_t history_length, uint8_t g_shared_bits);

/**
 * @brief       Resets the predictor structure and frees any memory it has allocated
 * 
 * @param me    Instance of branch predictor
 */
void predictor__reset(predictor_t* me);

/**
 * @brief       Print the configuration and statistics gathered over a simulation for the given predictor instance
 * 
 * @param me    Branch predictor instance
 */
void predictor__print_stats(predictor_t *me);

/**
 * @brief       Gives a prediction for the given branch
 * 
 * @param me    Predictor instance
 * @param pc    Program counter for the branch instruction
 * @param hint  Compiler provided hint. Can be none
 * @return      Prediction for whether the branch is taken or not
 */
bool predictor__make_prediction(predictor_t *me, uint64_t pc, hint_t hint);

/**
 * @brief               Update the tracked statistics
 * 
 * @param me            Predictor instance
 * @param prediction    What the predictor as given by predictor
 * @param actual        Actual outcome as given by the tracefile
 */
void predictor__update_stats(predictor_t *me, bool prediction, bool actual);

/**
 * @brief       Updates the predictor's data after the outcome of a branch is known
 * 
 * @param me    Predictor instance
 * @param pc    Program counter of branch instruction
 * @param taken Whether the branch was actually taken
 */
void predictor__update_predictor(predictor_t *me, uint64_t pc, bool taken);
