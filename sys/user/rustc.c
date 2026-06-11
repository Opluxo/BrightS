#include "libc.h"

/*
 * Simple Rust compiler frontend for BrightS
 *
 * This is a very basic implementation that can:
 * - Parse simple Rust syntax
 * - Generate basic C code
 * - Compile using clang
 */

#define MAX_TOKENS 1024
#define MAX_CODE_SIZE (64 * 1024)

typedef enum {
    TOK_EOF = 0,
    TOK_IDENT,
    TOK_STRING,
    TOK_NUMBER,
    TOK_FN,
    TOK_LET,
    TOK_MUT,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    TOK_PRINTLN,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_COMMA,
    TOK_DOT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_EQ,
    TOK_EQEQ,
    TOK_NE,
    TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_ARROW
} token_type_t;

typedef struct {
    token_type_t type;
    char *text;
    int line;
} token_t;

typedef struct {
    const char *input;
    int pos;
    int line;
    token_t tokens[MAX_TOKENS];
    int token_count;
    char output[MAX_CODE_SIZE];
    int output_pos;
} rust_compiler_t;

/* Token definitions */
static const char *keywords[] = {
    "fn", "let", "mut", "if", "else", "while", "for", "return", "println",
    NULL
};

/*
 * Initialize compiler
 */
static void rustc_init(rust_compiler_t *comp, const char *input)
{
    memset(comp, 0, sizeof(*comp));
    comp->input = input;
    comp->pos = 0;
    comp->line = 1;
}

/*
 * Output C code
 */
static void rustc_emit(rust_compiler_t *comp, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    comp->output_pos += vsnprintf(comp->output + comp->output_pos,
                                  MAX_CODE_SIZE - comp->output_pos, fmt, args);
    va_end(args);
}

/*
 * Simple lexer
 */
