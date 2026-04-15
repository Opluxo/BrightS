#include "gpu_hal.h"
#include "../kernel/kmalloc.h"
#include "fb.h"
#include <string.h>

static gpu_device_info_t devices[GPU_MAX_DEVICES];
static uint32_t device_count = 0;
static uint32_t selected_device = 0;
static int hal_initialized = 0;

static gpu_memory_t *all_memory = 0;
static uint32_t memory_count = 0;
static uint32_t max_memory = 256;

typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    gpu_family_t family;
    const char *name;
    const char *vendor;
} gpu_device_db_entry_t;

static const gpu_device_db_entry_t device_database[] = {
    {0x1AF4, 0x1050, GPU_FAMILY_VIRTIO, "VirtIO GPU", "Red Hat"},
    {0x1AF4, 0x1051, GPU_FAMILY_VIRTIO, "VirtIO GPU (1.0)", "Red Hat"},
    {0x8086, 0x0046, GPU_FAMILY_INTEL, "Intel HD Graphics", "Intel"},
    {0x8086, 0x0116, GPU_FAMILY_INTEL, "Intel HD Graphics 3000", "Intel"},
    {0x1002, 0x687F, GPU_FAMILY_AMD, "AMD Radeon RX 480", "AMD"},
    {0x10DE, 0x1B80, GPU_FAMILY_NVIDIA, "NVIDIA GeForce GTX 1070", "NVIDIA"},
    {0x0000, 0x0000, GPU_FAMILY_SOFTWARE, "Software Renderer", "BrightS"}
};

static gpu_family_t detect_gpu_family(uint32_t vendor_id, uint32_t device_id)
{
    for (int i = 0; i < sizeof(device_database) / sizeof(device_database[0]); i++) {
        if (device_database[i].vendor_id == vendor_id) {
            if (vendor_id == 0x1AF4) return GPU_FAMILY_VIRTIO;
            if (vendor_id == 0x8086) return GPU_FAMILY_INTEL;
            if (vendor_id == 0x1002) return GPU_FAMILY_AMD;
            if (vendor_id == 0x10DE) return GPU_FAMILY_NVIDIA;
        }
    }
    return GPU_FAMILY_SOFTWARE;
}

static const char *get_family_name(gpu_family_t family)
{
    switch (family) {
        case GPU_FAMILY_INTEL: return "Intel";
        case GPU_FAMILY_AMD: return "AMD";
        case GPU_FAMILY_NVIDIA: return "NVIDIA";
        case GPU_FAMILY_VIRTIO: return "VirtIO";
        case GPU_FAMILY_SOFTWARE: return "Software";
        default: return "Unknown";
    }
}

static void setup_software_renderer(gpu_device_info_t *dev)
{
    dev->type = GPU_TYPE_CPU;
    dev->family = GPU_FAMILY_SOFTWARE;
    strncpy(dev->name, "BrightS Software Renderer", sizeof(dev->name) - 1);
    strncpy(dev->vendor, "BrightS", sizeof(dev->vendor) - 1);
    dev->driver_version = 0x00010000;
    dev->features = GPU_FEAT_3D | GPU_FEAT_2D | GPU_FEAT_COMPUTE | GPU_FEAT_TESSELATION | GPU_FEAT_GEOMETRY_SHADER | GPU_FEAT_FP16;
    dev->limits = 65536;
    dev->queue_family_count = 3;
    dev->memory_type_count = 2;
    dev->memory_heap_count = 2;
    
    dev->memory_types[0].memory_type = 0;
    dev->memory_types[0].property_flags = GPU_MEMORY_HOST_VISIBLE | GPU_MEMORY_CACHED;
    dev->memory_types[0].heap_index = 0;
    
    dev->memory_types[1].memory_type = 1;
    dev->memory_types[1].property_flags = GPU_MEMORY_DEVICE_LOCAL;
    dev->memory_types[1].heap_index = 1;
    
    dev->memory_heaps[0].heap_size = 256 * 1024 * 1024;
    dev->memory_heaps[0].memory_type_bits = 1;
    dev->memory_heaps[0].supports_dma = 0;
    
    dev->memory_heaps[1].heap_size = 512 * 1024 * 1024;
    dev->memory_heaps[1].memory_type_bits = 2;
    dev->memory_heaps[1].supports_dma = 1;
}

