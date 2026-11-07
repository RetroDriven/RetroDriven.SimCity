#ifndef SIMCITY_DOS_SIMULATION_H
#define SIMCITY_DOS_SIMULATION_H
#include <stdint.h>
#include <stddef.h>

#define SIM_DOS_MEMORY_SIZE 0x100000u
#define SIM_DOS_DGROUP 0x4d2c0u

typedef enum sim_run_result {
    SIM_RUN_RETURNED=0, SIM_RUN_BUDGET=1, SIM_RUN_EXTERNAL=2,
    SIM_RUN_FAULT=3
} sim_run_result;

/* Exact flat byte backing retains original far-array aliases and unknown
 * state. Code is ahead-of-time translated to C; memory contains the original
 * initialized data and is not fetched as runtime opcodes by the translated
 * executor. The full original image also supplies embedded CS lookup tables.
 */
typedef struct sim_simulation {
    uint8_t memory[SIM_DOS_MEMORY_SIZE];
    uint16_t r[8]; /* AX,CX,DX,BX,SP,BP,SI,DI */
    uint16_t segment[4]; /* ES,CS,SS,DS */
    uint16_t flags;
    uint32_t pc, fault_pc;
    uint32_t clock_ticks;
    uint32_t clock_last;
    uint32_t steps;
    int running;
} sim_simulation;

typedef struct sim_cpu_context {
    uint16_t r[8],segment[4],flags;
    uint32_t pc,fault_pc;
    int running;
} sim_cpu_context;

size_t sim_simulation_sizeof(void);
int sim_simulation_init(sim_simulation *sim,const uint8_t *exe,size_t size);
uint8_t *sim_simulation_dgroup(sim_simulation *sim);
uint8_t *sim_simulation_memory(sim_simulation *sim);
/* Start a single translated original procedure with an isolated call stack.
 * near_body selects a near RET versus far RETF entry. No arguments are added.
 * Only translated procedures are supported; unknown external calls stop
 * explicitly, retaining registers/stack/PC for a platform event adapter.
 */
sim_run_result sim_simulation_call(sim_simulation *sim,uint32_t address,
                                  int near_body,uint32_t budget);
sim_run_result sim_simulation_resume(sim_simulation *sim,uint32_t budget);
/* Platform adapters can complete an external far call after servicing it. */
void sim_simulation_return_far(sim_simulation *sim,unsigned pop_bytes);
/* Secondary original routines (sprite updates/disaster commands) can share
 * the same memory while preserving the suspended core's CPU context. Caller
 * must finish/service that routine before resuming the primary core. Use a
 * separate original-DGROUP stack top (e.g0xe000); never reuse core's0xff00.
 */
void sim_simulation_cpu_save(const sim_simulation *sim,sim_cpu_context *context);
void sim_simulation_cpu_restore(sim_simulation *sim,const sim_cpu_context *context);
int sim_simulation_begin_aux(sim_simulation *sim,uint32_t address,
                              uint16_t code_segment,uint16_t stack_top,
                              const uint16_t *arguments,size_t argument_count);
/* Zero-argument near procedure, e.g original disaster menu entrypoints. */
int sim_simulation_begin_aux_near(sim_simulation *sim,uint32_t address,
                                   uint16_t code_segment,uint16_t stack_top);
#endif
