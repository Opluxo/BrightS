#include "libc.h"
#include "service.h"
#include "lang_runtime.h"
#include "rust_runtime.h"
#include "python_runtime.h"
#include "cpp_runtime.h"

/*
 * BrightS init - PID 1 - Service Manager
 *
 * This is the first user-space process. It manages the entire system:
 * 1. Initializes service management system
 * 2. Loads service configurations
 * 3. Starts essential services in dependency order
 * 4. Monitors service health and handles failures
 * 5. Provides service control interface
 */

static void setup_filesystem(void)
{
  /* Create essential directories */
  const char *dirs[] = {
    "/usr", "/usr/home", "/usr/home/guest",
    "/bin", "/bin/pkg", "/bin/config", "/bin/runtime", "/bin/firmware",
    "/mnt", "/mnt/drive", "/mnt/input", "/mnt/output",
    "/swp", "/tmp", "/var", "/var/log", "/var/run",
    NULL
  };

  for (int i = 0; dirs[i]; i++) {
    if (sys_mkdir(dirs[i]) < 0) {
      printf("init: failed to create directory %s\n", dirs[i]);
    }
  }

  printf("init: filesystem initialized\n");
}

static void setup_default_config(void)
{
  /* Create default service configuration */
  int fd = sys_open("/bin/config/services.ini", 0x40); /* O_CREAT */
  if (fd >= 0) {
    const char *config =
      "[syslogd]\n"
      "description=System Logging Daemon\n"
      "command=/bin/syslogd\n"
      "type=daemon\n"
      "restart=yes\n"
      "restart_delay=1000\n"
      "\n"
      "[dhcpd]\n"
      "description=DHCP Client Daemon\n"
      "command=/bin/dhcpd\n"
      "type=daemon\n"
      "restart=yes\n"
      "restart_delay=5000\n"
      "depends_on=syslogd\n"
      "\n"
      "[shell]\n"
      "description=BrightS Command Shell\n"
      "command=/bin/shell\n"
      "type=daemon\n"
      "restart=yes\n"
      "restart_delay=1000\n";

    sys_write(fd, config, strlen(config));
    sys_close(fd);
    printf("init: default service config created\n");
  } else {
    printf("init: failed to create service config\n");
  }

  /* Create default user profile */
  fd = sys_open("/bin/config/guest/profile.ini", 0x40);
  if (fd >= 0) {
    const char *profile =
      "[user]\n"
      "username=guest\n"
      "hostname=brights\n"
      "shell=/bin/shell\n"
      "home=/usr/home/guest\n";

    sys_write(fd, profile, strlen(profile));
    sys_close(fd);
    printf("init: default user profile created\n");
  }
}

static void init_language_runtimes(void)
{
  printf("init: initializing language runtimes...\n");

  /* Initialize language runtime system */
  if (lang_init() != 0) {
    printf("init: failed to initialize language system\n");
    return;
  }

  /* Register Rust runtime */
  runtime_t *rust_rt = rust_create_runtime();
  if (lang_register_runtime(rust_rt) != 0) {
    printf("init: failed to register Rust runtime\n");
  }

  /* Register Python runtime */
  runtime_t *python_rt = py_create_runtime();
  if (lang_register_runtime(python_rt) != 0) {
    printf("init: failed to register Python runtime\n");
  }

  /* Register C++ runtime */
  runtime_t *cpp_rt = cpp_create_runtime();
  if (lang_register_runtime(cpp_rt) != 0) {
    printf("init: failed to register C++ runtime\n");
  }

  /* Set C as default runtime */
  lang_switch_runtime("c");

  printf("init: language runtimes initialized\n");
}

static void start_essential_services(void)
{
  printf("init: starting essential services...\n");

  /* Load service configuration */
  if (service_load_config("/bin/config/services.ini") == 0) {
    /* Start core services in order */
    const char *essential_services[] = {
      "syslogd",  /* System logging first */
      "dhcpd",    /* Network configuration */
      "shell",    /* User interface */
      NULL
    };

    for (int i = 0; essential_services[i]; i++) {
      if (service_start(essential_services[i]) != 0) {
        printf("init: failed to start %s\n", essential_services[i]);
      }
    }
  } else {
    printf("init: no service configuration found, starting shell directly\n");

    /* Fallback: start shell directly */
    int64_t pid = sys_fork();
    if (pid == 0) {
      char *argv[] = { "shell", NULL };
      sys_exec("/bin/shell", argv, NULL);
      printf("init: failed to exec shell\n");
      sys_exit(1);
    } else if (pid > 0) {
      printf("init: shell started (pid=%d)\n", (int)pid);
    }
  }
}

int main(int argc, char **argv)
{
  (void)argc; (void)argv;

  printf("BrightS init v2.0 - Service Manager starting...\n");

  /* Initialize service management system */
  if (service_init() != 0) {
    printf("init: failed to initialize service system\n");
    return 1;
  }

  /* Setup filesystem structure */
  setup_filesystem();

  /* Create default configurations */
  setup_default_config();

  /* Initialize language runtimes */
  init_language_runtimes();

  printf("init: system initialization complete\n");

  /* Start essential services */
  start_essential_services();

  printf("init: all services started, entering monitor mode\n");

  /* Main service monitoring loop */
  service_monitor_loop();

  /* Should never reach here */
  return 0;
}
