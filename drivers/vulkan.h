#ifndef BRIGHTS_VULKAN_H
#define BRIGHTS_VULKAN_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t VkInstance;
typedef uint64_t VkPhysicalDevice;
typedef uint64_t VkDevice;
typedef uint64_t VkQueue;
typedef uint64_t VkSemaphore;
typedef uint64_t VkFence;
typedef uint64_t VkDeviceMemory;
typedef uint64_t VkBuffer;
typedef uint64_t VkBufferView;
typedef uint64_t VkImage;
typedef uint64_t VkImageView;
typedef uint64_t VkShaderModule;
typedef uint64_t VkPipeline;
typedef uint64_t VkPipelineCache;
typedef uint64_t VkPipelineLayout;
typedef uint64_t VkRenderPass;
typedef uint64_t VkFramebuffer;
typedef uint64_t VkCommandPool;
typedef uint64_t VkCommandBuffer;
typedef uint64_t VkDescriptorPool;
typedef uint64_t VkDescriptorSet;
typedef uint64_t VkDescriptorSetLayout;
typedef uint64_t VkSampler;
typedef uint64_t VkSurfaceKHR;
typedef uint64_t VkSwapchainKHR;

typedef uint32_t VkResult;
typedef uint32_t VkBool32;
typedef uint32_t VkDeviceSize;
typedef uint64_t VkSampleMask;

#define VK_SUCCESS 0
#define VK_NOT_READY 1
#define VK_TIMEOUT 2
#define VK_ERROR_OUT_OF_HOST_MEMORY ((VkResult)-1)
#define VK_ERROR_OUT_OF_DEVICE_MEMORY ((VkResult)-2)
#define VK_ERROR_INITIALIZATION_FAILED ((VkResult)-3)
#define VK_ERROR_DEVICE_LOST ((VkResult)-4)
#define VK_ERROR_OUT_OF_POOL_MEMORY ((VkResult)-7)
#define VK_TRUE 1
#define VK_FALSE 0

#define VK_API_VERSION_1_0 4194304
#define VK_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))

typedef enum {
    VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
    VK_PRESENT_MODE_MAILBOX_KHR = 1,
    VK_PRESENT_MODE_FIFO_KHR = 2,
    VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3
} VkPresentModeKHR;

typedef enum {
    VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    VK_STRUCTURE_TYPE_SUBMIT_INFO,
    VK_STRUCTURE_TYPE_DEVICE_MEMORY_ALLOCATE_INFO,
    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
    VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO,
    VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
    VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
    VK_STRUCTURE_TYPE_EXPORT_FENCE_FD_INFO_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES_KHR
} VkStructureType;

typedef enum {
    VK_FORMAT_UNDEFINED = 0,
    VK_FORMAT_R4G4_UNORM_PACK8 = 1,
    VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
    VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
    VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
    VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
    VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
    VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
    VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
    VK_FORMAT_R8_UNORM = 9,
    VK_FORMAT_R8_SNORM = 10,
    VK_FORMAT_R8_UINT = 13,
    VK_FORMAT_R8_SINT = 14,
    VK_FORMAT_R8G8_UNORM = 16,
    VK_FORMAT_R8G8_SNORM = 17,
    VK_FORMAT_R8G8_UINT = 18,
    VK_FORMAT_R8G8_SINT = 19,
    VK_FORMAT_R8G8B8A8_UNORM = 37,
    VK_FORMAT_R8G8B8A8_SNORM = 38,
    VK_FORMAT_R8G8B8A8_UINT = 30,
    VK_FORMAT_B8G8R8A8_UNORM = 44,
    VK_FORMAT_R16_SFLOAT = 76,
    VK_FORMAT_R16G16_SFLOAT = 81,
    VK_FORMAT_R16G16B16A16_SFLOAT = 89,
    VK_FORMAT_R32_SINT = 91,
    VK_FORMAT_R32_SFLOAT = 92,
    VK_FORMAT_R32G32_SFLOAT = 95,
    VK_FORMAT_R32G32B32_SFLOAT = 98,
    VK_FORMAT_R32G32B32A32_SFLOAT = 101,
    VK_FORMAT_D16_UNORM = 70,
    VK_FORMAT_D32_SFLOAT = 126,
    VK_FORMAT_S8_UINT = 127,
    VK_FORMAT_D16_UNORM_S8_UINT = 128,
    VK_FORMAT_D24_UNORM_S8_UINT = 129,
    VK_FORMAT_D32_SFLOAT_S8_UINT = 130
} VkFormat;

