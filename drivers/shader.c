#include "shader.h"
#include "../kernel/kmalloc.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define MAX_INSTRUCTIONS 4096

static const char *opcode_names[] = {
    "nop", "load", "store", "add", "sub", "mul", "div", "mod", "neg",
    "cmplt", "cmple", "cmpgt", "cmpge", "cmpeq", "cmpne",
    "and", "or", "xor", "not", "shl", "shr",
    "select", "mat_mul", "mat_add", "mat_sub", "mat_scale",
    "transpose", "determinant", "inverse", "dot", "cross",
    "length", "normalize", "reflect", "refract",
    "texture", "texture_lod", "texture_grad",
    "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
    "pow", "exp", "log", "exp2", "log2", "sqrt", "rsqrt",
    "abs", "floor", "ceil", "fract", "min", "max",
    "clamp", "step", "smoothstep", "discard",
    "emit_vertex", "end_primitive"
};

static const char *type_names[] = {
    "float", "vec2", "vec3", "vec4",
    "mat2", "mat3", "mat4",
    "int", "ivec2", "ivec3", "ivec4",
    "bool", "bvec2", "bvec3", "bvec4",
    "sampler2D", "sampler3D", "samplerCube"
};

uint32_t shader_type_sizeof(value_type_t type) {
    switch (type) {
        case VALUE_TYPE_FLOAT: return 4;
        case VALUE_TYPE_VEC2: return 8;
        case VALUE_TYPE_VEC3: return 12;
        case VALUE_TYPE_VEC4: return 16;
        case VALUE_TYPE_MAT2: return 16;
        case VALUE_TYPE_MAT3: return 36;
        case VALUE_TYPE_MAT4: return 64;
        case VALUE_TYPE_INT:
        case VALUE_TYPE_BOOL: return 4;
        case VALUE_TYPE_IVEC2:
        case VALUE_TYPE_BVEC2: return 8;
        case VALUE_TYPE_IVEC3:
        case VALUE_TYPE_BVEC3: return 12;
        case VALUE_TYPE_IVEC4:
        case VALUE_TYPE_BVEC4: return 16;
        default: return 4;
    }
}

const char *shader_type_name(value_type_t type) {
    if (type >= 0 && type <= VALUE_TYPE_SAMPLER_CUBE) {
        return type_names[type];
    }
    return "unknown";
}

int shader_module_create(shader_module_t **module) {
    shader_module_t *mod = (shader_module_t *)brights_kmalloc(sizeof(shader_module_t));
    if (!mod) return -1;
    
    memset(mod, 0, sizeof(shader_module_t));
    mod->code = (instruction_t *)brights_kmalloc(MAX_INSTRUCTIONS * sizeof(instruction_t));
    if (!mod->code) {
        brights_kfree(mod);
        return -1;
    }
    
    mod->instruction_count = 0;
    *module = mod;
    return 0;
}

void shader_module_destroy(shader_module_t *module) {
    if (!module) return;
    if (module->code) brights_kfree(module->code);
    brights_kfree(module);
}

static int parse_builtin_type(const char *name) {
    if (strcmp(name, "void") == 0) return -1;
    if (strcmp(name, "bool") == 0) return VALUE_TYPE_BOOL;
    if (strcmp(name, "int") == 0) return VALUE_TYPE_INT;
    if (strcmp(name, "float") == 0) return VALUE_TYPE_FLOAT;
    if (strcmp(name, "vec2") == 0) return VALUE_TYPE_VEC2;
    if (strcmp(name, "vec3") == 0) return VALUE_TYPE_VEC3;
    if (strcmp(name, "vec4") == 0) return VALUE_TYPE_VEC4;
    if (strcmp(name, "mat2") == 0) return VALUE_TYPE_MAT2;
    if (strcmp(name, "mat3") == 0) return VALUE_TYPE_MAT3;
    if (strcmp(name, "mat4") == 0) return VALUE_TYPE_MAT4;
    if (strcmp(name, "sampler2D") == 0) return VALUE_TYPE_SAMPLER2D;
    if (strcmp(name, "samplerCube") == 0) return VALUE_TYPE_SAMPLER_CUBE;
    return -1;
}

static int glsl_lexer_init(glsl_lexer_t *lexer, const char *source, size_t length) {
    lexer->source = source;
    lexer->length = length;
    lexer->position = 0;
    lexer->current_line = 1;
    lexer->current_column = 1;
    return 0;
}

