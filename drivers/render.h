#ifndef BRIGHTS_RENDER_H
#define BRIGHTS_RENDER_H

#include "gpu_hal.h"
#include "fb.h"
#include <stdbool.h>

#define MAX_VERTEX_ATTRIBUTES 16
#define MAX_UNIFORM_BUFFERS 16
#define MAX_TEXTURE_SLOTS 16
#define MAX_VERTEX_BUFFERS 8
#define MAX_RENDER_TARGETS 4
#define MAX_VIEWPORTS 16
#define MAX_SCISSORS 16

typedef enum {
    PRIM_POINT_LIST = 0,
    PRIM_LINE_LIST,
    PRIM_LINE_STRIP,
    PRIM_LINE_LOOP,
    PRIM_TRIANGLE_LIST,
    PRIM_TRIANGLE_STRIP,
    PRIM_TRIANGLE_FAN,
    PRIM_TRIANGLE_LIST_ADJ,
    PRIM_TRIANGLE_STRIP_ADJ,
    PRIM_PATCH_LIST
} primitive_type_t;

typedef enum {
    VERTEX_INPUT_RATE_VERTEX = 0,
    VERTEX_INPUT_RATE_INSTANCE = 1
} vertex_input_rate_t;

typedef enum {
    POLYGON_MODE_FILL = 0,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT
} polygon_mode_t;

typedef enum {
    CULL_MODE_NONE = 0,
    CULL_MODE_FRONT,
    CULL_MODE_BACK,
    CULL_MODE_FRONT_AND_BACK
} cull_mode_t;

typedef enum {
    FRONT_FACE_CW = 0,
    FRONT_FACE_CCW = 1
} front_face_t;

typedef enum {
    COMPARE_OP_NEVER = 0,
    COMPARE_OP_LESS,
    COMPARE_OP_EQUAL,
    COMPARE_OP_LESS_OR_EQUAL,
    COMPARE_OP_GREATER,
    COMPARE_OP_NOT_EQUAL,
    COMPARE_OP_GREATER_OR_EQUAL,
    COMPARE_OP_ALWAYS
} compare_op_t;

typedef enum {
    STENCIL_OP_KEEP = 0,
    STENCIL_OP_ZERO,
    STENCIL_OP_REPLACE,
    STENCIL_OP_INCREMENT_AND_CLAMP,
    STENCIL_OP_DECREMENT_AND_CLAMP,
    STENCIL_OP_INVERT,
    STENCIL_OP_INCREMENT_AND_WRAP,
    STENCIL_OP_DECREMENT_AND_WRAP
} stencil_op_t;

typedef enum {
    BLEND_OP_ADD = 0,
    BLEND_OP_SUBTRACT,
    BLEND_OP_REVERSE_SUBTRACT,
    BLEND_OP_MIN,
    BLEND_OP_MAX
} blend_op_t;

typedef enum {
    BLEND_FACTOR_ZERO = 0,
    BLEND_FACTOR_ONE,
    BLEND_FACTOR_SRC_COLOR,
    BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    BLEND_FACTOR_DST_COLOR,
    BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    BLEND_FACTOR_SRC_ALPHA,
    BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    BLEND_FACTOR_DST_ALPHA,
    BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    BLEND_FACTOR_CONSTANT_COLOR,
    BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    BLEND_FACTOR_CONSTANT_ALPHA,
    BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    BLEND_FACTOR_SRC_ALPHA_SATURATE
} blend_factor_t;

typedef enum {
    COLOR_COMPONENT_R = 1 << 0,
    COLOR_COMPONENT_G = 1 << 1,
    COLOR_COMPONENT_B = 1 << 2,
    COLOR_COMPONENT_A = 1 << 3
} color_component_t;

typedef enum {
    BUFFER_USAGE_VERTEX_BUFFER = 1 << 0,
    BUFFER_USAGE_INDEX_BUFFER = 1 << 1,
    BUFFER_USAGE_UNIFORM_BUFFER = 1 << 2,
    BUFFER_USAGE_STORAGE_BUFFER = 1 << 3,
    BUFFER_USAGE_INDIRECT_BUFFER = 1 << 4,
    BUFFER_USAGE_TRANSFER_SRC = 1 << 5,
    BUFFER_USAGE_TRANSFER_DST = 1 << 6
} buffer_usage_t;

