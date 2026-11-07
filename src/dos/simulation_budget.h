#ifndef SIMCITY_DOS_SIMULATION_BUDGET_H
#define SIMCITY_DOS_SIMULATION_BUDGET_H
#include <stdint.h>
#include <stddef.h>

typedef enum sim_budget_result {
    SIM_BUDGET_INVALID=0, SIM_BUDGET_INTERACTIVE=1, SIM_BUDGET_AUTOMATIC=2
} sim_budget_result;

/* Original3489e preallocation and354a2 percentage arithmetic. annual1 means
 * an actual tax/funding settlement; annual0 is viewing the budget from a menu.
 * Memory is the shared flat original-address image, at least1MB.
 */
sim_budget_result sim_budget_prepare(uint8_t *memory,size_t size,int annual);
/* Initial allocation calculation only. Do not invoke on commit after a UI
 * slider has capped an allocation against available funds. */
int sim_budget_recalculate(uint8_t *memory,size_t size);
/* Preserve current UI allocation choices; settle only for annual1, and clear
 * original waiting flag4f86. UI cleanup/rendering remain caller-owned.
 */
int sim_budget_commit(uint8_t *memory,size_t size,int annual);
#endif