static void glsl_lexer_next_token(glsl_lexer_t *lexer) {
    if (lexer->position >= lexer->length) {
        lexer->token_type = 0;
        return;
    }
    
    const char *p = lexer->source + lexer->position;
    
    while (isspace(*p) && lexer->position < lexer->length) {
        if (*p == '\n') {
            lexer->current_line++;
            lexer->current_column = 1;
        } else {
            lexer->current_column++;
        }
        lexer->position++;
        p = lexer->source + lexer->position;
    }
    
    if (lexer->position >= lexer->length) {
        lexer->token_type = 0;
        return;
    }
    
    if (isalpha(*p) || *p == '_') {
        size_t start = lexer->position;
        while (isalnum(*p) || *p == '_') {
            lexer->position++;
            p = lexer->source + lexer->position;
        }
        size_t len = lexer->position - start;
        if (len > 255) len = 255;
        strncpy(lexer->token_string, lexer->source + start, len);
        lexer->token_string[len] = '\0';
        
        if (strcmp(lexer->token_string, "if") == 0) lexer->token_type = GLSL_TOKEN_IF;
        else if (strcmp(lexer->token_string, "else") == 0) lexer->token_type = GLSL_TOKEN_ELSE;
        else if (strcmp(lexer->token_string, "for") == 0) lexer->token_type = GLSL_TOKEN_FOR;
        else if (strcmp(lexer->token_string, "while") == 0) lexer->token_type = GLSL_TOKEN_WHILE;
        else if (strcmp(lexer->token_string, "return") == 0) lexer->token_type = GLSL_TOKEN_RETURN;
        else if (strcmp(lexer->token_string, "discard") == 0) lexer->token_type = GLSL_TOKEN_DISCARD;
        else if (strcmp(lexer->token_string, "uniform") == 0) lexer->token_type = GLSL_TOKEN_UNIFORM;
        else if (strcmp(lexer->token_string, "in") == 0) lexer->token_type = GLSL_TOKEN_IN;
        else if (strcmp(lexer->token_string, "out") == 0) lexer->token_type = GLSL_TOKEN_OUT;
        else if (strcmp(lexer->token_string, "inout") == 0) lexer->token_type = GLSL_TOKEN_INOUT;
        else if (strcmp(lexer->token_string, "layout") == 0) lexer->token_type = GLSL_TOKEN_LAYOUT;
        else if (strcmp(lexer->token_string, "location") == 0) lexer->token_type = GLSL_TOKEN_LOCATION;
        else if (strcmp(lexer->token_string, "true") == 0) lexer->token_type = GLSL_TOKEN_TRUE;
        else if (strcmp(lexer->token_string, "false") == 0) lexer->token_type = GLSL_TOKEN_FALSE;
        else if (strcmp(lexer->token_string, "struct") == 0) lexer->token_type = GLSL_TOKEN_STRUCT;
        else if (parse_builtin_type(lexer->token_string) >= 0) {
            lexer->token_type = GLSL_TOKEN_IDENTIFIER;
        }
        else lexer->token_type = GLSL_TOKEN_IDENTIFIER;
        
        return;
    }
    
    if (isdigit(*p)) {
        size_t start = lexer->position;
        int is_float = 0;
        
        while (isdigit(*p) || *p == '.' || *p == 'e' || *p == 'E' || *p == '-' || *p == '+') {
            if (*p == '.' || *p == 'e' || *p == 'E') is_float = 1;
            lexer->position++;
            p = lexer->source + lexer->position;
        }
        
        size_t len = lexer->position - start;
        if (len > 255) len = 255;
        strncpy(lexer->token_string, lexer->source + start, len);
        lexer->token_string[len] = '\0';
        
        if (is_float) {
            lexer->token_float = atof(lexer->token_string);
            lexer->token_type = GLSL_TOKEN_FLOAT_CONST;
        } else {
            lexer->token_int = atoi(lexer->token_string);
            lexer->token_type = GLSL_TOKEN_INT_CONST;
        }
        return;
    }
    
    lexer->token_type = *p;
    lexer->position++;
    lexer->current_column++;
}

static instruction_t *emit_instruction(shader_module_t *module, opcode_t op, uint32_t dst, uint32_t src0, uint32_t src1, uint32_t src2) {
    if (module->instruction_count >= MAX_INSTRUCTIONS) return 0;
    
    instruction_t *inst = &module->code[module->instruction_count++];
    inst->opcode = op;
    inst->dst = dst;
    inst->src0 = src0;
    inst->src1 = src1;
    inst->src2 = src2;
    inst->imm = 0;
    
    return inst;
}

