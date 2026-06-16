#ifndef BRIGHTS_THEME_H
#define BRIGHTS_THEME_H

#include <stdint.h>
#include "fb.h"

/* Theme identifiers */
#define BRIGHTS_THEME_DARK       0
#define BRIGHTS_THEME_OCEAN      1
#define BRIGHTS_THEME_FOREST     2
#define BRIGHTS_THEME_SUNSET     3
#define BRIGHTS_THEME_MONO       4
#define BRIGHTS_THEME_COUNT      5

/* UI element color slots */
typedef struct {
  /* Background colors */
  brights_color_t bg_primary;       /* Main background (desktop, console) */
  brights_color_t bg_secondary;     /* Panel/box backgrounds */
  brights_color_t bg_tertiary;      /* Input fields, recessed areas */

  /* Bar colors */
  brights_color_t bar_bg;           /* Title/status bar background */
  brights_color_t bar_bg_alt;       /* Bar gradient end */
  brights_color_t bar_accent;       /* Top accent line */
  brights_color_t bar_edge;         /* Bottom edge highlight */

  /* Text colors */
  brights_color_t text_primary;     /* Main text (white equivalent) */
  brights_color_t text_secondary;   /* Dimmed text */
  brights_color_t text_accent;      /* Highlighted/linked text */
  brights_color_t text_muted;       /* Very dim text */

  /* Semantic colors */
  brights_color_t success;          /* Green: online, running */
  brights_color_t warning;          /* Yellow/orange: caution */
  brights_color_t error;            /* Red: error, danger */
  brights_color_t info;             /* Cyan: informational */

  /* Border colors */
  brights_color_t border;           /* Default border */
  brights_color_t border_focus;     /* Focused/active border */
  brights_color_t border_accent;    /* Accent border (dialog title) */

  /* Shadow */
  brights_color_t shadow;           /* Shadow color */

  /* Gradient */
  brights_color_t gradient_start;   /* Background gradient top */
  brights_color_t gradient_end;     /* Background gradient bottom */

  /* Selection / highlight */
  brights_color_t selection;        /* Selected item background */
  brights_color_t hover;            /* Hover state background */
} brights_theme_colors_t;

/* Theme descriptor */
typedef struct {
  const char *name;
  const char *description;
  brights_theme_colors_t colors;
} brights_theme_t;

/* Initialize theme system with default theme */
void brights_theme_init(void);

/* Set active theme by ID */
void brights_theme_set(int theme_id);

/* Get active theme colors */
const brights_theme_colors_t *brights_theme_get(void);

/* Get theme by ID */
const brights_theme_t *brights_theme_get_by_id(int theme_id);

/* Get number of available themes */
int brights_theme_count(void);

/* Get current theme ID */
int brights_theme_current_id(void);

/* Convenience accessors */
brights_color_t brights_theme_bg(void);
brights_color_t brights_theme_bar_bg(void);
brights_color_t brights_theme_bar_accent(void);
brights_color_t brights_theme_text(void);
brights_color_t brights_theme_text_accent(void);
brights_color_t brights_theme_success(void);
brights_color_t brights_theme_warning(void);
brights_color_t brights_theme_error(void);
brights_color_t brights_theme_info(void);
brights_color_t brights_theme_border(void);
brights_color_t brights_theme_border_focus(void);
brights_color_t brights_theme_shadow(void);
brights_color_t brights_theme_selection(void);
brights_color_t brights_theme_gradient_start(void);
brights_color_t brights_theme_gradient_end(void);

#endif