int gpu_hal_init(void)
{
    if (hal_initialized) return 0;
    
    device_count = 0;
    memset(devices, 0, sizeof(devices));
    
    if (brights_fb_available()) {
        gpu_device_info_t *dev = &devices[device_count++];
        dev->type = GPU_TYPE_INTEGRATED;
        dev->family = GPU_FAMILY_SOFTWARE;
        strncpy(dev->name, "UEFI Framebuffer", sizeof(dev->name) - 1);
        strncpy(dev->vendor, "UEFI GOP", sizeof(dev->vendor) - 1);
        dev->driver_version = 0x00010000;
        dev->features = GPU_FEAT_2D | GPU_FEAT_VULKAN;
        dev->limits = 4096;
        dev->queue_family_count = 1;
        dev->memory_type_count = 1;
        dev->memory_heap_count = 1;
        dev->memory_types[0].memory_type = 0;
        dev->memory_types[0].property_flags = GPU_MEMORY_HOST_VISIBLE | GPU_MEMORY_DEVICE_LOCAL;
        dev->memory_types[0].heap_index = 0;
        dev->memory_heaps[0].heap_size = 64 * 1024 * 1024;
        dev->memory_heaps[0].memory_type_bits = 1;
        dev->memory_heaps[0].supports_dma = 0;
    }
    
    gpu_device_info_t *sw_dev = &devices[device_count++];
    setup_software_renderer(sw_dev);
    
    all_memory = (gpu_memory_t *)brights_kmalloc(max_memory * sizeof(gpu_memory_t));
    if (!all_memory) return -1;
    memset(all_memory, 0, max_memory * sizeof(gpu_memory_t));
    
    hal_initialized = 1;
    return 0;
}

void gpu_hal_shutdown(void)
{
    if (!hal_initialized) return;
    
    for (uint32_t i = 0; i < memory_count; i++) {
        if (all_memory[i].mapped_ptr) {
            gpu_memory_unmap(&all_memory[i]);
        }
    }
    
    if (all_memory) {
        brights_kfree(all_memory);
        all_memory = 0;
    }
    
    memory_count = 0;
    hal_initialized = 0;
}

int gpu_enumerate_devices(gpu_device_info_t *devs, uint32_t *count)
{
    if (!hal_initialized) gpu_hal_init();
    
    uint32_t n = device_count < *count ? device_count : *count;
    for (uint32_t i = 0; i < n; i++) {
        memcpy(&devs[i], &devices[i], sizeof(gpu_device_info_t));
    }
    *count = device_count;
    return 0;
}

int gpu_select_device(uint32_t index)
{
    if (index >= device_count) return -1;
    selected_device = index;
    return 0;
}

gpu_device_info_t *gpu_get_selected_device(void)
{
    if (!hal_initialized || selected_device >= device_count) return 0;
    return &devices[selected_device];
}

int gpu_memory_allocate(gpu_memory_t *mem, uint64_t size, gpu_memory_flags_t flags)
{
    if (!mem || !hal_initialized) return -1;
    
    mem->size = size;
    mem->alignment = 4096;
    mem->flags = flags;
    mem->mapped_ptr = 0;
    mem->private_data = 0;
    
    size = (size + 4095) & ~4095ULL;
    
    mem->mapped_ptr = brights_kmalloc((size_t)size);
    if (!mem->mapped_ptr) return -1;
    memset(mem->mapped_ptr, 0, (size_t)size);
    
    if (memory_count < max_memory) {
        memcpy(&all_memory[memory_count], mem, sizeof(gpu_memory_t));
        memory_count++;
    }
    
    return 0;
}

void gpu_memory_free(gpu_memory_t *mem)
{
    if (!mem) return;
    if (mem->mapped_ptr) {
        brights_kfree(mem->mapped_ptr);
        mem->mapped_ptr = 0;
    }
    mem->size = 0;
}