typedef enum {
    VK_IMAGE_TYPE_1D = 0,
    VK_IMAGE_TYPE_2D = 1,
    VK_IMAGE_TYPE_3D = 2
} VkImageType;

typedef enum {
    VK_IMAGE_TILING_OPTIMAL = 0,
    VK_IMAGE_TILING_LINEAR = 1
} VkImageTiling;

typedef enum {
    VK_IMAGE_LAYOUT_UNDEFINED = 0,
    VK_IMAGE_LAYOUT_GENERAL = 1,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
    VK_IMAGE_LAYOUT_PREINITIALIZED = 8,
    VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002
} VkImageLayout;

typedef uint32_t VkImageUsageFlags;
enum {
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
    VK_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT = 0x00000040,
    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT = 0x00000080
};

typedef uint32_t VkBufferUsageFlags;
enum {
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
    VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080,
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT = 0x00000100
};

typedef enum {
    VK_SHARING_MODE_EXCLUSIVE = 0,
    VK_SHARING_MODE_CONCURRENT = 1
} VkSharingMode;

typedef enum {
    VK_DESCRIPTOR_TYPE_SAMPLER = 0,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10
} VkDescriptorType;

typedef uint32_t VkShaderStageFlags;
enum {
    VK_SHADER_STAGE_VERTEX_BIT = 0x00000001,
    VK_SHADER_STAGE_FRAGMENT_BIT = 0x00000080,
    VK_SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000010,
    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000020,
    VK_SHADER_STAGE_COMPUTE_BIT = 0x00000200,
    VK_SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
    VK_SHADER_STAGE_ALL = 0x7FFFFFFF
};

typedef uint32_t VkPipelineStageFlags;
enum {
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001,
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT = 0x00000002,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT = 0x00000004,
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT = 0x00000008,
    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT = 0x00000040,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080,
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = 0x00000200,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800,
    VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000,
    VK_PIPELINE_STAGE_HOST_BIT = 0x00004000,
    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT = 0x00008000,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT = 0x00010000
};

typedef enum {
    VK_ATTACHMENT_LOAD_OP_LOAD = 0,
    VK_ATTACHMENT_LOAD_OP_CLEAR = 1,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE = 2
} VkAttachmentLoadOp;

typedef enum {
    VK_ATTACHMENT_STORE_OP_STORE = 0,
    VK_ATTACHMENT_STORE_OP_DONT_CARE = 1
} VkAttachmentStoreOp;

typedef enum {
    VK_PIPELINE_BIND_POINT_GRAPHICS = 0,
    VK_PIPELINE_BIND_POINT_COMPUTE = 1
} VkPipelineBindPoint;

typedef enum {
    VK_SUBPASS_CONTENTS_INLINE = 0,
    VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS = 1
} VkSubpassContents;

typedef enum {
    VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0,
    VK_COMMAND_BUFFER_LEVEL_SECONDARY = 1
} VkCommandBufferLevel;

typedef uint32_t VkCullModeFlags;
enum {
    VK_CULL_MODE_NONE = 0,
    VK_CULL_MODE_FRONT_BIT = 0x00000001,
    VK_CULL_MODE_BACK_BIT = 0x00000002,
    VK_CULL_MODE_FRONT_AND_BACK = 0x00000003
};

typedef enum {
    VK_POLYGON_MODE_FILL = 0,
    VK_POLYGON_MODE_LINE = 1,
    VK_POLYGON_MODE_POINT = 2
} VkPolygonMode;

typedef enum {
    VK_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    VK_FRONT_FACE_CLOCKWISE = 1
} VkFrontFace;

typedef enum {
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    VK_PRIMITIVE_TOPOLOGY_PATCH_LIST = 10
} VkPrimitiveTopology;

typedef enum {
    VK_LOGIC_OP_CLEAR = 0,
    VK_LOGIC_OP_AND = 1,
    VK_LOGIC_OP_COPY = 3,
    VK_LOGIC_OP_NO_OP = 5,
    VK_LOGIC_OP_XOR = 6,
    VK_LOGIC_OP_OR = 7,
    VK_LOGIC_OP_SET = 15
} VkLogicOp;

typedef enum {
    VK_BLEND_OP_ADD = 0,
    VK_BLEND_OP_SUBTRACT = 1,
    VK_BLEND_OP_REVERSE_SUBTRACT = 2,
    VK_BLEND_OP_MIN = 3,
    VK_BLEND_OP_MAX = 4
} VkBlendOp;

