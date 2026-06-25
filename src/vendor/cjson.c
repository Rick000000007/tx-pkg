/* Minimal cJSON for TX Package Manager - Implementation */
#include "cjson.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <limits.h>

static char *cjson_strdup(const char *s) {
    size_t n; char *d;
    if (!s) return NULL;
    n = strlen(s) + 1;
    d = (char *)malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}
static const char *skip_ws(const char *p) {
    while (p && *p && (unsigned char)*p <= ' ') p++;
    return p;
}
static const char *parse_string(cJSON *item, const char *str) {
    const char *p = str + 1;
    char *out, *o;
    size_t len = 0;
    if (*str != '"') return NULL;
    while (*p != '"' && *p && ++len) if (*p++ == '\\') p++;
    out = (char *)malloc(len + 1);
    if (!out) return NULL;
    p = str + 1; o = out;
    while (*p != '"' && *p) {
        if (*p != '\\') { *o++ = *p++; }
        else {
            p++;
            switch (*p) {
                case 'b': *o++ = '\b'; break; case 'f': *o++ = '\f'; break;
                case 'n': *o++ = '\n'; break; case 'r': *o++ = '\r'; break;
                case 't': *o++ = '\t'; break; case '"': *o++ = '"'; break;
                case '\\': *o++ = '\\'; break; default: *o++ = *p; break;
            }
            p++;
        }
    }
    *o = '\0';
    if (*p == '"') p++;
    item->valuestring = out;
    item->type = cJSON_String;
    return p;
}
static const char *parse_number(cJSON *item, const char *num) {
    double n = 0, sign = 1, scale = 0;
    int ss = 0, sss = 1;
    if (*num == '-') { sign = -1; num++; }
    if (*num == '0') num++;
    if (*num >= '1' && *num <= '9')
        do { n = (n * 10.0) + (*num++ - '0'); } while (*num >= '0' && *num <= '9');
    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
        num++;
        do { n = (n * 10.0) + (*num++ - '0'); scale--; } while (*num >= '0' && *num <= '9');
    }
    if (*num == 'e' || *num == 'E') {
        num++;
        if (*num == '+') num++;
        else if (*num == '-') { sss = -1; num++; }
        while (*num >= '0' && *num <= '9') ss = (ss * 10) + (*num++ - '0');
    }
    n = sign * n * pow(10.0, (scale + ss * sss));
    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;
    return num;
}
static const char *parse_value(cJSON *item, const char *value);
static const char *parse_array(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '[') return NULL;
    item->type = cJSON_Array;
    value = skip_ws(value + 1);
    if (*value == ']') return value + 1;
    item->child = child = (cJSON *)calloc(1, sizeof(cJSON));
    if (!item->child) return NULL;
    value = skip_ws(parse_value(child, skip_ws(value)));
    if (!value) return NULL;
    while (*value == ',') {
        cJSON *ni = (cJSON *)calloc(1, sizeof(cJSON));
        if (!ni) return NULL;
        child->next = ni; ni->prev = child; child = ni;
        value = skip_ws(parse_value(child, skip_ws(value + 1)));
        if (!value) return NULL;
    }
    if (*value == ']') return value + 1;
    return NULL;
}
static const char *parse_object(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '{') return NULL;
    item->type = cJSON_Object;
    value = skip_ws(value + 1);
    if (*value == '}') return value + 1;
    item->child = child = (cJSON *)calloc(1, sizeof(cJSON));
    if (!item->child) return NULL;
    value = skip_ws(parse_string(child, value));
    if (!value) return NULL;
    child->string = child->valuestring; child->valuestring = NULL;
    value = skip_ws(value); if (*value != ':') return NULL;
    value = skip_ws(parse_value(child, skip_ws(value + 1)));
    if (!value) return NULL;
    while (*value == ',') {
        cJSON *ni = (cJSON *)calloc(1, sizeof(cJSON));
        if (!ni) return NULL;
        child->next = ni; ni->prev = child; child = ni;
        value = skip_ws(parse_string(child, skip_ws(value + 1)));
        if (!value) return NULL;
        child->string = child->valuestring; child->valuestring = NULL;
        value = skip_ws(value); if (*value != ':') return NULL;
        value = skip_ws(parse_value(child, skip_ws(value + 1)));
        if (!value) return NULL;
    }
    if (*value == '}') return value + 1;
    return NULL;
}
static const char *parse_value(cJSON *item, const char *value) {
    if (!value) return NULL;
    if (!strncmp(value, "null", 4)) { item->type = cJSON_NULL; return value + 4; }
    if (!strncmp(value, "false", 5)) { item->type = cJSON_False; return value + 5; }
    if (!strncmp(value, "true", 4)) { item->type = cJSON_True; return value + 4; }
    if (*value == '"') return parse_string(item, value);
    if (*value == '-' || (*value >= '0' && *value <= '9')) return parse_number(item, value);
    if (*value == '[') return parse_array(item, value);
    if (*value == '{') return parse_object(item, value);
    return NULL;
}
cJSON *cJSON_Parse(const char *value) {
    cJSON *c = (cJSON *)calloc(1, sizeof(cJSON));
    if (!c) return NULL;
    const char *end = parse_value(c, skip_ws(value));
    if (!end) { cJSON_Delete(c); return NULL; }
    end = skip_ws(end); if (*end) { cJSON_Delete(c); return NULL; }
    return c;
}
static int ensure_buf(char **buf, size_t *size, size_t need) {
    if (need >= *size) {
        size_t ns = need + 256;
        char *nb = (char *)realloc(*buf, ns);
        if (!nb) return 0;
        *buf = nb; *size = ns;
    }
    return 1;
}
static void buf_append(char **buf, size_t *size, size_t *len, const char *s) {
    size_t sl = strlen(s);
    if (!ensure_buf(buf, size, *len + sl + 1)) return;
    strcpy(*buf + *len, s); *len += sl;
}
static char *print_string(const char *str) {
    const char *p; size_t len = 2; char *out, *o;
    if (!str) { out = (char *)malloc(5); if (out) strcpy(out, "null"); return out; }
    for (p = str; *p; p++) {
        switch (*p) { case '\b': case '\f': case '\n': case '\r': case '\t': case '"': case '\\': len += 2; break; default: len++; break; }
    }
    out = (char *)malloc(len + 1); if (!out) return NULL;
    o = out; *o++ = '"';
    for (p = str; *p; p++) {
        switch (*p) {
            case '\b': *o++ = '\\'; *o++ = 'b'; break; case '\f': *o++ = '\\'; *o++ = 'f'; break;
            case '\n': *o++ = '\\'; *o++ = 'n'; break; case '\r': *o++ = '\\'; *o++ = 'r'; break;
            case '\t': *o++ = '\\'; *o++ = 't'; break; case '"': *o++ = '\\'; *o++ = '"'; break;
            case '\\': *o++ = '\\'; *o++ = '\\'; break; default: *o++ = *p; break;
        }
    }
    *o++ = '"'; *o = '\0'; return out;
}
static void print_value_to_buf(const cJSON *item, char **buf, size_t *size, size_t *len);
static void print_array_to_buf(const cJSON *item, char **buf, size_t *size, size_t *len) {
    cJSON *child; int first = 1;
    buf_append(buf, size, len, "[");
    child = item->child;
    while (child) {
        if (!first) buf_append(buf, size, len, ","); first = 0;
        print_value_to_buf(child, buf, size, len); child = child->next;
    }
    buf_append(buf, size, len, "]");
}
static void print_object_to_buf(const cJSON *item, char **buf, size_t *size, size_t *len) {
    cJSON *child; int first = 1;
    buf_append(buf, size, len, "{");
    child = item->child;
    while (child) {
        if (!first) buf_append(buf, size, len, ","); first = 0;
        { char *key = print_string(child->string);
          if (key) { buf_append(buf, size, len, key); free(key); } }
        buf_append(buf, size, len, ":");
        print_value_to_buf(child, buf, size, len); child = child->next;
    }
    buf_append(buf, size, len, "}");
}
static void print_value_to_buf(const cJSON *item, char **buf, size_t *size, size_t *len) {
    if (!item) { buf_append(buf, size, len, "null"); return; }
    switch (item->type) {
        case cJSON_NULL: buf_append(buf, size, len, "null"); break;
        case cJSON_False: buf_append(buf, size, len, "false"); break;
        case cJSON_True: buf_append(buf, size, len, "true"); break;
        case cJSON_Number: {
            char num[64];
            snprintf(num, sizeof(num), "%.15g", item->valuedouble);
            buf_append(buf, size, len, num); break;
        }
        case cJSON_String: { char *s = print_string(item->valuestring);
            if (s) { buf_append(buf, size, len, s); free(s); } break; }
        case cJSON_Array: print_array_to_buf(item, buf, size, len); break;
        case cJSON_Object: print_object_to_buf(item, buf, size, len); break;
        default: buf_append(buf, size, len, "null"); break;
    }
}
char *cJSON_Print(const cJSON *item) {
    size_t size = 256, len = 0;
    char *buf = (char *)malloc(size);
    if (!buf) return NULL;
    buf[0] = '\0';
    print_value_to_buf(item, &buf, &size, &len);
    return buf;
}
void cJSON_Delete(cJSON *c) {
    cJSON *n;
    while (c) { n = c->next; if (c->child) cJSON_Delete(c->child);
        free(c->valuestring); free(c->string); free(c); c = n; }
}
cJSON *cJSON_CreateObject(void) { cJSON *i = (cJSON *)calloc(1, sizeof(cJSON)); if (i) i->type = cJSON_NULL; return i; }
cJSON *cJSON_CreateArray(void) { cJSON *i = cJSON_CreateObject(); if (i) i->type = cJSON_Array; return i; }
cJSON *cJSON_CreateString(const char *s) { cJSON *i = cJSON_CreateObject(); if (i) { i->type = cJSON_String; i->valuestring = cjson_strdup(s ? s : ""); } return i; }
cJSON *cJSON_CreateNumber(double n) { cJSON *i = cJSON_CreateObject(); if (i) { i->type = cJSON_Number; i->valuedouble = n; i->valueint = (int)n; } return i; }
cJSON *cJSON_CreateBool(int b) { cJSON *i = cJSON_CreateObject(); if (i) i->type = b ? cJSON_True : cJSON_False; return i; }
void cJSON_AddItemToObject(cJSON *o, const char *n, cJSON *item) {
    cJSON *c;
    if (!o || !n || !item) return;
    if (!o->child) { o->child = item; }
    else { c = o->child; while (c->next) c = c->next; c->next = item; item->prev = c; }
    free(item->string); item->string = cjson_strdup(n);
}
void cJSON_AddItemToArray(cJSON *a, cJSON *item) {
    cJSON *c;
    if (!a || !item) return;
    if (!a->child) { a->child = item; }
    else { c = a->child; while (c->next) c = c->next; c->next = item; item->prev = c; }
}
void cJSON_AddStringToObject(cJSON *o, const char *n, const char *s) { cJSON_AddItemToObject(o, n, cJSON_CreateString(s)); }
void cJSON_AddNumberToObject(cJSON *o, const char *n, double v) { cJSON_AddItemToObject(o, n, cJSON_CreateNumber(v)); }
void cJSON_AddBoolToObject(cJSON *o, const char *n, int b) { cJSON_AddItemToObject(o, n, cJSON_CreateBool(b)); }
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *o, const char *s) {
    cJSON *c;
    if (!o || !s) return NULL;
    c = o->child;
    while (c) { if (c->string && strcmp(c->string, s) == 0) return c; c = c->next; }
    return NULL;
}
int cJSON_GetArraySize(const cJSON *a) { int n = 0; cJSON *c; if (!a) return 0; c = a->child; while (c) { n++; c = c->next; } return n; }
cJSON *cJSON_GetArrayItem(const cJSON *a, int idx) { cJSON *c; if (!a) return NULL; c = a->child; while (c && idx-- > 0) c = c->next; return c; }
int cJSON_IsNumber(const cJSON *i) { return i && i->type == cJSON_Number; }
int cJSON_IsString(const cJSON *i) { return i && i->type == cJSON_String; }
int cJSON_IsArray(const cJSON *i) { return i && i->type == cJSON_Array; }
int cJSON_IsObject(const cJSON *i) { return i && i->type == cJSON_Object; }
int cJSON_IsTrue(const cJSON *i) { return i && i->type == cJSON_True; }
