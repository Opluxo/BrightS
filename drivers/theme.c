#include "theme.h"
#include "../kernel/core/kernel_util.h"

static int current_theme_id = BRIGHTS_THEME_DARK;

/* ---- Built-in themes ---- */

static const brights_theme_t themes[BRIGHTS_THEME_COUNT] = {
  /* BRIGHTS_THEME_DARK - Default dark theme */
  [BRIGHTS_THEME_DARK] = {
    .name = "Dark",
    .description = "Classic dark theme with cyan accents",
    .colors = {
      .bg_primary     = {  5,  8, 20 },
      .bg_secondary   = { 12, 18, 42 },
      .bg_tertiary    = {  8, 12, 30 },
      .bar_bg         = { 10, 18, 45 },
      .bar_bg_alt     = { 14, 30, 65 },
      .bar_accent     = {  0, 200, 255 },
      .bar_edge       = {  0, 100, 150 },
      .text_primary   = { 230, 235, 245 },
      .text_secondary = { 150, 160, 180 },
      .text_accent    = {  85, 220, 255 },
      .text_muted     = {  70,  80, 100 },
      .success        = {  80, 220, 100 },
      .warning        = { 255, 200,  50 },
      .error          = { 255,  80,  80 },
      .info           = {  80, 200, 255 },
      .border         = {  40,  60, 100 },
      .border_focus   = {  0, 200, 255 },
      .border_accent  = {  0, 180, 220 },
      .shadow         = {  2,  4,  12 },
      .gradient_start = {  5,  8, 22 },
      .gradient_end   = {  2,  4,  14 },
      .selection      = {  20, 50, 100 },
      .hover          = {  15, 35,  70 },
    }
  },

  /* BRIGHTS_THEME_OCEAN - Blue-tinted theme */
  [BRIGHTS_THEME_OCEAN] = {
    .name = "Ocean",
    .description = "Deep ocean blue theme",
    .colors = {
      .bg_primary     = {  4,  12, 28 },
      .bg_secondary   = {  8, 22, 50 },
      .bg_tertiary    = {  6, 16, 38 },
      .bar_bg         = {  6, 18, 48 },
      .bar_bg_alt     = { 10, 30, 70 },
      .bar_accent     = {  0, 180, 230 },
      .bar_edge       = {  0, 120, 180 },
      .text_primary   = { 210, 230, 250 },
      .text_secondary = { 130, 160, 200 },
      .text_accent    = { 100, 200, 255 },
      .text_muted     = {  60,  80, 110 },
      .success        = {  60, 210, 130 },
      .warning        = { 255, 190,  60 },
      .error          = { 255,  90,  90 },
      .info           = {  60, 190, 255 },
      .border         = {  30,  60, 110 },
      .border_focus   = {  0, 180, 230 },
      .border_accent  = {  0, 160, 210 },
      .shadow         = {  2,  6,  16 },
      .gradient_start = {  4, 12, 30 },
      .gradient_end   = {  2,  6,  18 },
      .selection      = {  15, 45,  95 },
      .hover          = {  10, 30,  70 },
    }
  },

  /* BRIGHTS_THEME_FOREST - Green-tinted theme */
  [BRIGHTS_THEME_FOREST] = {
    .name = "Forest",
    .description = "Deep forest green theme",
    .colors = {
      .bg_primary     = {  4,  14,  10 },
      .bg_secondary   = {  8,  26,  18 },
      .bg_tertiary    = {  6,  20,  14 },
      .bar_bg         = {  6,  22,  16 },
      .bar_bg_alt     = { 10,  36,  24 },
      .bar_accent     = {  0, 210, 120 },
      .bar_edge       = {  0, 140,  80 },
      .text_primary   = { 215, 240, 225 },
      .text_secondary = { 130, 175, 150 },
      .text_accent    = { 100, 235, 160 },
      .text_muted     = {  55,  90,  70 },
      .success        = {  60, 220, 120 },
      .warning        = { 255, 200,  50 },
      .error          = { 255,  85,  85 },
      .info           = {  80, 210, 180 },
      .border         = {  30,  70,  50 },
      .border_focus   = {  0, 210, 120 },
      .border_accent  = {  0, 180, 100 },
      .shadow         = {  2,   6,   4 },
      .gradient_start = {  4,  14,  10 },
      .gradient_end   = {  2,   8,   6 },
      .selection      = {  15,  55,  35 },
      .hover          = {  10,  40,  28 },
    }
  },

  /* BRIGHTS_THEME_SUNSET - Warm orange/red theme */
  [BRIGHTS_THEME_SUNSET] = {
    .name = "Sunset",
    .description = "Warm sunset orange theme",
    .colors = {
      .bg_primary     = {  18,  8,  6 },
      .bg_secondary   = {  30, 14, 10 },
      .bg_tertiary    = {  24, 10,  8 },
      .bar_bg         = {  28, 12,  8 },
      .bar_bg_alt     = {  45, 20, 14 },
      .bar_accent     = { 255, 140,  40 },
      .bar_edge       = { 200, 100,  30 },
      .text_primary   = { 250, 230, 215 },
      .text_secondary = { 200, 160, 140 },
      .text_accent    = { 255, 180,  80 },
      .text_muted     = { 120,  80,  60 },
      .success        = { 100, 210, 100 },
      .warning        = { 255, 180,  40 },
      .error          = { 255,  70,  60 },
      .info           = { 255, 160,  80 },
      .border         = {  80,  45,  30 },
      .border_focus   = { 255, 140,  40 },
      .border_accent  = { 230, 120,  35 },
      .shadow         = {  8,  3,  2 },
      .gradient_start = {  18,  8,  6 },
      .gradient_end   = {  10,  4,  3 },
      .selection      = {  70, 30,  18 },
      .hover          = {  50, 22,  14 },
    }
  },

  /* BRIGHTS_THEME_MONO - Monochrome theme */
  [BRIGHTS_THEME_MONO] = {
    .name = "Mono",
    .description = "Pure monochrome theme",
    .colors = {
      .bg_primary     = {  10, 10, 10 },
      .bg_secondary   = {  20, 20, 20 },
      .bg_tertiary    = {  15, 15, 15 },
      .bar_bg         = {  18, 18, 18 },
      .bar_bg_alt     = {  30, 30, 30 },
      .bar_accent     = { 200, 200, 200 },
      .bar_edge       = { 120, 120, 120 },
      .text_primary   = { 230, 230, 230 },
      .text_secondary = { 150, 150, 150 },
      .text_accent    = { 255, 255, 255 },
      .text_muted     = {  70,  70,  70 },
      .success        = { 180, 180, 180 },
      .warning        = { 220, 220, 220 },
      .error          = { 255, 255, 255 },
      .info           = { 200, 200, 200 },
      .border         = {  60,  60,  60 },
      .border_focus   = { 200, 200, 200 },
      .border_accent  = { 180, 180, 180 },
      .shadow         = {  4,  4,  4 },
      .gradient_start = {  10, 10, 10 },
      .gradient_end   = {  5,  5,  5 },
      .selection      = {  45, 45, 45 },
      .hover          = {  35, 35, 35 },
    }
  },
};