typedef enum {
    VK_BLEND_FACTOR_ZERO = 0,
    VK_BLEND_FACTOR_ONE = 1,
    VK_BLEND_FACTOR_SRC_COLOR = 2,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
    VK_BLEND_FACTOR_DST_COLOR = 4,
    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
    VK_BLEND_FACTOR_SRC_ALPHA = 6,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
    VK_BLEND_FACTOR_DST_ALPHA = 8,
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    VK_BLEND_FACTOR_CONSTANT_COLOR = 10,
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14
} VkBlendFactor;

typedef uint32_t VkColorComponentFlags;
enum {
    VK_COLOR_COMPONENT_R_BIT = 1,
    VK_COLOR_COMPONENT_G_BIT = 2,
    VK_COLOR_COMPONENT_B_BIT = 4,
    VK_COLOR_COMPONENT_A_BIT = 8
};

typedef uint32_t VkSampleCountFlags;
enum {
    VK_SAMPLE_COUNT_1_BIT = 1,
    VK_SAMPLE_COUNT_2_BIT = 2,
    VK_SAMPLE_COUNT_4_BIT = 4,
    VK_SAMPLE_COUNT_8_BIT = 8,
    VK_SAMPLE_COUNT_16_BIT = 16
};
typedef uint32_t VkSampleCountFlagBits;

typedef enum {
    VK_COMPARE_OP_NEVER = 0,
    VK_COMPARE_OP_LESS = 1,
    VK_COMPARE_OP_EQUAL = 2,
    VK_COMPARE_OP_LESS_OR_EQUAL = 3,
    VK_COMPARE_OP_GREATER = 4,
    VK_COMPARE_OP_NOT_EQUAL = 5,
    VK_COMPARE_OP_GREATER_OR_EQUAL = 6,
    VK_COMPARE_OP_ALWAYS = 7
} VkCompareOp;

typedef enum {
    VK_STENCIL_OP_KEEP = 0,
    VK_STENCIL_OP_ZERO = 1,
    VK_STENCIL_OP_REPLACE = 2,
    VK_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
    VK_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
    VK_STENCIL_OP_INVERT = 5,
    VK_STENCIL_OP_INCREMENT_AND_WRAP = 6,
    VK_STENCIL_OP_DECREMENT_AND_WRAP = 7
} VkStencilOp;

typedef enum {
    VK_DYNAMIC_STATE_VIEWPORT = 0,
    VK_DYNAMIC_STATE_SCISSOR = 1,
    VK_DYNAMIC_STATE_LINE_WIDTH = 2,
    VK_DYNAMIC_STATE_DEPTH_BIAS = 3,
    VK_DYNAMIC_STATE_BLEND_CONSTANTS = 4,
    VK_DYNAMIC_STATE_DEPTH_BOUNDS = 5,
    VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK = 6,
    VK_DYNAMIC_STATE_STENCIL_WRITE_MASK = 7,
    VK_DYNAMIC_STATE_STENCIL_REFERENCE = 8
} VkDynamicState;

typedef uint32_t VkCompositeAlphaFlagsKHR;
enum {
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR = 1,
    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR = 2,
    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR = 4,
    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR = 8
};
typedef uint32_t VkCompositeAlphaFlagBitsKHR;

typedef enum {
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR = 0
} VkColorSpaceKHR;

typedef struct {
    VkFormat format;
    VkColorSpaceKHR colorSpace;
} VkSurfaceFormatKHR;

typedef struct {
    VkStructureType sType;
    const void *pNext;
} VkBaseInStructure;

typedef uint32_t VkMemoryPropertyFlags;
enum {
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 1,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 2,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 4,
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT = 8,
    VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = 16
};

typedef uint32_t VkAccessFlags;
enum {
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT = 1,
    VK_ACCESS_INDEX_READ_BIT = 2,
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT = 4,
    VK_ACCESS_UNIFORM_READ_BIT = 8,
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT = 16,
    VK_ACCESS_SHADER_READ_BIT = 32,
    VK_ACCESS_SHADER_WRITE_BIT = 64,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT = 128,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 256,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 512,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 1024,
    VK_ACCESS_TRANSFER_READ_BIT = 4096,
    VK_ACCESS_TRANSFER_WRITE_BIT = 8192,
    VK_ACCESS_HOST_READ_BIT = 16384,
    VK_ACCESS_HOST_WRITE_BIT = 32768
};

