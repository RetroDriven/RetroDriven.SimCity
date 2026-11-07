#ifndef SIMCITY_DOS_CITY_STATS_H
#define SIMCITY_DOS_CITY_STATS_H
#include <stddef.h>
#include <stdint.h>

/* Exact original3560e signed currency prefix and35564 comma formatter.
 * Output requires up to17 bytes including NUL; undersized output is cleared.
 */
int sim_city_stats_money(char *output,size_t capacity,int32_t funds);
/* Original302d4 strips the first currency character if the full string is
 * longer than10, then applies "Funds:%-9s" (minimum width, no truncation).
 * This preserves the original large-negative-balance display quirk. */
int sim_city_stats_funds_label(char *output,size_t capacity,int32_t funds);
/* Original304b0: signed tick/48 + base_year, truncated to signed16 year;
 * (tick%48)/4 chooses month. Negative ticks are invalid game state.
 * months may be NULL for original English MESSAGE.PTF record4 strings. */
int sim_city_stats_date(char *output,size_t capacity,int32_t city_tick,
                         int16_t base_year,const char *const months[12]);
/* Draw original dynamic UI over the already rendered static city shell.
 * Original DGROUP is read-only. Standard MCGA geometry/mode6 only.
 * font8192 is the original 128x64 byte mask section. */
int sim_city_stats_demand_render(uint8_t *fb,const uint8_t *dgroup,size_t size);
int sim_city_stats_funds_render(uint8_t *fb,const uint8_t *font8192,
                                 const uint8_t *dgroup,size_t size);
int sim_city_stats_date_render(uint8_t *fb,const uint8_t *font8192,
                                const uint8_t *dgroup,size_t size);
#endif
