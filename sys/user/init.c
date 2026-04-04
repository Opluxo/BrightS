#include "libc.h"

/*
 * BrightS init - PID 1
 *
 * This is the first user-space process. It:
 * 1. Mounts essential filesystems
 * 2. Sets up /dev, /tmp directories
 * 3. Launches the login shell
 * 4. Reaps zombie children
 */

static void mount_dev(void)
{
  /* /dev is handled by devfs in kernel */
  printf("init: /dev ready\n");
}

static void mount_tmp(void)
{
  /* /tmp is handled by ramfs in kernel */
  printf("init: /tmp ready\n");
}

static void setup_dirs(void)
{
  sys_mkdir("/usr");
  sys_mkdir("/usr/home");
  sys_mkdir("/usr/home/guest");
  sys_mkdir("/bin");
  sys_mkdir("/bin/pkg");
  sys_mkdir("/bin/config");
  sys_mkdir("/bin/runtime");
  sys_mkdir("/bin/firmware");
  sys_mkdir("/mnt");
  sys_mkdir("/mnt/drive");
  sys_mkdir("/mnt/input");
  sys_mkdir("/mnt/output");
  sys_mkdir("/swp");
  sys_mkdir("/tmp");
  printf("init: directories ready\n");
}

static void setup_profile(void)
{
  /* Create default guest profile */
  int fd = sys_open("/bin/config/guest/example.pf", 0x40); /* O_CREAT */
  if (fd >= 0) {
    const char *profile =
      "username:guest\n"
      "hostname:brights\n"
      "avatar:\"default\"\n"
      "email:user@local\n"
      "password:guest\n";
    sys_write(fd, profile, strlen(profile));
    sys_close(fd);
  }
  printf("init: profile ready\n");
}

static void reap_children(void)
{
  /* Reap any zombie children */
  int status;
  while (sys_wait(-1, &status) > 0) {
    /* Child reaped */
  }
}

static void launch_shell(void)
{
  printf("init: launching shell...\n");

  /* Fork and exec shell */
  int64_t pid = sys_fork();
  if (pid == 0) {
    /* Child: exec shell */
    char *argv[] = { "shell", 0 };
    sys_exec("/bin/shell", argv, 0);

    /* If exec fails, print error and exit */
    printf("init: failed to exec shell\n");
    sys_exit(1);
  } else if (pid > 0) {
    printf("init: shell pid=%d\n", (int)pid);
  } else {
    printf("init: fork failed\n");
  }
}

int main(int argc, char **argv)
{
  (void)argc; (void)argv;

  printf("BrightS init starting...\n");

  /* Setup directories */
  setup_dirs();

  /* Mount filesystems */
  mount_dev();
  mount_tmp();

  /* Setup default profile */
  setup_profile();

  printf("init: system ready\n");

  /* Launch shell */
  launch_shell();

  /* Main loop: reap zombies */
  for (;;) {
    sys_sleep_ms(1000);
    reap_children();
  }

  return 0;
}