typedef uint32_t VkQueueFlags;
enum {
    VK_QUEUE_GRAPHICS_BIT = 1,
    VK_QUEUE_COMPUTE_BIT = 2,
    VK_QUEUE_TRANSFER_BIT = 4,
    VK_QUEUE_SPARSE_BINDING_BIT = 8
};

typedef uint32_t VkMemoryHeapFlags;
enum {
    VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = 1
};

typedef uint32_t VkMemoryAllocateFlags;

typedef uint32_t VkCommandPoolCreateFlags;
enum {
    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT = 1,
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 2
};

typedef uint32_t VkCommandPoolResetFlags;

typedef uint32_t VkCommandBufferUsageFlags;
enum {
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 1,
    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT = 2,
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT = 4
};

typedef uint32_t VkCommandBufferResetFlags;

typedef uint32_t VkDescriptorPoolResetFlags;

typedef uint32_t VkStencilFaceFlags;

typedef uint32_t VkDependencyFlags;
enum {
    VK_DEPENDENCY_BY_REGION_BIT = 1,
    VK_DEPENDENCY_DEVICE_GROUP_BIT = 4,
    VK_DEPENDENCY_VIEW_LOCAL_BIT = 2
};

typedef uint32_t VkPipelineCreateFlags;
enum {
    VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT = 1,
    VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT = 2,
    VK_PIPELINE_CREATE_DERIVATIVE_BIT = 4
};

typedef uint32_t VkSurfaceTransformFlagsKHR;
enum {
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR = 1,
    VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR = 2,
    VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR = 4,
    VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR = 8,
    VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR = 16,
    VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR = 256
};
typedef uint32_t VkSurfaceTransformFlagBitsKHR;

typedef enum {
    VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
    VK_PHYSICAL_DEVICE_TYPE_CPU = 4
} VkPhysicalDeviceType;

typedef enum {
    VK_FILTER_NEAREST = 0,
    VK_FILTER_LINEAR = 1
} VkFilter;

typedef enum {
    VK_SAMPLER_ADDRESS_MODE_REPEAT = 0,
    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3
} VkSamplerAddressMode;

typedef enum {
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
    VK_BORDER_COLOR_INT_TRANSPARENT_BLACK = 1,
    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK = 2,
    VK_BORDER_COLOR_INT_OPAQUE_BLACK = 3,
    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE = 4
} VkBorderColor;

typedef enum {
    VK_INDEX_TYPE_UINT16 = 0,
    VK_INDEX_TYPE_UINT32 = 1
} VkIndexType;

typedef uint32_t VkImageAspectFlags;
enum {
    VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
    VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
    VK_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
    VK_IMAGE_ASPECT_METADATA_BIT = 0x00000008
};

typedef enum {
    VK_COMPONENT_SWIZZLE_IDENTITY = 0,
    VK_COMPONENT_SWIZZLE_ZERO = 1,
    VK_COMPONENT_SWIZZLE_ONE = 2,
    VK_COMPONENT_SWIZZLE_R = 3,
    VK_COMPONENT_SWIZZLE_G = 4,
    VK_COMPONENT_SWIZZLE_B = 5,
    VK_COMPONENT_SWIZZLE_A = 6
} VkComponentSwizzle;

typedef enum {
    VK_IMAGE_VIEW_TYPE_1D = 0,
    VK_IMAGE_VIEW_TYPE_2D = 1,
    VK_IMAGE_VIEW_TYPE_3D = 2,
    VK_IMAGE_VIEW_TYPE_CUBE = 3,
    VK_IMAGE_VIEW_TYPE_1D_ARRAY = 4,
    VK_IMAGE_VIEW_TYPE_2D_ARRAY = 5
} VkImageViewType;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    const char *pApplicationName;
    uint32_t applicationVersion;
    const char *pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
} VkApplicationInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    const VkApplicationInfo *pApplicationInfo;
    uint32_t enabledLayerCount;
    const char *const *ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char *const *ppEnabledExtensionNames;
} VkInstanceCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    float queuePriority;
} VkDeviceQueueCreateInfo;

