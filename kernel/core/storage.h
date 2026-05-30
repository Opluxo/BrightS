#ifndef BRIGHTS_STORAGE_H
#define BRIGHTS_STORAGE_H
int brights_storage_bootstrap(void);
int brights_storage_mount_system(void);
int brights_storage_rescan_and_mount(void);
const char *brights_storage_backend(void);
#endif
