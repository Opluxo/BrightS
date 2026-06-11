#include "devfs.h"
#include "devfs_vfs.h"
#include "../drivers/tty.h"

int brights_devfs_init(void)
{
  brights_devfs_vfs_init();
  return 6; /* tty0, null, zero, rtc, kbd, disk0 */
}
