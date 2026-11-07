#ifndef SIMCITY_DOS_QUESTION_UI_H
#define SIMCITY_DOS_QUESTION_UI_H
#include <stdint.h>
enum {SIM_QUESTION_YES_NO=0,SIM_QUESTION_YES_NO_CANCEL=1,SIM_QUESTION_RETRY_CANCEL=2};
typedef struct {int x0,y0,x1,y1;} SimQuestionRect;
typedef struct {SimQuestionRect box,button[3];int count;} SimQuestionLayout;
int sim_question_layout(const char *text,unsigned kind,SimQuestionLayout *layout);
int sim_question_render(uint8_t *fb,const uint8_t *font,const char *text,unsigned kind);
/* Original263c2 choice index, or-1 for an unmatched key/point. Enter always
 * chooses0; Escape chooses No(1) or Cancel(2/1), independent of focus. */
int sim_question_key(unsigned kind,unsigned key);
int sim_question_hit(const SimQuestionLayout *layout,int x,int y);
#endif
