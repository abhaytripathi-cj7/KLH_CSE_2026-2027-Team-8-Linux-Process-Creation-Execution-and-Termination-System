#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *value; int operator; } Token;
static int name_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int name_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

static int add_char(char *out, size_t *used, char c) {
    if (*used >= 8191) return -1;
    out[(*used)++] = c; out[*used] = 0; return 0;
}

static int expand(const char *input, size_t *pos, char *out, size_t *used, int depth) {
    size_t start = *pos + 1, end = start;
    int braced = input[start] == '{';
    if (braced) { start++; end++; }
    if (!name_start(input[end])) return add_char(out, used, input[(*pos)++]);
    while (name_char(input[end])) end++;
    if (braced && input[end] != '}') return -1;
    char name[256];
    if (end - start >= sizeof name) return -1;
    memcpy(name, input + start, end - start); name[end - start] = 0;
    *pos = end + (braced ? 1 : 0);
    const char *value = getenv(name);
    if (!value) value = "";
    if (depth >= 8) return -1;
    for (size_t i = 0; value[i];) {
        if (value[i] == '$') {
            if (expand(value, &i, out, used, depth + 1) != 0) return -1;
        } else if (add_char(out, used, value[i++]) != 0) return -1;
    }
    return 0;
}

static int token_add(Token *tokens, int *count, const char *value, int op) {
    if (*count >= 256) return -1;
    tokens[*count].value = strdup(value);
    if (!tokens[*count].value) return -1;
    tokens[(*count)++].operator = op; return 0;
}

static void tokens_free(Token *tokens, int count) {
    for (int i = 0; i < count; ++i) free(tokens[i].value);
}

static int tokenize(const char *line, Token *tokens, int *count, char *err, size_t errcap) {
    *count = 0;
    for (size_t i = 0; line[i];) {
        if (isspace((unsigned char)line[i])) { i++; continue; }
        if (line[i] == '#') break;
        const char *ops[] = { "2>&1", "2>>", "2>", ">>", "|", "<", ">", "&" };
        int matched = 0;
        for (size_t k = 0; k < sizeof ops / sizeof ops[0]; ++k) {
            size_t n = strlen(ops[k]);
            if (strncmp(line + i, ops[k], n) == 0) {
                if (token_add(tokens, count, ops[k], 1) != 0) goto large;
                i += n; matched = 1; break;
            }
        }
        if (matched) continue;
        char word[8192] = ""; size_t used = 0;
        int quoted = 0;
        while (line[i]) {
            char c = line[i];
            if (c == '\\' && line[i + 1]) {
                i++; if (add_char(word, &used, line[i++]) != 0) goto large; quoted = 1; continue;
            }
            if (c == '\'' || c == '"') {
                char quote = c; quoted = 1; i++;
                while (line[i] && line[i] != quote) {
                    if (quote == '"' && line[i] == '$') {
                        if (expand(line, &i, word, &used, 0) != 0) goto badvar;
                    } else if (quote == '"' && line[i] == '\\' && line[i + 1]) {
                        i++; if (add_char(word, &used, line[i++]) != 0) goto large;
                    } else if (add_char(word, &used, line[i++]) != 0) goto large;
                }
                if (!line[i]) { snprintf(err, errcap, "unclosed quote"); return -1; }
                i++; continue;
            }
            if (isspace((unsigned char)c) || strchr("|<>&", c) ||
                (c == '2' && line[i + 1] == '>')) break;
            if (c == '$') { if (expand(line, &i, word, &used, 0) != 0) goto badvar; }
            else if (add_char(word, &used, line[i++]) != 0) goto large;
        }
        if (used || quoted) { if (token_add(tokens, count, word, 0) != 0) goto large; }
    }
    return 0;
large: snprintf(err, errcap, "command or expansion too large"); return -1;
badvar: snprintf(err, errcap, "invalid or recursive variable expansion"); return -1;
}

void free_pipeline(Pipeline *p) {
    for (int c = 0; c < p->count; ++c) {
        for (int i = 0; i < p->commands[c].argc; ++i) free(p->commands[c].argv[i]);
        for (int i = 0; i < p->commands[c].nredirs; ++i) free(p->commands[c].redirs[i].path);
    }
    free(p->text); memset(p, 0, sizeof *p);
}

int parse_line(const char *line, Pipeline *p, char *err, size_t errcap) {
    memset(p, 0, sizeof *p);
    Token tokens[256] = {0}; int count = 0;
    if (tokenize(line, tokens, &count, err, errcap) != 0) { tokens_free(tokens, count); return -1; }
    if (!count) return 0;
    p->text = strdup(line); p->count = 1;
    if (!p->text) goto oom;
    for (int i = 0; i < count; ++i) {
        Command *cmd = &p->commands[p->count - 1];
        char *t = tokens[i].value;
        if (tokens[i].operator && strcmp(t, "|") == 0) {
            if (!cmd->argc || p->count >= MAX_COMMANDS) goto syntax;
            p->count++; continue;
        }
        if (tokens[i].operator && strcmp(t, "&") == 0) {
            if (i != count - 1 || !cmd->argc) goto syntax;
            p->background = 1; continue;
        }
        if (tokens[i].operator) {
            if (cmd->nredirs >= MAX_REDIRS) goto syntax;
            Redir *r = &cmd->redirs[cmd->nredirs++];
            if (strcmp(t, "2>&1") == 0) { r->type = R_ERR_TO_OUT; continue; }
            if (++i >= count || tokens[i].operator) goto syntax;
            r->type = strcmp(t, "<") == 0 ? R_IN : strcmp(t, ">") == 0 ? R_OUT :
                strcmp(t, ">>") == 0 ? R_APPEND : strcmp(t, "2>") == 0 ? R_ERR : R_ERR_APPEND;
            r->path = strdup(tokens[i].value);
            if (!r->path) goto oom;
            continue;
        }
        if (cmd->argc >= MAX_ARGS - 1) goto syntax;
        cmd->argv[cmd->argc] = strdup(t);
        if (!cmd->argv[cmd->argc]) goto oom;
        cmd->argc++;
    }
    if (!p->commands[p->count - 1].argc) goto syntax;
    tokens_free(tokens, count); return 0;
syntax: snprintf(err, errcap, "invalid pipeline or redirection syntax"); goto fail;
oom: snprintf(err, errcap, "out of memory");
fail: tokens_free(tokens, count); free_pipeline(p); return -1;
}
