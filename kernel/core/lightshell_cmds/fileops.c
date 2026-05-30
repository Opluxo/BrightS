#include "fileops.h"
#include "../lightshell.h"
#include "../printf.h"
#include "fs/ramfs.h"
#include "../drivers/serial.h"
#include "string.h"

int cmd_ls_handler(const char *arg)
{
    const char *path = arg && arg[0] ? arg : "/";
    // First check if it's a directory
    brights_ramfs_stat_t st;
    if (brights_ramfs_stat(path, &st) != 0 || !st.is_dir) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ls: cannot access '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, path);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "': No such directory\n");
        return 1;
    }
    
    // List all files using public directory entry interface
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    
    const char *entries[64];
    int count = brights_ramfs_get_dir_entries(path, entries, 64);
    
    for (int i = 0; i < count; i++) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, entries[i]);
        
        brights_ramfs_stat_t st;
        char full_path[BRIGHTS_RAMFS_MAX_NAME];
        kutil_strcpy(full_path, path);
        kutil_strcat(full_path, "/");
        kutil_strcat(full_path, entries[i]);
        
        if (brights_ramfs_stat(full_path, &st) == 0 && st.is_dir) {
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, "/");
        }
        
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    return 1;
}

int cmd_cd_handler(const char *arg)
{
    const char *path = arg && arg[0] ? arg : "/";
    
    int fd = brights_ramfs_open(path);
    if (fd < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "cd: no such directory: ");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, path);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
        return 1;
    }
    
    if (!brights_ramfs_is_dir_fd(fd)) {
        brights_ramfs_close(fd);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "cd: not a directory: ");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, path);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
        return 1;
    }
    
    brights_ramfs_close(fd);
    brights_lightshell_set_current_dir(path);
    return 1;
}

int cmd_pwd_handler(const char *arg)
{
    (void)arg;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, brights_lightshell_current_dir());
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    return 1;
}

int cmd_mkdir_handler(const char *arg)
{
    if (!arg || !arg[0]) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mkdir: missing operand\n");
        return 1;
    }
    if (brights_ramfs_mkdir(arg) != 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mkdir: cannot create directory '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "'\n");
        return 1;
    }
    return 1;
}

int cmd_rmdir_handler(const char *arg)
{
    if (!arg || !arg[0]) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "rmdir: missing operand\n");
        return 1;
    }
    if (brights_ramfs_rmdir(arg) != 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "rmdir: failed to remove '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "'\n");
        return 1;
    }
    return 1;
}

int cmd_cat_handler(const char *arg)
{
    if (!arg || !arg[0]) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "cat: missing file operand\n");
        return 1;
    }
    
    int fd = brights_ramfs_open(arg);
    if (fd < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "cat: cannot open '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "' for reading\n");
        return 1;
    }
    
    char buf[128];
    int64_t n;
    while ((n = brights_ramfs_read(fd, 0, buf, sizeof(buf))) > 0) {
        brights_serial_write(BRIGHTS_COM1_PORT, buf, n);
    }
    
    if (n < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\ncat: error reading '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "'\n");
    }
    
    brights_ramfs_close(fd);
    return 1;
}

int cmd_stat_handler(const char *arg)
{
    if (!arg || !arg[0]) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "stat: missing file operand\n");
        return 1;
    }
    
    brights_ramfs_stat_t st;
    if (brights_ramfs_stat(arg, &st) != 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "stat: cannot stat '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "'\n");
        return 1;
    }
    
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  File: ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n  Size: ");
    char numbuf[32];
    int len = kutil_utoa(st.size, numbuf, 10);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, numbuf);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
    return 1;
}

int cmd_rm_handler(const char *arg)
{
    if (!arg || !arg[0]) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "rm: missing operand\n");
        return 1;
    }
    if (brights_ramfs_unlink(arg) != 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "rm: cannot remove '");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "'\n");
        return 1;
    }
    return 1;
}

int cmd_cp_handler(const char *arg)
{
    char src[BRIGHTS_RAMFS_MAX_NAME], dst[BRIGHTS_RAMFS_MAX_NAME];
    
    if (parse_two_args(arg, src, sizeof(src), dst, sizeof(dst)) < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: cp <source> <destination>\n");
        return 1;
    }
    
    int src_fd = brights_ramfs_open(src);
    if (src_fd < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "cp: cannot open source file\n");
        return 1;
    }
    
    uint64_t size = brights_ramfs_file_size(src_fd);
    
    int dst_fd = brights_ramfs_create(dst);
    if (dst_fd < 0) {
        brights_ramfs_close(src_fd);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "cp: cannot create destination file\n");
        return 1;
    }
    
    uint8_t buf[512];
    uint64_t total = 0;
    uint64_t left = size;
    
    while (left > 0) {
        uint64_t chunk = (left > sizeof(buf)) ? sizeof(buf) : left;
        int64_t readn = brights_ramfs_read(src_fd, total, buf, chunk);
        
        if (readn <= 0) break;
        
        brights_ramfs_write(dst_fd, total, buf, readn);
        total += readn;
        left -= readn;
    }
    
    brights_ramfs_close(src_fd);
    brights_ramfs_close(dst_fd);
    return 1;
}

int cmd_mv_handler(const char *arg)
{
    char src[BRIGHTS_RAMFS_MAX_NAME], dst[BRIGHTS_RAMFS_MAX_NAME];
    
    if (parse_two_args(arg, src, sizeof(src), dst, sizeof(dst)) < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: mv <source> <destination>\n");
        return 1;
    }
    
    // For now, mv is implemented as cp + rm
    int src_fd = brights_ramfs_open(src);
    if (src_fd < 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mv: cannot open source file\n");
        return 1;
    }
    
    uint64_t size = brights_ramfs_file_size(src_fd);
    
    int dst_fd = brights_ramfs_create(dst);
    if (dst_fd < 0) {
        brights_ramfs_close(src_fd);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mv: cannot create destination file\n");
        return 1;
    }
    
    uint8_t buf[512];
    uint64_t total = 0;
    uint64_t left = size;
    
    while (left > 0) {
        uint64_t chunk = (left > sizeof(buf)) ? sizeof(buf) : left;
        int64_t readn = brights_ramfs_read(src_fd, total, buf, chunk);
        
        if (readn <= 0) break;
        
        brights_ramfs_write(dst_fd, total, buf, readn);
        total += readn;
        left -= readn;
    }
    
    brights_ramfs_close(src_fd);
    brights_ramfs_close(dst_fd);
    
    // Remove original file
    brights_ramfs_unlink(src);
    return 1;
}

int cmd_touch_handler(const char *arg)
{
    if (!arg || !arg[0]) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "touch: missing file operand\n");
        return 1;
    }
    int fd = brights_ramfs_open(arg);
    if (fd >= 0) {
        // File exists, just close (no timestamp tracking)
        brights_ramfs_close(fd);
    } else {
        // File doesn't exist, create it
        fd = brights_ramfs_create(arg);
        if (fd < 0) {
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, "touch: cannot touch '");
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, "'\n");
            return 1;
        }
        brights_ramfs_close(fd);
    }
    return 1;
}
