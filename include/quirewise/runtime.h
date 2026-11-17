#ifndef QUIREWISE_RUNTIME_H
#define QUIREWISE_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*! ABI version of the runtime types; compare with qw_runtime_abi_version(). */
#define QUIREWISE_RUNTIME_ABI_VERSION 1u
/*! Status codes shared by the runtime buffer helpers. */
typedef enum QwRuntimeStatus { QW_RUNTIME_OK=0, QW_RUNTIME_INVALID_ARGUMENT=-1, QW_RUNTIME_OUT_OF_MEMORY=-2, QW_RUNTIME_RESOURCE_LIMIT=-3 } QwRuntimeStatus;
/*! Malloc-owned byte buffer returned by qw_runtime_buffer_alloc(). */
typedef struct QwRuntimeBuffer { uint8_t *data; size_t size; } QwRuntimeBuffer;
/*! Return the ABI version compiled into the installed runtime package. */
uint32_t qw_runtime_abi_version(void);
/*! Allocate a zero-filled buffer of size bytes; release with
 *  qw_runtime_buffer_free(). Returns QW_RUNTIME_INVALID_ARGUMENT for a NULL
 *  output and QW_RUNTIME_OUT_OF_MEMORY when the allocation fails. */
QwRuntimeStatus qw_runtime_buffer_alloc(size_t size, QwRuntimeBuffer *out);
/*! Release a buffer returned by qw_runtime_buffer_alloc(); NULL is accepted
 *  and the struct is reset so a second call is safe. */
void qw_runtime_buffer_free(QwRuntimeBuffer *buffer);
/*! Return a static human-readable name for a runtime status code. */
const char *qw_runtime_status_string(QwRuntimeStatus status);
#ifdef __cplusplus
}
#endif
#endif