int gpu_memory_map(gpu_memory_t *mem, uint64_t offset, uint64_t size, void **ptr)
{
    if (!mem || !ptr) return -1;
    *ptr = (uint8_t *)mem->mapped_ptr + offset;
    return 0;
}

void gpu_memory_unmap(gpu_memory_t *mem)
{
}

int gpu_buffer_create(gpu_buffer_t *buffer, uint64_t size, gpu_buffer_type_t type, gpu_memory_flags_t flags)
{
    if (!buffer) return -1;
    
    buffer->size = size;
    buffer->mapped_ptr = 0;
    
    size_t alloc_size = (size_t)((size + 4095) & ~4095ULL);
    buffer->mapped_ptr = brights_kmalloc(alloc_size);
    if (!buffer->mapped_ptr) return -1;
    memset(buffer->mapped_ptr, 0, alloc_size);
    
    return 0;
}

void gpu_buffer_destroy(gpu_buffer_t *buffer)
{
    if (!buffer) return;
    if (buffer->mapped_ptr) {
        brights_kfree(buffer->mapped_ptr);
        buffer->mapped_ptr = 0;
    }
    buffer->size = 0;
}

int gpu_buffer_bind_memory(gpu_buffer_t *buffer, gpu_memory_t *mem, uint64_t offset)
{
    if (!buffer || !mem) return -1;
    return 0;
}

void *gpu_buffer_map(gpu_buffer_t *buffer, uint64_t offset, uint64_t size)
{
    if (!buffer || !buffer->mapped_ptr) return 0;
    return (uint8_t *)buffer->mapped_ptr + offset;
}

void gpu_buffer_unmap(gpu_buffer_t *buffer)
{
}

int gpu_image_create(gpu_image_t *image, uint32_t width, uint32_t height, uint32_t depth,
                    uint32_t format, uint32_t samples, gpu_image_type_t type, gpu_memory_flags_t flags)
{
    if (!image) return -1;
    
    image->width = width;
    image->height = height;
    image->depth = depth;
    image->format = format;
    image->samples = samples;
    image->memory = 0;
    
    uint32_t bytes_per_pixel = 4;
    if (format == 0x1907) bytes_per_pixel = 2;
    else if (format == 0x1908) bytes_per_pixel = 2;
    else if (format == 0x1905) bytes_per_pixel = 2;
    
    uint64_t size = (uint64_t)width * height * depth * bytes_per_pixel;
    size = (size + 4095) & ~4095ULL;
    
    image->memory = (gpu_memory_t *)brights_kmalloc(sizeof(gpu_memory_t));
    if (!image->memory) return -1;
    
    if (gpu_memory_allocate(image->memory, size, flags) != 0) {
        brights_kfree(image->memory);
        return -1;
    }
    
    return 0;
}

void gpu_image_destroy(gpu_image_t *image)
{
    if (!image) return;
    if (image->memory) {
        gpu_memory_free(image->memory);
        brights_kfree(image->memory);
        image->memory = 0;
    }
}

int gpu_image_bind_memory(gpu_image_t *image, gpu_memory_t *mem, uint64_t offset)
{
    if (!image || !mem) return -1;
    image->memory = mem;
    return 0;
}

int gpu_fence_create(gpu_fence_t *fence)
{
    if (!fence) return -1;
    fence->max_closures = 1;
    fence->private_data = 0;
    return 0;
}

void gpu_fence_destroy(gpu_fence_t *fence)
{
    if (!fence) return;
}

int gpu_fence_wait(gpu_fence_t *fence, uint64_t timeout_ns)
{
    if (!fence) return -1;
    return 0;
}

int gpu_fence_reset(gpu_fence_t *fence)
{
    if (!fence) return -1;
    return 0;
}

int gpu_semaphore_create(gpu_semaphore_t *sem)
{
    if (!sem) return -1;
    sem->private_data = 0;
    return 0;
}

void gpu_semaphore_destroy(gpu_semaphore_t *sem)
{
    if (!sem) return;
}

void *gpu_get_device_extension(const char *name)
{
    return 0;
}
