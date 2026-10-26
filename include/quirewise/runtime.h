#ifndef QUIREWISE_RUNTIME_H
#define QUIREWISE_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define QUIREWISE_RUNTIME_ABI_VERSION 1u
typedef enum QwRuntimeStatus { QW_RUNTIME_OK=0, QW_RUNTIME_INVALID_ARGUMENT=-1, QW_RUNTIME_OUT_OF_MEMORY=-2, QW_RUNTIME_RESOURCE_LIMIT=-3 } QwRuntimeStatus;
typedef struct QwRuntimeBuffer { uint8_t *data; size_t size; } QwRuntimeBuffer;
uint32_t qw_runtime_abi_version(void);
QwRuntimeStatus qw_runtime_buffer_alloc(size_t size, QwRuntimeBuffer *out);
void qw_runtime_buffer_free(QwRuntimeBuffer *buffer);
const char *qw_runtime_status_string(QwRuntimeStatus status);
#ifdef __cplusplus
}
#endif
#endif