typedef struct {
    VkBool32 robustBufferAccess;
    VkBool32 fullDrawIndexUint32;
    VkBool32 geometryShader;
    VkBool32 tessellationShader;
    VkBool32 sampleRateShading;
    VkBool32 dualSrcBlend;
    VkBool32 logicOp;
    VkBool32 multiDrawIndirect;
    VkBool32 depthBiasClamp;
    VkBool32 fillModeNonSolid;
    VkBool32 depthBounds;
    VkBool32 wideLines;
    VkBool32 largePoints;
    VkBool32 alphaToOne;
    VkBool32 multiViewport;
    VkBool32 samplerAnisotropy;
    VkBool32 textureCompressionETC2;
    VkBool32 textureCompressionASTC_LDR;
    VkBool32 textureCompressionBC;
    VkBool32 occlusionQueryPrecise;
    VkBool32 pipelineStatisticsQuery;
    VkBool32 vertexPipelineStoresAndAtomics;
    VkBool32 fragmentStoresAndAtomics;
    VkBool32 shaderClipDistance;
    VkBool32 shaderCullDistance;
    VkBool32 sparseBinding;
    VkBool32 variableMultisampleRate;
} VkPhysicalDeviceFeatures;

typedef struct {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    VkPhysicalDeviceType deviceType;
    char deviceName[256];
    uint8_t pipelineCacheUUID[16];
    uint32_t limits_maxImageDimension1D;
    uint32_t limits_maxImageDimension2D;
    uint32_t limits_maxImageDimension3D;
    uint32_t limits_maxImageDimensionCube;
    uint32_t limits_maxImageArrayLayers;
    uint32_t limits_maxUniformBufferRange;
    uint32_t limits_maxStorageBufferRange;
    uint32_t limits_maxPushConstantsSize;
    uint32_t limits_maxBoundDescriptorSets;
    uint32_t limits_maxPerStageDescriptorSamplers;
    uint32_t limits_maxPerStageDescriptorUniformBuffers;
    uint32_t limits_maxPerStageDescriptorStorageBuffers;
    uint32_t limits_maxPerStageDescriptorSampledImages;
    uint32_t limits_maxPerStageDescriptorStorageImages;
    uint32_t limits_maxVertexInputBindings;
    uint32_t limits_maxVertexInputAttributes;
    uint32_t limits_maxVertexInputAttributeOffset;
    uint32_t limits_maxVertexInputBindingStride;
    uint32_t limits_maxFragmentInputComponents;
    uint32_t limits_maxColorAttachments;
    uint32_t limits_maxClipDistances;
    uint32_t limits_maxCullDistances;
    uint32_t limits_maxViewports;
    uint32_t limits_maxViewportDimensions[2];
    float limits_viewportBoundsRange[2];
    uint32_t limits_minMemoryMapAlignment;
    VkDeviceSize limits_minUniformBufferOffsetAlignment;
    float limits_lineWidthRange[2];
    VkSampleCountFlags limits_framebufferColorSampleCounts;
    VkSampleCountFlags limits_framebufferDepthSampleCounts;
} VkPhysicalDeviceProperties;

typedef struct {
    VkQueueFlags queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    VkDeviceSize minImageTransferGranularity;
} VkQueueFamilyProperties;

typedef struct {
    uint32_t memoryTypeCount;
    uint32_t memoryHeapCount;
} VkPhysicalDeviceMemoryProperties;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t queueCreateInfoCount;
    const VkDeviceQueueCreateInfo *pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char *const *ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char *const *ppEnabledExtensionNames;
    const VkPhysicalDeviceFeatures *pEnabledFeatures;
} VkDeviceCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkMemoryAllocateFlags flags;
} VkMemoryAllocateFlagsInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize allocationSize;
    uint32_t memoryTypeIndex;
} VkMemoryAllocateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t queueFamilyIndex;
    VkCommandPoolCreateFlags flags;
} VkCommandPoolCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkCommandBufferLevel level;
    VkCommandBufferUsageFlags flags;
} VkCommandBufferAllocateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkPipelineBindPoint pipelineBindPoint;
    uint32_t subpass;
    uint32_t occlusionQueryEnable;
    uint32_t queryFlags;
    uint32_t pipelineStatistics;
} VkCommandBufferInheritanceInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkCommandBufferUsageFlags flags;
    const VkCommandBufferInheritanceInfo *pInheritanceInfo;
} VkCommandBufferBeginInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore *pWaitSemaphores;
    const VkPipelineStageFlags *pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
    uint32_t signalSemaphoreCount;
    const VkSemaphore *pSignalSemaphores;
} VkSubmitInfo;

