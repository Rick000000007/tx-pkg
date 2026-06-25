/* Minimal cJSON for TX Package Manager */
#ifndef CJSON_H
#define CJSON_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cJSON {
    struct cJSON *next; struct cJSON *prev; struct cJSON *child;
    int type; char *valuestring; int valueint; double valuedouble; char *string;
} cJSON;
#define cJSON_Invalid 0
#define cJSON_False   1
#define cJSON_True    2
#define cJSON_NULL    3
#define cJSON_Number  4
#define cJSON_String  5
#define cJSON_Array   6
#define cJSON_Object  7
cJSON *cJSON_Parse(const char *value);
char  *cJSON_Print(const cJSON *item);
void   cJSON_Delete(cJSON *item);
cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateBool(int b);
void cJSON_AddItemToObject(cJSON *object, const char *name, cJSON *item);
void cJSON_AddItemToArray(cJSON *array, cJSON *item);
void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string);
void cJSON_AddNumberToObject(cJSON *object, const char *name, double number);
void cJSON_AddBoolToObject(cJSON *object, const char *name, int b);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);
int    cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
#define cJSON_ArrayForEach(element, array) \
    for(element = (array != NULL) ? (array)->child : NULL; \
        element != NULL; element = element->next)
int cJSON_IsNumber(const cJSON *item);
int cJSON_IsString(const cJSON *item);
int cJSON_IsArray(const cJSON *item);
int cJSON_IsObject(const cJSON *item);
int cJSON_IsTrue(const cJSON *item);
#ifdef __cplusplus
}
#endif
#endif
