#ifndef BRIGHTS_SHADER_H
#define BRIGHTS_SHADER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SHADER_CODE_SIZE 65536
#define MAX_SHADER_CONSTANTS 256
#define MAX_SHADER_SAMPLERS 16
#define MAX_SHADER_OUTPUTS 8

typedef enum {
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_GEOMETRY,
    SHADER_STAGE_COMPUTE,
    SHADER_STAGE_TESS_CONTROL,
    SHADER_STAGE_TESS_EVAL
} shader_stage_t;

typedef enum {
    VALUE_TYPE_FLOAT = 0,
    VALUE_TYPE_VEC2,
    VALUE_TYPE_VEC3,
    VALUE_TYPE_VEC4,
    VALUE_TYPE_MAT2,
    VALUE_TYPE_MAT3,
    VALUE_TYPE_MAT4,
    VALUE_TYPE_INT,
    VALUE_TYPE_IVEC2,
    VALUE_TYPE_IVEC3,
    VALUE_TYPE_IVEC4,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_BVEC2,
    VALUE_TYPE_BVEC3,
    VALUE_TYPE_BVEC4,
    VALUE_TYPE_SAMPLER2D,
    VALUE_TYPE_SAMPLER3D,
    VALUE_TYPE_SAMPLER_CUBE
} value_type_t;

typedef enum {
    OP_NOP = 0,
    OP_LOAD,
    OP_STORE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,
    OP_CMPLT,
    OP_CMPLE,
    OP_CMPGT,
    OP_CMPGE,
    OP_CMPEQ,
    OP_CMPNE,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,
    OP_SHL,
    OP_SHR,
    OP_SELECT,
    OP_MAT_MUL,
    OP_MAT_ADD,
    OP_MAT_SUB,
    OP_MAT_SCALE,
    OP_TRANSPOSE,
    OP_DETERMINANT,
    OP_INVERSE,
    OP_DOT,
    OP_CROSS,
    OP_LENGTH,
    OP_NORMALIZE,
    OP_REFLECT,
    OP_REFRACT,
    OP_TEXTURE,
    OP_TEXTURE_LOD,
    OP_TEXTURE_GRAD,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_ASIN,
    OP_ACOS,
    OP_ATAN,
    OP_ATAN2,
    OP_POW,
    OP_EXP,
    OP_LOG,
    OP_EXP2,
    OP_LOG2,
    OP_SQRT,
    OP_RSQRT,
    OP_ABS,
    OP_FLOOR,
    OP_CEIL,
    OP_FRACT,
    OP_MIN,
    OP_MAX,
    OP_CLAMP,
    OP_STEP,
    OP_SMOOTHSTEP,
    OP_DISCARD,
    OP_EMIT_VERTEX,
    OP_END_PRIMITIVE
} opcode_t;

typedef struct {
    opcode_t opcode;
    uint32_t dst;
    uint32_t src0;
    uint32_t src1;
    uint32_t src2;
    uint32_t imm;
} instruction_t;

typedef struct {
    value_type_t type;
    uint32_t size;
    uint32_t location;
    char name[64];
} uniform_t;

typedef struct {
    value_type_t type;
    uint32_t size;
    uint32_t binding;
    char name[64];
} sampler_t;

typedef struct {
    uint32_t index;
    char name[64];
} shader_output_t;

typedef struct {
    shader_stage_t stage;
    uint32_t code_size;
    uint32_t instruction_count;
    instruction_t *code;
    uniform_t uniforms[MAX_SHADER_CONSTANTS];
    uint32_t uniform_count;
    sampler_t samplers[MAX_SHADER_SAMPLERS];
    uint32_t sampler_count;
    shader_output_t outputs[MAX_SHADER_OUTPUTS];
    uint32_t output_count;
    uint32_t workgroup_size[3];
    uint32_t local_size[3];
} shader_module_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t generator;
    uint32_t bound;
    uint32_t instruction_count;
    uint32_t word_count;
} spirv_header_t;

