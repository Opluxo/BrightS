#ifndef BRIGHTS_KSHELL_H
#define BRIGHTS_KSHELL_H

void brights_kshell_run(void);

#ifdef BRIGHTS_KSHELL_TESTING
void brights_kshell_reset_for_test(void);
int brights_kshell_eval_for_test(char *line);
const char *brights_kshell_current_user_for_test(void);
const char *brights_kshell_current_dir_for_test(void);
int brights_kshell_is_root_for_test(void);
#endif

#endif
