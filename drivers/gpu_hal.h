#ifndef BRIGHTS_GPU_HAL_H
#define BRIGHTS_GPU_HAL_H

#include <stdint.h>
#include "../include/kernel/stddef.h"

#define GPU_MAX_DEVICES 4
#define GPU_MAX_QUEUES 16
#define GPU_MAX_MEMORY_TYPES 8
#define GPU_MAX_MEMORY_HEAPS 4

typedef enum {
    GPU_TYPE_UNKNOWN = 0,
    GPU_TYPE_INTEGRATED,
    GPU_TYPE_DISCRETE,
    GPU_TYPE_VIRTUAL,
    GPU_TYPE_CPU
} gpu_type_t;

typedef enum {
    GPU_FAMILY_UNKNOWN = 0,
    GPU_FAMILY_INTEL,
    GPU_FAMILY_AMD,
    GPU_FAMILY_NVIDIA,
    GPU_FAMILY_VIRTIO,
    GPU_FAMILY_SOFTWARE
} gpu_family_t;

typedef enum {
    GPU_FEAT_3D = 1 << 0,
    GPU_FEAT_2D = 1 << 1,
    GPU_FEAT_COMPUTE = 1 << 2,
    GPU_FEAT_VIDEO_DECODE = 1 << 3,
    GPU_FEAT_VIDEO_ENCODE = 1 << 4,
    GPU_FEAT_VULKAN = 1 << 5,
    GPU_FEAT_OPENGL = 1 << 6,
    GPU_FEAT_DX12 = 1 << 7,
    GPU_FEAT_TESSELATION = 1 << 8,
    GPU_FEAT_GEOMETRY_SHADER = 1 << 9,
    GPU_FEAT_MULTI_GPU = 1 << 10,
    GPU_FEAT_FP16 = 1 << 11
} gpu_feature_t;

typedef enum {
    GPU_QUEUE_GRAPHICS = 0,
    GPU_QUEUE_COMPUTE,
    GPU_QUEUE_TRANSFER,
    GPU_QUEUE_VIDEO
} gpu_queue_type_t;

typedef enum {
    GPU_MEMORY_HOST_VISIBLE = 1 << 0,
    GPU_MEMORY_DEVICE_LOCAL = 1 << 1,
    GPU_MEMORY_CACHED = 1 << 2,
    GPU_MEMORY_LAZILY_ALLOCATED = 1 << 3,
    GPU_MEMORY_PROTECTED = 1 << 4
} gpu_memory_flags_t;

typedef struct {
    uint64_t size;
    uint64_t base_address;
    uint32_t pci_bus;
    uint32_t pci_device;
    uint32_t pci_function;
    uint64_t vram_size;
    uint64_t aperture_size;
} gpu_pci_info_t;

typedef struct {
    uint32_t memory_type_bits;
    uint64_t heap_size;
    int supports_dma;
} gpu_memory_heap_t;

typedef struct {
    uint32_t memory_type;
    uint64_t property_flags;
    uint64_t heap_index;
} gpu_memory_type_t;

typedef struct {
    gpu_type_t type;
    gpu_family_t family;
    char name[128];
    char vendor[64];
    uint32_t driver_version;
    uint64_t features;
    uint64_t limits;
    uint32_t queue_family_count;
    gpu_memory_type_t memory_types[GPU_MAX_MEMORY_TYPES];
    gpu_memory_heap_t memory_heaps[GPU_MAX_MEMORY_HEAPS];
    uint32_t memory_type_count;
    uint32_t memory_heap_count;
    gpu_pci_info_t pci_info;
    void *private_data;
} gpu_device_info_t;

typedef struct {
    uint32_t queue_family_index;
    gpu_queue_type_t type;
    uint32_t queue_count;
    float priority;
} gpu_queue_family_t;

typedef struct {
    uint64_t handle;
    uint32_t size;
    uint32_t alignment;
    void *mapped_ptr;
    gpu_memory_flags_t flags;
    void *private_data;
} gpu_memory_t;

typedef struct {
    uint64_t handle;
    uint32_t size;
    void *mapped_ptr;
} gpu_buffer_t;

typedef struct {
    uint64_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    uint32_t array_layers;
    uint32_t format;
    uint32_t samples;
    gpu_memory_t *memory;
} gpu_image_t;

typedef enum {
    GPU_OPAQUE_BUFFER,
    GPU_STAGING_BUFFER,
    GPU_UNIFORM_BUFFER,
    GPU_STORAGE_BUFFER,
    GPU_INDEX_BUFFER,
    GPU_VERTEX_BUFFER,
    GPU_INDIRECT_BUFFER
} gpu_buffer_type_t;

typedef enum {
    GPU_IMAGE_1D,
    GPU_IMAGE_2D,
    GPU_IMAGE_3D,
    GPU_IMAGE_CUBE
} gpu_image_type_t;

typedef enum {
    GPU_IMAGE_OPTIMAL,
    GPU_IMAGE_LINEAR
} gpu_image_layout_t;

typedef struct {
    uint32_t signal_semaphore_count;
    uint32_t wait_semaphore_count;
    uint32_t signal_fence_count;
    uint32_t wait_fence_count;
} gpu_submit_info_t;

typedef struct {
    uint64_t handle;
    uint32_t max_closures;
    void *private_data;
} gpu_fence_t;

typedef struct {
    uint64_t handle;
    void *private_data;
} gpu_semaphore_t;

typedef struct {
    void (*pfn_signal)(void *, uint64_t);
    void (*pfn_wait)(void *, uint64_t, uint64_t);
    void *user_data;
} gpu_wsi_platform_t;

int gpu_hal_init(void);
void gpu_hal_shutdown(void);
int gpu_enumerate_devices(gpu_device_info_t *devices, uint32_t *count);
int gpu_select_device(uint32_t index);
gpu_device_info_t *gpu_get_selected_device(void);

int gpu_memory_allocate(gpu_memory_t *mem, uint64_t size, gpu_memory_flags_t flags);
void gpu_memory_free(gpu_memory_t *mem);
int gpu_memory_map(gpu_memory_t *mem, uint64_t offset, uint64_t size, void **ptr);
void gpu_memory_unmap(gpu_memory_t *mem);

int gpu_buffer_create(gpu_buffer_t *buffer, uint64_t size, gpu_buffer_type_t type, gpu_memory_flags_t flags);
void gpu_buffer_destroy(gpu_buffer_t *buffer);
int gpu_buffer_bind_memory(gpu_buffer_t *buffer, gpu_memory_t *mem, uint64_t offset);
void *gpu_buffer_map(gpu_buffer_t *buffer, uint64_t offset, uint64_t size);
void gpu_buffer_unmap(gpu_buffer_t *buffer);

int gpu_image_create(gpu_image_t *image, uint32_t width, uint32_t height, uint32_t depth, 
                    uint32_t format, uint32_t samples, gpu_image_type_t type, gpu_memory_flags_t flags);
void gpu_image_destroy(gpu_image_t *image);
int gpu_image_bind_memory(gpu_image_t *image, gpu_memory_t *mem, uint64_t offset);

int gpu_fence_create(gpu_fence_t *fence);
void gpu_fence_destroy(gpu_fence_t *fence);
int gpu_fence_wait(gpu_fence_t *fence, uint64_t timeout_ns);
int gpu_fence_reset(gpu_fence_t *fence);

int gpu_semaphore_create(gpu_semaphore_t *sem);
void gpu_semaphore_destroy(gpu_semaphore_t *sem);

void *gpu_get_device_extension(const char *name);

#endif