typedef struct {
    uint32_t attachment;
    VkImageLayout layout;
} VkAttachmentReference;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
    VkAttachmentLoadOp stencilLoadOp;
    VkAttachmentStoreOp stencilStoreOp;
    VkImageLayout initialLayout;
    VkImageLayout finalLayout;
} VkAttachmentDescription;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkPipelineBindPoint pipelineBindPoint;
    uint32_t inputAttachmentCount;
    const VkAttachmentReference *pInputAttachments;
    uint32_t colorAttachmentCount;
    const VkAttachmentReference *pColorAttachments;
    const VkAttachmentReference *pResolveAttachments;
    const VkAttachmentReference *pDepthStencilAttachment;
    uint32_t preserveAttachmentCount;
    const uint32_t *pPreserveAttachments;
} VkSubpassDescription;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t srcSubpass;
    uint32_t dstSubpass;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkDependencyFlags dependencyFlags;
} VkSubpassDependency;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t attachmentCount;
    const VkAttachmentDescription *pAttachments;
    uint32_t subpassCount;
    const VkSubpassDescription *pSubpasses;
    uint32_t dependencyCount;
    const VkSubpassDependency *pDependencies;
} VkRenderPassCreateInfo;

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} VkClearRect;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkRenderPass renderPass;
    uint32_t contentX;
    uint32_t contentY;
    uint32_t contentWidth;
    uint32_t contentHeight;
    uint32_t layerCount;
    uint32_t clearRectCount;
    const VkClearRect *pClearRects;
} VkRenderPassBeginInfo;

typedef struct {
    VkStencilOp failOp;
    VkStencilOp passOp;
    VkStencilOp depthFailOp;
    VkCompareOp compareOp;
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
} VkStencilOpState;

typedef struct {
    VkBool32 depthTestEnable;
    VkBool32 depthWriteEnable;
    VkCompareOp depthCompareOp;
    VkBool32 depthBoundsTestEnable;
    VkBool32 stencilTestEnable;
    VkStencilOpState front;
    VkStencilOpState back;
    float minDepthBounds;
    float maxDepthBounds;
} VkPipelineDepthStencilStateCreateInfo;

typedef struct {
    float x, y, width, height, minDepth, maxDepth;
} VkViewport;

typedef struct {
    int32_t x, y;
    uint32_t width, height;
} VkRect2D;

typedef struct {
    VkBool32 blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp colorBlendOp;
    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;
    VkBlendOp alphaBlendOp;
    VkColorComponentFlags colorWriteMask;
} VkPipelineColorBlendAttachmentState;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkBool32 logicOpEnable;
    VkLogicOp logicOp;
    uint32_t attachmentCount;
    const VkPipelineColorBlendAttachmentState *pAttachments;
    float blendConstants[4];
} VkPipelineColorBlendStateCreateInfo;

typedef struct {
    uint32_t binding;
    uint32_t stride;
    uint32_t inputRate;
} VkVertexInputBindingDescription;

typedef struct {
    uint32_t location;
    uint32_t binding;
    VkFormat format;
    uint32_t offset;
} VkVertexInputAttributeDescription;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t vertexBindingDescriptionCount;
    const VkVertexInputBindingDescription *pVertexBindingDescriptions;
    uint32_t vertexAttributeDescriptionCount;
    const VkVertexInputAttributeDescription *pVertexAttributeDescriptions;
} VkPipelineVertexInputStateCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkPrimitiveTopology topology;
    VkBool32 primitiveRestartEnable;
} VkPipelineInputAssemblyStateCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t viewportCount;
    const VkViewport *pViewports;
    uint32_t scissorCount;
    const VkRect2D *pScissors;
} VkPipelineViewportStateCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkBool32 depthClampEnable;
    VkBool32 rasterizerDiscardEnable;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    VkBool32 depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
    float lineWidth;
} VkPipelineRasterizationStateCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkBool32 sampleShadingEnable;
    VkSampleCountFlagBits rasterizationSamples;
    float minSampleShading;
    const VkSampleMask *pSampleMask;
    VkBool32 alphaToCoverageEnable;
    VkBool32 alphaToOneEnable;
} VkPipelineMultisampleStateCreateInfo;

typedef struct {
    uint32_t binding;
    uint32_t descriptorType;
    uint32_t descriptorCount;
    VkShaderStageFlags stageFlags;
    const void *pImmutableSamplers;
} VkDescriptorSetLayoutBinding;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t bindingCount;
    const VkDescriptorSetLayoutBinding *pBindings;
} VkDescriptorSetLayoutCreateInfo;

