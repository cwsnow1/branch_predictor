

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
