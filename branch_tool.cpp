/*
 *  This file contains an ISA-portable PIN tool for tracing branches.
 */

#include <stdio.h>
#include "pin.H"

FILE* trace;

VOID* branch_ip;

enum taken_hint_t {
    NO_HINT,
    HINT_TAKEN,
    HINT_NOT_TAKEN,
};
taken_hint_t taken_hint;

VOID RecordBranch(VOID* ip) {
    branch_ip = ip;
}

VOID RecordPrevBranch(VOID* ip) {
    long unsigned this_addr   = reinterpret_cast<long unsigned>(ip);
    long unsigned branch_addr = reinterpret_cast<long unsigned>(branch_ip);
    bool taken = false;
    if (this_addr < branch_addr || this_addr - branch_addr != 2) {
        taken = true;
    }
    char taken_char = taken ? 'T' : 'F';
    fprintf(trace, "%p: taken %c, hint %d\n", branch_ip, taken_char, static_cast<int>(taken_hint));
}

// Is called for every instruction and records branches and instruction immediately after branches
VOID Instruction(INS ins, VOID* v)
{
    static bool was_last_instruction_branch = false;
    if (was_last_instruction_branch) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordPrevBranch, IARG_INST_PTR, IARG_END);
    }
    if (INS_IsBranch(ins)) {
        taken_hint = NO_HINT;
        if (INS_BranchTakenPrefix(ins)) {
            taken_hint = HINT_TAKEN;
        } else if (INS_BranchNotTakenPrefix(ins)) {
            taken_hint = HINT_NOT_TAKEN;
        }
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordBranch, IARG_INST_PTR, IARG_END);
        was_last_instruction_branch = true;
    } else {
        was_last_instruction_branch = false;
    }
}

VOID Fini(INT32 code, VOID* v)
{
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of branches\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("branch.out", "w");

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
