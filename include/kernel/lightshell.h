#ifndef BRIGHTS_LIGHTSHELL_H
#define BRIGHTS_LIGHTSHELL_H

void brights_lightshell_run(void);

const char *brights_lightshell_current_dir(void);
void brights_lightshell_set_current_dir(const char *path);
int parse_two_args(const char *arg, char *first, int first_cap, char *second, int second_cap);

#ifdef BRIGHTS_LIGHTSHELL_TESTING
void brights_lightshell_reset_for_test(void);
int brights_lightshell_eval_for_test(char *line);
const char *brights_lightshell_current_user_for_test(void);
const char *brights_lightshell_current_dir_for_test(void);
int brights_lightshell_is_root_for_test(void);
#endif

#endif
