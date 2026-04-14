#ifndef BRIGHTS_IM_H
#define BRIGHTS_IM_H

#include <stdint.h>

#define IM_MAX_PINYIN 16
#define IM_MAX_CANDIDATES 10
#define IM_CANDIDATE_WIDTH 80

void brights_im_init(void);

int brights_im_is_active(void);

void brights_im_toggle(void);

void brights_im_handle_char(char ch);

void brights_im_handle_special(int key);

const char *brights_im_get_preedit(void);

int brights_im_get_candidate_count(void);

const char *brights_im_get_candidate(int index);

void brights_im_select_candidate(int index);

void brights_im_commit(void);

void brights_im_backspace(void);

void brights_im_clear(void);

void brights_im_draw_candidates(void);

int brights_im_cursor_pos(void);

#endif