static token_t *rustc_lex(rust_compiler_t *comp)
{
    if (comp->token_count >= MAX_TOKENS) return NULL;

    token_t *tok = &comp->tokens[comp->token_count++];

    /* Skip whitespace */
    while (comp->input[comp->pos] && (comp->input[comp->pos] == ' ' ||
                                     comp->input[comp->pos] == '\t')) {
        comp->pos++;
    }

    if (comp->input[comp->pos] == '\n') {
        comp->line++;
        comp->pos++;
        return rustc_lex(comp); /* Skip newlines */
    }

    if (!comp->input[comp->pos]) {
        tok->type = TOK_EOF;
        tok->text = "";
        return tok;
    }

    /* Single character tokens */
    switch (comp->input[comp->pos]) {
    case '(': tok->type = TOK_LPAREN; tok->text = "("; comp->pos++; break;
    case ')': tok->type = TOK_RPAREN; tok->text = ")"; comp->pos++; break;
    case '{': tok->type = TOK_LBRACE; tok->text = "{"; comp->pos++; break;
    case '}': tok->type = TOK_RBRACE; tok->text = "}"; comp->pos++; break;
    case '[': tok->type = TOK_LBRACKET; tok->text = "["; comp->pos++; break;
    case ']': tok->type = TOK_RBRACKET; tok->text = "]"; comp->pos++; break;
    case ';': tok->type = TOK_SEMICOLON; tok->text = ";"; comp->pos++; break;
    case ':': tok->type = TOK_COLON; tok->text = ":"; comp->pos++; break;
    case ',': tok->type = TOK_COMMA; tok->text = ","; comp->pos++; break;
    case '.': tok->type = TOK_DOT; tok->text = "."; comp->pos++; break;
    case '+': tok->type = TOK_PLUS; tok->text = "+"; comp->pos++; break;
    case '-':
        if (comp->input[comp->pos + 1] == '>') {
            tok->type = TOK_ARROW;
            tok->text = "->";
            comp->pos += 2;
        } else {
            tok->type = TOK_MINUS;
            tok->text = "-";
            comp->pos++;
        }
        break;
    case '*': tok->type = TOK_STAR; tok->text = "*"; comp->pos++; break;
    case '/': tok->type = TOK_SLASH; tok->text = "/"; comp->pos++; break;
    case '=':
        if (comp->input[comp->pos + 1] == '=') {
            tok->type = TOK_EQEQ;
            tok->text = "==";
            comp->pos += 2;
        } else {
            tok->type = TOK_EQ;
            tok->text = "=";
            comp->pos++;
        }
        break;
    case '!':
        if (comp->input[comp->pos + 1] == '=') {
            tok->type = TOK_NE;
            tok->text = "!=";
            comp->pos += 2;
        } else {
            tok->type = TOK_NOT;
            tok->text = "!";
            comp->pos++;
        }
        break;
    case '<':
        if (comp->input[comp->pos + 1] == '=') {
            tok->type = TOK_LE;
            tok->text = "<=";
            comp->pos += 2;
        } else {
            tok->type = TOK_LT;
            tok->text = "<";
            comp->pos++;
        }
        break;
    case '>':
        if (comp->input[comp->pos + 1] == '=') {
            tok->type = TOK_GE;
            tok->text = ">=";
            comp->pos += 2;
        } else {
            tok->type = TOK_GT;
            tok->text = ">";
            comp->pos++;
        }
        break;
    case '&':
        if (comp->input[comp->pos + 1] == '&') {
            tok->type = TOK_AND;
            tok->text = "&&";
            comp->pos += 2;
        }
        break;
    case '|':
        if (comp->input[comp->pos + 1] == '|') {
            tok->type = TOK_OR;
            tok->text = "||";
            comp->pos += 2;
        }
        break;
    default:
        /* String literal */
        if (comp->input[comp->pos] == '"') {
            comp->pos++;
            const char *start = &comp->input[comp->pos];
            while (comp->input[comp->pos] && comp->input[comp->pos] != '"') {
                if (comp->input[comp->pos] == '\\') comp->pos++;
                comp->pos++;
            }
            int len = &comp->input[comp->pos] - start;
            tok->type = TOK_STRING;
            tok->text = strndup(start, len);
            comp->pos++; /* Skip closing quote */
        }
        /* Number */
        else if (comp->input[comp->pos] >= '0' && comp->input[comp->pos] <= '9') {
            const char *start = &comp->input[comp->pos];
            while (comp->input[comp->pos] >= '0' && comp->input[comp->pos] <= '9') {
                comp->pos++;
            }
            int len = &comp->input[comp->pos] - start;
            tok->type = TOK_NUMBER;
            tok->text = strndup(start, len);
        }
        /* Identifier or keyword */
        else if ((comp->input[comp->pos] >= 'a' && comp->input[comp->pos] <= 'z') ||
                 (comp->input[comp->pos] >= 'A' && comp->input[comp->pos] <= 'Z') ||
                 comp->input[comp->pos] == '_') {
            const char *start = &comp->input[comp->pos];
            while ((comp->input[comp->pos] >= 'a' && comp->input[comp->pos] <= 'z') ||
                   (comp->input[comp->pos] >= 'A' && comp->input[comp->pos] <= 'Z') ||
                   (comp->input[comp->pos] >= '0' && comp->input[comp->pos] <= '9') ||
                   comp->input[comp->pos] == '_') {
                comp->pos++;
            }
            int len = &comp->input[comp->pos] - start;

            /* Check if it's a keyword */
            int is_keyword = 0;
            for (int i = 0; keywords[i]; i++) {
                if (strncmp(start, keywords[i], len) == 0 && strlen(keywords[i]) == len) {
                    is_keyword = 1;
                    if (strcmp(keywords[i], "fn") == 0) tok->type = TOK_FN;
                    else if (strcmp(keywords[i], "let") == 0) tok->type = TOK_LET;
                    else if (strcmp(keywords[i], "mut") == 0) tok->type = TOK_MUT;
                    else if (strcmp(keywords[i], "if") == 0) tok->type = TOK_IF;
                    else if (strcmp(keywords[i], "else") == 0) tok->type = TOK_ELSE;
                    else if (strcmp(keywords[i], "while") == 0) tok->type = TOK_WHILE;
                    else if (strcmp(keywords[i], "for") == 0) tok->type = TOK_FOR;
                    else if (strcmp(keywords[i], "return") == 0) tok->type = TOK_RETURN;
                    else if (strcmp(keywords[i], "println") == 0) tok->type = TOK_PRINTLN;
                    break;
                }
            }

            if (!is_keyword) {
                tok->type = TOK_IDENT;
                tok->text = strndup(start, len);
            }
        }
        break;
    }

    tok->line = comp->line;
    return tok;
}