int shader_compile_glsl(shader_module_t *module, const char *glsl_source, size_t source_length, shader_stage_t stage) {
    if (!module || !glsl_source) return -1;
    
    module->stage = stage;
    module->instruction_count = 0;
    
    glsl_lexer_t lexer;
    glsl_lexer_init(&lexer, glsl_source, source_length);
    
    while (lexer.position < lexer.length) {
        glsl_lexer_next_token(&lexer);
        
        if (lexer.token_type == GLSL_TOKEN_UNIFORM) {
            glsl_lexer_next_token(&lexer);
            int type = parse_builtin_type(lexer.token_string);
            if (type >= 0) {
                glsl_lexer_next_token(&lexer);
                if (lexer.token_type == GLSL_TOKEN_IDENTIFIER) {
                    if (module->uniform_count < MAX_SHADER_CONSTANTS) {
                        uniform_t *u = &module->uniforms[module->uniform_count++];
                        u->type = (value_type_t)type;
                        u->size = shader_type_sizeof(u->type);
                        strncpy(u->name, lexer.token_string, sizeof(u->name) - 1);
                    }
                }
            }
        }
    }
    
    emit_instruction(module, OP_NOP, 0, 0, 0, 0);
    
    return 0;
}

int glsl_compile_vertex_shader(shader_module_t *module, const char *source, size_t length) {
    return shader_compile_glsl(module, source, length, SHADER_STAGE_VERTEX);
}

int glsl_compile_fragment_shader(shader_module_t *module, const char *source, size_t length) {
    return shader_compile_glsl(module, source, length, SHADER_STAGE_FRAGMENT);
}

int glsl_compile_compute_shader(shader_module_t *module, const char *source, size_t length) {
    return shader_compile_glsl(module, source, length, SHADER_STAGE_COMPUTE);
}

int shader_compile_spirv(shader_module_t *module, const uint32_t *spirv_data, size_t word_count) {
    if (!module || !spirv_data || word_count < 5) return -1;
    
    spirv_header_t *header = (spirv_header_t *)spirv_data;
    
    if (header->magic != 0x07230203) return -1;
    if (header->version < 0x00010000) return -1;
    
    module->instruction_count = 0;
    
    return 0;
}

const char *shader_get_disassembly(shader_module_t *module) {
    static char buffer[16384];
    char *p = buffer;
    *p = '\0';
    
    for (uint32_t i = 0; i < module->instruction_count && i < MAX_INSTRUCTIONS; i++) {
        instruction_t *inst = &module->code[i];
        const char *opname = (inst->opcode < sizeof(opcode_names)/sizeof(opcode_names[0])) 
            ? opcode_names[inst->opcode] : "unknown";
        
        p += snprintf(p, buffer + sizeof(buffer) - p, "%04d: %s", i, opname);
        
        if (inst->dst) p += snprintf(p, buffer + sizeof(buffer) - p, " r%d", inst->dst);
        if (inst->src0) p += snprintf(p, buffer + sizeof(buffer) - p, " r%d", inst->src0);
        if (inst->src1) p += snprintf(p, buffer + sizeof(buffer) - p, " r%d", inst->src1);
        if (inst->src2) p += snprintf(p, buffer + sizeof(buffer) - p, " r%d", inst->src2);
        if (inst->imm) p += snprintf(p, buffer + sizeof(buffer) - p, " #%d", inst->imm);
        
        p += snprintf(p, buffer + sizeof(buffer) - p, "\n");
    }
    
    return buffer;
}

void shader_print_ir(shader_module_t *module) {
}

int spirv_validate(const uint32_t *data, size_t word_count) {
    if (!data || word_count < 5) return 0;
    
    spirv_header_t *header = (spirv_header_t *)data;
    
    if (header->magic != 0x07230203) return 0;
    if (header->bound > 4096) return 0;
    if (word_count != header->word_count) return 0;
    
    return 1;
}

int spirv_disassemble(const uint32_t *data, size_t word_count, char *output, size_t output_size) {
    if (!spirv_validate(data, word_count)) return -1;
    
    char *p = output;
    *p = '\0';
    
    p += snprintf(p, output_size - (p - output), "; SPIR-V Disassembly\n");
    p += snprintf(p, output_size - (p - output), "; Version: %d.%d\n", 
                  (data[1] >> 16) & 0xFF, (data[1] >> 8) & 0xFF);
    p += snprintf(p, output_size - (p - output), "; Generator: 0x%08X\n", data[2]);
    p += snprintf(p, output_size - (p - output), "; Bound: %d\n\n", data[3]);
    
    return 0;
}

int shader_optimize(shader_module_t *module) {
    if (!module) return -1;
    
    shader_dce(module);
    shader_copy_propagation(module);
    shader_constant_folding(module);
    
    return 0;
}

int shader_dce(shader_module_t *module) {
    return 0;
}

int shader_copy_propagation(shader_module_t *module) {
    return 0;
}

int shader_constant_folding(shader_module_t *module) {
    return 0;
}
