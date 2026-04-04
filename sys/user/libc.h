#ifndef BRIGHTS_USER_LIBC_H
#define BRIGHTS_USER_LIBC_H

#include <stdint.h>

/* ===== String functions ===== */
void *memset(void *s, int c, uint64_t n);
void *memcpy(void *dst, const void *src, uint64_t n);
void *memmove(void *dst, const void *src, uint64_t n);
int   memcmp(const void *s1, const void *s2, uint64_t n);
uint64_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, uint64_t n);
int   strcmp(const char *s1, const char *s2);
int   strncmp(const char *s1, const char *s2, uint64_t n);
char *strcat(char *dst, const char *src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

/* ===== Memory allocation ===== */
void *malloc(uint64_t size);
void  free(void *ptr);
void *realloc(void *ptr, uint64_t new_size);

/* ===== Standard I/O ===== */
int putchar(int ch);
int puts(const char *s);
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, uint64_t n, const char *fmt, ...);

/* ===== System calls (direct wrappers) ===== */
int64_t sys_read(int fd, void *buf, uint64_t count);
int64_t sys_write(int fd, const void *buf, uint64_t count);
int64_t sys_open(const char *path, int flags);
int     sys_close(int fd);
int64_t sys_fork(void);
int64_t sys_exec(const char *path, char **argv, char **envp);
int64_t sys_exit(int status);
int64_t sys_getpid(void);
int64_t sys_getppid(void);
int64_t sys_wait(int pid, int *status);
int64_t sys_sleep_ms(uint64_t ms);
int64_t sys_clock_ms(void);
int64_t sys_mkdir(const char *path);
int64_t sys_unlink(const char *path);
int64_t sys_create(const char *path);
int64_t sys_lseek(int fd, int64_t offset, int whence);
int64_t sys_stat(const char *path, uint64_t *size, uint32_t *mode);
int64_t sys_getcwd(char *buf, uint64_t size);
int64_t sys_chdir(const char *path);
int64_t sys_readdir(int fd, char *buf, uint64_t count);
int64_t sys_yield(void);
int64_t sys_dup(int fd);
int64_t sys_dup2(int oldfd, int newfd);
int64_t sys_pipe(int *fds);
int64_t sys_chmod(const char *path, uint32_t mode);
int64_t sys_chown(const char *path, uint32_t uid, uint32_t gid);
int64_t sys_symlink(const char *target, const char *linkpath);
int64_t sys_readlink(const char *path, char *buf, uint64_t bufsize);
int64_t sys_uname(void *buf);
int64_t sys_sysinfo(void *buf);
int64_t sys_socket(int domain, int type, int protocol);
int64_t sys_bind(int sockfd, uint32_t addr, uint16_t port);
int64_t sys_listen(int sockfd, int backlog);
int64_t sys_accept(int sockfd, uint32_t *addr, uint16_t *port);
int64_t sys_connect(int sockfd, uint32_t addr, uint16_t port);
int64_t sys_send(int sockfd, const void *buf, uint64_t len);
int64_t sys_recv(int sockfd, void *buf, uint64_t len);
int64_t sys_close_socket(int sockfd);
int64_t sys_icmp_echo(uint32_t dst_ip);
int64_t sys_ifconfig(int cmd);
int64_t sys_ip_parse(const char *str);
void sys_ip_to_str(uint32_t ip, char *buf);

/* ===== Utility ===== */
void abort(void);
void exit(int status);

#endif