typedef enum {
    GLSL_TOKEN_IDENTIFIER = 256,
    GLSL_TOKEN_INT_CONST,
    GLSL_TOKEN_FLOAT_CONST,
    GLSL_TOKEN_STRING,
    GLSL_TOKEN_TYPE_VOID,
    GLSL_TOKEN_TYPE_BOOL,
    GLSL_TOKEN_TYPE_INT,
    GLSL_TOKEN_TYPE_UINT,
    GLSL_TOKEN_TYPE_FLOAT,
    GLSL_TOKEN_TYPE_VEC2,
    GLSL_TOKEN_TYPE_VEC3,
    GLSL_TOKEN_TYPE_VEC4,
    GLSL_TOKEN_TYPE_MAT2,
    GLSL_TOKEN_TYPE_MAT3,
    GLSL_TOKEN_TYPE_MAT4,
    GLSL_TOKEN_TYPE_SAMPLER2D,
    GLSL_TOKEN_TYPE_SAMPLER3D,
    GLSL_TOKEN_TYPE_SAMPLERCUBE,
    GLSL_TOKEN_PRECISION_HIGH,
    GLSL_TOKEN_PRECISION_MEDIUM,
    GLSL_TOKEN_PRECISION_LOW,
    GLSL_TOKEN_UNIFORM,
    GLSL_TOKEN_ATTRIBUTE,
    GLSL_TOKEN_VARYING,
    GLSL_TOKEN_IN,
    GLSL_TOKEN_OUT,
    GLSL_TOKEN_INOUT,
    GLSL_TOKEN_CONST,
    GLSL_TOKEN_STRUCT,
    GLSL_TOKEN_LAYOUT,
    GLSL_TOKEN_LOCATION,
    GLSL_TOKEN_BINDING,
    GLSL_TOKEN_SET,
    GLSL_TOKEN_IF,
    GLSL_TOKEN_ELSE,
    GLSL_TOKEN_FOR,
    GLSL_TOKEN_WHILE,
    GLSL_TOKEN_DO,
    GLSL_TOKEN_BREAK,
    GLSL_TOKEN_CONTINUE,
    GLSL_TOKEN_RETURN,
    GLSL_TOKEN_DISCARD,
    GLSL_TOKEN_SWITCH,
    GLSL_TOKEN_CASE,
    GLSL_TOKEN_DEFAULT,
    GLSL_TOKEN_TRUE,
    GLSL_TOKEN_FALSE,
    GLSL_TOKEN_VERTEX,
    GLSL_TOKEN_FRAGMENT,
    GLSL_TOKEN_GEOMETRY,
    GLSL_TOKEN_COMPUTE
} glsl_token_type_t;

typedef struct {
    const char *source;
    size_t length;
    size_t position;
    int current_line;
    int current_column;
    glsl_token_type_t token_type;
    char token_string[256];
    int token_int;
    float token_float;
} glsl_lexer_t;

typedef struct {
    shader_module_t *module;
    uint32_t next_register;
    uint32_t next_label;
} glsl_compiler_t;

int shader_module_create(shader_module_t **module);
void shader_module_destroy(shader_module_t *module);
int shader_compile_glsl(shader_module_t *module, const char *glsl_source, size_t source_length, shader_stage_t stage);
int shader_compile_spirv(shader_module_t *module, const uint32_t *spirv_data, size_t word_count);

int glsl_compile_vertex_shader(shader_module_t *module, const char *source, size_t length);
int glsl_compile_fragment_shader(shader_module_t *module, const char *source, size_t length);
int glsl_compile_compute_shader(shader_module_t *module, const char *source, size_t length);

const char *shader_get_disassembly(shader_module_t *module);
void shader_print_ir(shader_module_t *module);

uint32_t shader_type_sizeof(value_type_t type);
const char *shader_type_name(value_type_t type);

int spirv_validate(const uint32_t *data, size_t word_count);
int spirv_disassemble(const uint32_t *data, size_t word_count, char *output, size_t output_size);

int shader_optimize(shader_module_t *module);
int shader_dce(shader_module_t *module);
int shader_copy_propagation(shader_module_t *module);
int shader_constant_folding(shader_module_t *module);

#endif
