#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

int main(void) {
    printf("=== EGL test ===\n");

    int fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0) { perror("open /dev/dri/card0"); return 1; }
    printf("[ok] opened /dev/dri/card0\n");

    struct gbm_device* gbm = gbm_create_device(fd);
    if (!gbm) { fprintf(stderr, "[fail] gbm_create_device\n"); return 1; }
    printf("[ok] gbm device created\n");

    EGLDisplay dpy = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm, NULL);
    if (dpy == EGL_NO_DISPLAY) {
        fprintf(stderr, "[fail] eglGetPlatformDisplay\n");
        return 1;
    }
    printf("[ok] eglGetPlatformDisplay\n");

    EGLint major, minor;
    if (!eglInitialize(dpy, &major, &minor)) {
        fprintf(stderr, "[fail] eglInitialize\n");
        return 1;
    }
    printf("[ok] EGL %d.%d\n", major, minor);
    printf("     vendor:     %s\n", eglQueryString(dpy, EGL_VENDOR));
    printf("     version:    %s\n", eglQueryString(dpy, EGL_VERSION));
    printf("     client APIs:%s\n", eglQueryString(dpy, EGL_CLIENT_APIS));

    eglTerminate(dpy);
    gbm_device_destroy(gbm);
    close(fd);
    printf("=== EGL test passed ===\n");
    return 0;
}
