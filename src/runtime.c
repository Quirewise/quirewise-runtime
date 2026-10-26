#include "quirewise/runtime.h"
#include <stdlib.h>
#include <string.h>
uint32_t qw_runtime_abi_version(void){return QUIREWISE_RUNTIME_ABI_VERSION;}
QwRuntimeStatus qw_runtime_buffer_alloc(size_t size,QwRuntimeBuffer *out){if(!out)return QW_RUNTIME_INVALID_ARGUMENT;out->data=NULL;out->size=0;if(!size)return QW_RUNTIME_OK;out->data=(uint8_t*)malloc(size);if(!out->data)return QW_RUNTIME_OUT_OF_MEMORY;out->size=size;return QW_RUNTIME_OK;}
void qw_runtime_buffer_free(QwRuntimeBuffer *buffer){if(!buffer)return;free(buffer->data);buffer->data=NULL;buffer->size=0;}
const char *qw_runtime_status_string(QwRuntimeStatus s){switch(s){case QW_RUNTIME_OK:return "ok";case QW_RUNTIME_INVALID_ARGUMENT:return "invalid argument";case QW_RUNTIME_OUT_OF_MEMORY:return "out of memory";case QW_RUNTIME_RESOURCE_LIMIT:return "resource limit";default:return "unknown";}}