typedef enum {
    FORMAT_UNDEFINED = 0,
    FORMAT_R4G4_UNORM,
    FORMAT_R4G4B4A4_UNORM,
    FORMAT_B4G4R4A4_UNORM,
    FORMAT_R5G6B5_UNORM,
    FORMAT_B5G6R5_UNORM,
    FORMAT_R5G5B5A1_UNORM,
    FORMAT_B5G5R5A1_UNORM,
    FORMAT_A1R5G5B5_UNORM,
    FORMAT_R8_UNORM,
    FORMAT_R8_SNORM,
    FORMAT_R8_USCALED,
    FORMAT_R8_SSCALED,
    FORMAT_R8_UINT,
    FORMAT_R8_SINT,
    FORMAT_R8_SRGB,
    FORMAT_R8G8_UNORM,
    FORMAT_R8G8_SNORM,
    FORMAT_R8G8_UINT,
    FORMAT_R8G8_SINT,
    FORMAT_R8G8_SRGB,
    FORMAT_R8G8B8_UNORM,
    FORMAT_R8G8B8_SNORM,
    FORMAT_R8G8B8_UINT,
    FORMAT_R8G8B8_SINT,
    FORMAT_R8G8B8_SRGB,
    FORMAT_B8G8R8_UNORM,
    FORMAT_B8G8R8_SNORM,
    FORMAT_R8G8B8A8_UNORM,
    FORMAT_R8G8B8A8_SNORM,
    FORMAT_R8G8B8A8_UINT,
    FORMAT_R8G8B8A8_SINT,
    FORMAT_R8G8B8A8_SRGB,
    FORMAT_B8G8R8A8_UNORM,
    FORMAT_B8G8R8A8_SRGB,
    FORMAT_R16_UNORM,
    FORMAT_R16_SNORM,
    FORMAT_R16_USCALED,
    FORMAT_R16_SSCALED,
    FORMAT_R16_UINT,
    FORMAT_R16_SINT,
    FORMAT_R16_SFLOAT,
    FORMAT_R16G16_UNORM,
    FORMAT_R16G16_SNORM,
    FORMAT_R16G16_UINT,
    FORMAT_R16G16_SINT,
    FORMAT_R16G16_SFLOAT,
    FORMAT_R16G16B16_SFLOAT,
    FORMAT_R16G16B16A16_UNORM,
    FORMAT_R16G16B16A16_SNORM,
    FORMAT_R16G16B16A16_UINT,
    FORMAT_R16G16B16A16_SINT,
    FORMAT_R16G16B16A16_SFLOAT,
    FORMAT_R32_UINT,
    FORMAT_R32_SINT,
    FORMAT_R32_SFLOAT,
    FORMAT_R32G32_UINT,
    FORMAT_R32G32_SINT,
    FORMAT_R32G32_SFLOAT,
    FORMAT_R32G32B32_UINT,
    FORMAT_R32G32B32_SINT,
    FORMAT_R32G32B32_SFLOAT,
    FORMAT_R32G32B32A32_UINT,
    FORMAT_R32G32B32A32_SINT,
    FORMAT_R32G32B32A32_SFLOAT,
    FORMAT_D16_UNORM,
    FORMAT_D32_SFLOAT,
    FORMAT_S8_UINT,
    FORMAT_D16_UNORM_S8_UINT,
    FORMAT_D24_UNORM_S8_UINT,
    FORMAT_D32_SFLOAT_S8_UINT
} format_t;

typedef enum {
    IMAGE_TYPE_1D = 0,
    IMAGE_TYPE_2D,
    IMAGE_TYPE_3D,
    IMAGE_TYPE_CUBE
} image_type_t;

typedef enum {
    FILTER_NEAREST = 0,
    FILTER_LINEAR,
    FILTER_CUBIC
} filter_mode_t;

typedef enum {
    SAMPLER_MIPMAP_MODE_NEAREST = 0,
    SAMPLER_MIPMAP_MODE_LINEAR
} sampler_mipmap_mode_t;

typedef enum {
    SAMPLER_ADDRESS_MODE_REPEAT = 0,
    SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
} sampler_address_mode_t;

typedef enum {
    BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
    BORDER_COLOR_INT_TRANSPARENT_BLACK,
    BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    BORDER_COLOR_INT_OPAQUE_BLACK,
    BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    BORDER_COLOR_INT_OPAQUE_WHITE
} border_color_t;

typedef struct {
    uint32_t binding;
    uint32_t stride;
    vertex_input_rate_t input_rate;
    uint32_t offset;
} vertex_binding_t;

typedef struct {
    uint32_t location;
    uint32_t binding;
    format_t format;
    uint32_t offset;
} vertex_attribute_t;

typedef struct {
    bool depth_test;
    bool depth_write;
    compare_op_t depth_compare;
    bool depth_bounds_test;
    float min_depth_bounds;
    float max_depth_bounds;
    bool stencil_test;
    struct {
        compare_op_t compare;
        stencil_op_t fail_op;
        stencil_op_t pass_op;
        stencil_op_t depth_fail_op;
        uint32_t compare_mask;
        uint32_t write_mask;
        uint32_t reference;
    } front;
    struct {
        compare_op_t compare;
        stencil_op_t fail_op;
        stencil_op_t pass_op;
        stencil_op_t depth_fail_op;
        uint32_t compare_mask;
        uint32_t write_mask;
        uint32_t reference;
    } back;
} depth_stencil_state_t;

typedef struct {
    bool enable;
    bool alpha_to_coverage;
    bool alpha_to_one;
    blend_factor_t src_color_factor;
    blend_factor_t dst_color_factor;
    blend_op_t color_op;
    blend_factor_t src_alpha_factor;
    blend_factor_t dst_alpha_factor;
    blend_op_t alpha_op;
    uint32_t color_write_mask;
} blend_state_t;