typedef struct {
    uint32_t offset;
    uint32_t size;
} VkPushConstantRange;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t setLayoutCount;
    const VkDescriptorSetLayout *pSetLayouts;
    uint32_t pushConstantRangeCount;
    const VkPushConstantRange *pPushConstantRanges;
} VkPipelineLayoutCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    VkShaderStageFlags stage;
    VkShaderModule module;
    const char *pName;
    const void *pSpecializationInfo;
} VkPipelineShaderStageCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t stageCount;
    const VkPipelineShaderStageCreateInfo *pStages;
    const VkPipelineVertexInputStateCreateInfo *pVertexInputState;
    const VkPipelineInputAssemblyStateCreateInfo *pInputAssemblyState;
    const void *pTessellationState;
    const VkPipelineViewportStateCreateInfo *pViewportState;
    const VkPipelineRasterizationStateCreateInfo *pRasterizationState;
    const VkPipelineMultisampleStateCreateInfo *pMultisampleState;
    const VkPipelineDepthStencilStateCreateInfo *pDepthStencilState;
    const VkPipelineColorBlendStateCreateInfo *pColorBlendState;
    const void *pDynamicState;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
} VkGraphicsPipelineCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t stageCount;
    const VkPipelineShaderStageCreateInfo *pStages;
    VkPipelineLayout layout;
} VkComputePipelineCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t flags;
    uint32_t shaderModuleCreateInfoCount;
    const void *pShaderModuleCreateInfos;
} VkPipelineCacheCreateInfo;

typedef struct {
    uint32_t width;
    uint32_t height;
} VkExtent2D;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkPresentModeKHR presentMode;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkCompositeAlphaFlagBitsKHR compositeAlpha;
    VkBool32 clipped;
    VkSwapchainKHR oldSwapchain;
} VkSwapchainCreateInfoKHR;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t minImageCount;
    uint32_t maxImageCount;
    VkExtent2D currentExtent;
    uint32_t minImageExtent;
    uint32_t maxImageExtent;
    uint32_t maxImageArrayLayers;
    VkSurfaceTransformFlagsKHR supportedTransforms;
    VkSurfaceTransformFlagBitsKHR currentTransform;
    VkCompositeAlphaFlagsKHR supportedCompositeAlpha;
    VkImageUsageFlags supportedUsageFlags;
} VkSurfaceCapabilities2KHR;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore *pWaitSemaphores;
    const VkPipelineStageFlags *pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
    uint32_t signalSemaphoreCount;
    const VkSemaphore *pSignalSemaphores;
} VkTimelineSemaphoreSubmitInfoKHR;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} VkOffset3D;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} VkExtent3D;

typedef struct {
    VkComponentSwizzle r;
    VkComponentSwizzle g;
    VkComponentSwizzle b;
    VkComponentSwizzle a;
} VkComponentMapping;

typedef struct {
    VkImageAspectFlags aspectMask;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
} VkImageSubresourceRange;

typedef struct {
    uint32_t aspectMask;
    uint32_t mipLevel;
} VkImageSubresource;

typedef struct {
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceSize rowLength;
    VkDeviceSize arrayPitch;
    VkDeviceSize depthPitch;
} VkSubresourceLayout;

typedef struct {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;
} VkClearColorValue;

typedef struct {
    float depth;
    uint32_t stencil;
} VkClearDepthStencilValue;

typedef union {
    VkClearColorValue color;
    VkClearDepthStencilValue depthStencil;
} VkClearValue;

typedef struct {
    uint32_t buffer;
    uint32_t offset;
    uint32_t stride;
} VkBufferCopy;

typedef struct {
    VkImageAspectFlags aspectMask;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
} VkImageSubresourceLayers;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const void *pRegions;
} VkImageCopy;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkBuffer dstBuffer;
    VkDeviceSize dstOffset;
    VkDeviceSize dstRowLength;
    VkDeviceSize dstImageHeight;
    VkImageSubresourceLayers dstSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VkBufferImageCopy;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkImage imageType;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
    VkImageLayout initialLayout;
} VkImageCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkImageViewType viewType;
    VkFormat format;
    VkComponentMapping components;
    VkImageSubresourceRange subresourceRange;
} VkImageViewCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
    VkSharingMode sharingMode;
} VkImageFormatListCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    uint32_t maxSets;
    uint32_t poolSizeCount;
    const void *pPoolSizes;
} VkDescriptorPoolCreateInfo;

