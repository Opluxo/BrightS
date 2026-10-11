#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int main(void) {
    printf("=== DRM test ===\n");

    int fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0) { perror("open /dev/dri/card0"); return 1; }
    printf("[ok] opened /dev/dri/card0\n");

    if (drmSetMaster(fd) < 0) perror("drmSetMaster (warning only)");

    drmModeRes *res = drmModeGetResources(fd);
    if (!res) { fprintf(stderr, "[fail] drmModeGetResources\n"); return 1; }
    printf("[ok] resources: %d connectors, %d CRTCs\n",
           res->count_connectors, res->count_crtcs);

    drmModeConnector *conn = NULL;
    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *c = drmModeGetConnector(fd, res->connectors[i]);
        if (!c) continue;
        printf("     connector %u: %s\n", c->connector_id,
               c->connection == DRM_MODE_CONNECTED ? "connected" : "disconnected");
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            conn = c;
            break;
        }
        drmModeFreeConnector(c);
    }
    if (!conn) { fprintf(stderr, "[fail] no connected connector\n"); return 1; }

    drmModeModeInfo mode = conn->modes[0];
    printf("[ok] mode: %dx%d @ %dHz\n",
           mode.hdisplay, mode.vdisplay, mode.vrefresh);

    uint32_t crtc_id = 0;
    if (conn->encoder_id) {
        drmModeEncoder *enc = drmModeGetEncoder(fd, conn->encoder_id);
        if (enc) { crtc_id = enc->crtc_id; drmModeFreeEncoder(enc); }
    }
    if (!crtc_id && res->count_crtcs > 0) crtc_id = res->crtcs[0];
    printf("[ok] crtc id: %u\n", crtc_id);

    struct drm_mode_create_dumb creq = {0};
    creq.width  = mode.hdisplay;
    creq.height = mode.vdisplay;
    creq.bpp    = 32;
    if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
        perror("[fail] create dumb");
        return 1;
    }
    printf("[ok] dumb buffer: %ux%u pitch=%u size=%u\n",
           creq.width, creq.height, creq.pitch, creq.size);

    uint32_t fb_id = 0;
    if (drmModeAddFB(fd, creq.width, creq.height, 24, 32,
                     creq.pitch, creq.handle, &fb_id) < 0) {
        perror("[fail] drmModeAddFB");
        return 1;
    }
    printf("[ok] framebuffer id: %u\n", fb_id);

    struct drm_mode_map_dumb mreq = {0};
    mreq.handle = creq.handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
        perror("[fail] map dumb");
        return 1;
    }

    uint32_t *map = mmap(NULL, creq.size, PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, mreq.offset);
    if (map == MAP_FAILED) { perror("[fail] mmap"); return 1; }
    printf("[ok] mmap at %p\n", map);

    /* 填充颜色：XRGB8888，像素 = 0x00RRGGBB */
    uint32_t color;
    const char *name;
    const char *c = getenv("DRM_COLOR");
    if (c && strcmp(c, "green") == 0)      { color = 0x0000FF00; name = "GREEN"; }
    else if (c && strcmp(c, "blue") == 0)  { color = 0x000000FF; name = "BLUE"; }
    else if (c && strcmp(c, "white") == 0) { color = 0x00FFFFFF; name = "WHITE"; }
    else                                    { color = 0x00FF0000; name = "RED"; }

    size_t n = creq.size / 4;
    for (size_t i = 0; i < n; i++) map[i] = color;
    printf("[ok] filled with %s (0x%08X)\n", name, color);

    if (drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0,
                       &conn->connector_id, 1, &mode) < 0) {
        perror("[fail] drmModeSetCrtc");
        return 1;
    }
    printf("[ok] drmModeSetCrtc\n");
    printf("=== Screen should be %s for 60s ===\n", name);
    fflush(stdout);

    sleep(60);

    munmap(map, creq.size);
    drmModeRmFB(fd, fb_id);
    struct drm_mode_destroy_dumb dreq = {0};
    dreq.handle = creq.handle;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    drmDropMaster(fd);
    close(fd);
    printf("=== DRM test done ===\n");
    return 0;
}
