#ifndef BRIGHTS_LIGHTSHELL_H
#define BRIGHTS_LIGHTSHELL_H

void brights_lightshell_run(void);

#ifdef BRIGHTS_LIGHTSHELL_TESTING
void brights_lightshell_reset_for_test(void);
int brights_lightshell_eval_for_test(char *line);
const char *brights_lightshell_current_user_for_test(void);
const char *brights_lightshell_current_dir_for_test(void);
int brights_lightshell_is_root_for_test(void);
#endif

#endif