/*
 * Simple Rust to C translator
 */
static int rustc_compile(rust_compiler_t *comp)
{
    /* Generate C header */
    rustc_emit(comp, "#include \"libc.h\"\n\n");

    /* Process tokens */
    int i = 0;
    while (i < comp->token_count) {
        token_t *tok = &comp->tokens[i];

        switch (tok->type) {
        case TOK_FN:
            /* Function definition */
            rustc_emit(comp, "void ");
            i++;
            if (i < comp->token_count && comp->tokens[i].type == TOK_IDENT) {
                rustc_emit(comp, "%s", comp->tokens[i].text);
                i++;
            }
            rustc_emit(comp, "(");
            /* Skip parameters for now */
            while (i < comp->token_count && comp->tokens[i].type != TOK_LBRACE) {
                i++;
            }
            rustc_emit(comp, "void)\n{\n");
            break;

        case TOK_PRINTLN:
            /* println! macro */
            i++; /* skip println */
            if (i < comp->token_count && comp->tokens[i].type == TOK_NOT) i++; /* skip ! */
            if (i < comp->token_count && comp->tokens[i].type == TOK_LPAREN) i++; /* skip ( */

            rustc_emit(comp, "    printf(\"");
            while (i < comp->token_count && comp->tokens[i].type != TOK_RPAREN) {
                if (comp->tokens[i].type == TOK_STRING) {
                    /* Remove quotes and handle escapes */
                    char *str = comp->tokens[i].text;
                    if (*str == '"') str++;
                    char *end = str + strlen(str) - 1;
                    if (*end == '"') *end = 0;
                    rustc_emit(comp, "%s", str);
                }
                i++;
            }
            rustc_emit(comp, "\\n\");\n");
            if (i < comp->token_count && comp->tokens[i].type == TOK_RPAREN) i++; /* skip ) */
            if (i < comp->token_count && comp->tokens[i].type == TOK_SEMICOLON) i++; /* skip ; */
            break;

        case TOK_RBRACE:
            rustc_emit(comp, "}\n\n");
            i++;
            break;

        default:
            i++;
            break;
        }
    }

    rustc_emit(comp, "\nint main(int argc, char **argv) {\n");
    rustc_emit(comp, "    main_();\n");
    rustc_emit(comp, "    return 0;\n");
    rustc_emit(comp, "}\n");

    return 0;
}

/*
 * Main rustc function
 */
int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("Usage: rustc <input.rs> <output>\n");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    /* Read input file */
    int fd = sys_open(input_file, 0); /* O_RDONLY */
    if (fd < 0) {
        printf("rustc: cannot open input file %s\n", input_file);
        return 1;
    }

    char input[32 * 1024]; /* 32KB limit */
    int64_t n = sys_read(fd, input, sizeof(input) - 1);
    sys_close(fd);

    if (n <= 0) {
        printf("rustc: cannot read input file\n");
        return 1;
    }
    input[n] = 0;

    /* Initialize compiler */
    rust_compiler_t comp;
    rustc_init(&comp, input);

    /* Lexical analysis */
    token_t *tok;
    while ((tok = rustc_lex(&comp)) && tok->type != TOK_EOF) {
        /* Tokenize complete */
    }

    /* Compile to C */
    if (rustc_compile(&comp) != 0) {
        printf("rustc: compilation failed\n");
        return 1;
    }

    /* Write output */
    fd = sys_open(output_file, 0x41); /* O_CREAT | O_WRONLY */
    if (fd < 0) {
        printf("rustc: cannot create output file %s\n", output_file);
        return 1;
    }

    sys_write(fd, comp.output, strlen(comp.output));
    sys_close(fd);

    printf("rustc: compiled %s -> %s.c\n", input_file, output_file);
    printf("rustc: use 'gcc %s.c -o %s' to build executable\n", output_file, output_file);

    return 0;
}