typedef struct {
    bool enable;
    polygon_mode_t mode;
    cull_mode_t cull_mode;
    front_face_t front_face;
    bool depth_bias;
    float depth_bias_constant;
    float depth_bias_clamp;
    float depth_bias_slope;
    float line_width;
    float point_size;
} raster_state_t;

typedef struct {
    float m[16];
} matrix4x4_t;

typedef struct {
    float x, y, z, w;
} vec4_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x, y;
} vec2_t;

typedef struct {
    vec4_t position;
    vec4_t texcoord;
    vec4_t normal;
    vec4_t color;
} vertex_t;

typedef struct {
    gpu_buffer_t *buffers[MAX_VERTEX_BUFFERS];
    uint64_t offsets[MAX_VERTEX_BUFFERS];
    uint64_t strides[MAX_VERTEX_BUFFERS];
    gpu_buffer_t *index_buffer;
    uint64_t index_offset;
    format_t index_format;
    gpu_buffer_t *uniform_buffers[MAX_UNIFORM_BUFFERS];
    gpu_image_t *textures[MAX_TEXTURE_SLOTS];
} render_resource_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    format_t format;
    gpu_image_t *image;
    gpu_memory_t *memory;
} render_target_t;

typedef struct {
    bool enable_scissor;
    uint32_t scissor_count;
    struct {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    } scissors[MAX_SCISSORS];
    uint32_t viewport_count;
    struct {
        float x, y, width, height;
        float min_depth;
        float max_depth;
    } viewports[MAX_VIEWPORTS];
} viewport_state_t;

typedef struct {
    void *vertex_shader;
    void *fragment_shader;
    void *geometry_shader;
    void *tess_control_shader;
    void *tess_eval_shader;
    void *compute_shader;
} shader_t;

typedef struct {
    primitive_type_t primitive_type;
    uint32_t patch_control_points;
    depth_stencil_state_t depth_stencil;
    blend_state_t blend;
    raster_state_t raster;
    viewport_state_t viewport;
    render_target_t render_targets[MAX_RENDER_TARGETS];
    uint32_t render_target_count;
} pipeline_state_t;

typedef struct {
    matrix4x4_t model;
    matrix4x4_t view;
    matrix4x4_t projection;
    matrix4x4_t mvp;
    vec4_t viewport_size;
    float time;
    float delta_time;
} uniform_data_t;

void render3d_init(void);
void render3d_shutdown(void);

int render_create_pipeline(pipeline_state_t *state);
void render_destroy_pipeline(int pipeline_id);

int render_bind_pipeline(int pipeline_id);
int render_bind_vertex_buffers(uint32_t first_binding, uint32_t count, gpu_buffer_t **buffers, uint64_t *offsets);
int render_bind_index_buffer(gpu_buffer_t *buffer, uint64_t offset, format_t format);
int render_bind_uniform_buffer(uint32_t set, uint32_t binding, gpu_buffer_t *buffer, uint64_t offset, uint64_t range);
int render_bind_texture(uint32_t set, uint32_t binding, gpu_image_t *image);

int render_set_viewport(uint32_t first_viewport, uint32_t viewport_count, void *viewports);
int render_set_scissor(uint32_t first_scissor, uint32_t scissor_count, void *scissors);

int render_begin_render_pass(pipeline_state_t *state, render_target_t *targets, uint32_t count);
void render_end_render_pass(void);

int render_draw(uint32_t vertex_count, uint32_t first_vertex);
int render_draw_indexed(uint32_t index_count, uint32_t first_index, int32_t vertex_offset);
int render_draw_instanced(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
int render_draw_indexed_instanced(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

int render_update_uniform(int set, uint32_t binding, void *data, size_t size);

void render_set_depth_bias(float constant, float clamp, float slope);
void render_set_depth_bounds(float min, float max);
void render_setStencilReference(uint32_t reference);
void render_setStencilWriteMask(uint32_t mask);
void render_setStencilCompareMask(uint32_t mask);
void render_setBlendConstants(float r, float g, float b, float a);
void render_setLineWidth(float width);
void render_setPointSize(float size);

int render_submit(void);
int render_wait_idle(void);

matrix4x4_t matrix_identity(void);
matrix4x4_t matrix_multiply(matrix4x4_t a, matrix4x4_t b);
matrix4x4_t matrix_translate(float x, float y, float z);
matrix4x4_t matrix_scale(float x, float y, float z);
matrix4x4_t matrix_rotate(float angle, float x, float y, float z);
matrix4x4_t matrix_perspective(float fov, float aspect, float near, float far);
matrix4x4_t matrix_ortho(float left, float right, float bottom, float top, float near, float far);
matrix4x4_t matrix_look_at(vec3_t eye, vec3_t center, vec3_t up);
vec3_t matrix_transform_point(matrix4x4_t m, vec3_t v);

void render_blit_to_framebuffer(gpu_image_t *src, int dst_x, int dst_y, int dst_w, int dst_h);
void render_present(void);

#endif
