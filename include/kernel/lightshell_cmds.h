#ifndef BRIGHTS_LIGHTSHELL_CMDS_H
#define BRIGHTS_LIGHTSHELL_CMDS_H

/* Lightshell command handler type */
typedef int (*lightshell_cmd_fn)(const char *arg);

/* Command registration */
typedef struct {
  const char *name;
  lightshell_cmd_fn handler;
} lightshell_cmd_t;

/* Built-in commands */
extern const lightshell_cmd_t lightshell_builtin_cmds[];
extern const int lightshell_builtin_count;

/* BST procom commands */
extern const lightshell_cmd_t lightshell_bst_cmds[];
extern const int lightshell_bst_count;

/* Helper functions */
int lightshell_cmd_help(const char *arg);
int lightshell_cmd_ls(const char *arg);
int lightshell_cmd_pwd(const char *arg);
int lightshell_cmd_cd(const char *arg);
int lightshell_cmd_mkdir(const char *arg);
int lightshell_cmd_rmdir(const char *arg);
int lightshell_cmd_cat(const char *arg);
int lightshell_cmd_stat(const char *arg);
int lightshell_cmd_rm(const char *arg);
int lightshell_cmd_cp(const char *arg);
int lightshell_cmd_mv(const char *arg);
int lightshell_cmd_echo(const char *arg);
int lightshell_cmd_kill(const char *arg);
int lightshell_cmd_jobs(const char *arg);
int lightshell_cmd_wifi(const char *arg);
int lightshell_cmd_ifconfig(const char *arg);
int lightshell_cmd_bst(const char *arg);

#endif