typedef struct {
    uint32_t dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
    uint32_t descriptorType;
    const void *pImageInfo;
    const void *pBufferInfo;
    const void *pTexelBufferView;
} VkWriteDescriptorSet;

typedef struct {
    uint32_t srcSet;
    uint32_t srcBinding;
    uint32_t srcArrayElement;
    uint32_t dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
} VkCopyDescriptorSet;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkBufferUsageFlags flags;
    VkDeviceSize size;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
} VkBufferCreateInfo;

typedef struct {
    VkStructureType sType;
    const void *pNext;
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
    float mipLodBias;
    VkBool32 anisotropyEnable;
    float maxAnisotropy;
    VkBool32 compareEnable;
    VkCompareOp compareOp;
    float minLod;
    float maxLod;
    VkBorderColor borderColor;
    VkBool32 unnormalizedCoordinates;
} VkSamplerCreateInfo;

VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, VkInstance *pInstance);
void vkDestroyInstance(VkInstance instance);
VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices);
void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties);
void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures);
void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties);
VkResult vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, VkDevice *pDevice);
void vkDestroyDevice(VkDevice device);
VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, void *pProperties);
VkResult vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount, void *pProperties);
VkResult vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount, void *pProperties);
void vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue);
VkResult vkDeviceWaitIdle(VkDevice device);
VkResult vkQueueWaitIdle(VkQueue queue);
VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, VkDeviceMemory *pMemory);
void vkFreeMemory(VkDevice device, VkDeviceMemory memory, const void *pAllocator);
VkResult vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, uint32_t flags, void **ppData);
void vkUnmapMemory(VkDevice device, VkDeviceMemory memory);
VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, VkBuffer *pBuffer);
void vkDestroyBuffer(VkDevice device, VkBuffer buffer);
VkResult vkCreateBufferView(VkDevice device, const void *pCreateInfo, VkBufferView *pView);
void vkDestroyBufferView(VkDevice device, VkBufferView bufferView);
VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, VkImage *pImage);
void vkDestroyImage(VkDevice device, VkImage image);
void vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout);
VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, VkImageView *pView);
void vkDestroyImageView(VkDevice device, VkImageView imageView);
VkResult vkCreateShaderModule(VkDevice device, const void *pCreateInfo, VkShaderModule *pShaderModule);
void vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule);
VkResult vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, VkPipelineCache *pPipelineCache);
void vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache);
VkResult vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, uint64_t *pDataSize, void *pData);
VkResult vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches);
VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines);
VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines);
void vkDestroyPipeline(VkDevice device, VkPipeline pipeline);
VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, VkPipelineLayout *pPipelineLayout);
void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout);
VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, VkSampler *pSampler);
void vkDestroySampler(VkDevice device, VkSampler sampler);
VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayout *pSetLayout);
void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, VkDescriptorPool *pDescriptorPool);
void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);
VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags);
VkResult vkAllocateDescriptorSets(VkDevice device, const void *pAllocateInfo, VkDescriptorSet *pDescriptorSets);
VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets);
void vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies);
VkResult vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, VkRenderPass *pRenderPass);
void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass);
VkResult vkCreateFramebuffer(VkDevice device, const void *pCreateInfo, VkFramebuffer *pFramebuffer);
void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer);
VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, VkCommandPool *pCommandPool);
void vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool);
VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags);
VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers);
void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers);
VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo);
VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer);
VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags);
void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
void vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports);
void vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors);
void vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth);
void vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
void vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]);
void vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds);
void vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask);
void vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask);
void vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference);
void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets);
void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets);
void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents);
void vkCmdEndRenderPass(VkCommandBuffer commandBuffer);
void vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
void vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
void vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
void vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
void vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
void vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset);
void vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions);
void vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions);
void vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const void *pRegions, VkFilter filter);
void vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions);
void vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions);
void vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData);
void vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges);
void vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const void *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges);
void vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const void *pAttachments, uint32_t rectCount, const VkClearRect *pRects);
void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags);
VkResult vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence);
VkResult vkQueueWaitIdle(VkQueue queue);
VkResult vkQueuePresentKHR(VkQueue queue, const void *pPresentInfo);

#endif