/* ---- Implementation ---- */

void brights_theme_init(void)
{
  current_theme_id = BRIGHTS_THEME_DARK;
}

void brights_theme_set(int theme_id)
{
  if (theme_id >= 0 && theme_id < BRIGHTS_THEME_COUNT)
    current_theme_id = theme_id;
}

const brights_theme_colors_t *brights_theme_get(void)
{
  return &themes[current_theme_id].colors;
}

const brights_theme_t *brights_theme_get_by_id(int theme_id)
{
  if (theme_id < 0 || theme_id >= BRIGHTS_THEME_COUNT) return 0;
  return &themes[theme_id];
}

int brights_theme_count(void)
{
  return BRIGHTS_THEME_COUNT;
}

int brights_theme_current_id(void)
{
  return current_theme_id;
}

/* ---- Convenience accessors ---- */

brights_color_t brights_theme_bg(void)          { return themes[current_theme_id].colors.bg_primary; }
brights_color_t brights_theme_bar_bg(void)      { return themes[current_theme_id].colors.bar_bg; }
brights_color_t brights_theme_bar_accent(void)  { return themes[current_theme_id].colors.bar_accent; }
brights_color_t brights_theme_text(void)        { return themes[current_theme_id].colors.text_primary; }
brights_color_t brights_theme_text_accent(void) { return themes[current_theme_id].colors.text_accent; }
brights_color_t brights_theme_success(void)     { return themes[current_theme_id].colors.success; }
brights_color_t brights_theme_warning(void)     { return themes[current_theme_id].colors.warning; }
brights_color_t brights_theme_error(void)       { return themes[current_theme_id].colors.error; }
brights_color_t brights_theme_info(void)        { return themes[current_theme_id].colors.info; }
brights_color_t brights_theme_border(void)      { return themes[current_theme_id].colors.border; }
brights_color_t brights_theme_border_focus(void){ return themes[current_theme_id].colors.border_focus; }
brights_color_t brights_theme_shadow(void)      { return themes[current_theme_id].colors.shadow; }
brights_color_t brights_theme_selection(void)   { return themes[current_theme_id].colors.selection; }
brights_color_t brights_theme_gradient_start(void) { return themes[current_theme_id].colors.gradient_start; }
brights_color_t brights_theme_gradient_end(void)   { return themes[current_theme_id].colors.gradient_end; }
