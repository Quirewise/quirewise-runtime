#include <quirewise/runtime.h>

#include <stdint.h>
#include <string.h>

int main(void)
{
    QwRuntimeBuffer buffer;

    if (qw_runtime_abi_version() != QUIREWISE_RUNTIME_ABI_VERSION) {
        return 1;
    }

    memset(&buffer, 0, sizeof(buffer));
    if (qw_runtime_buffer_alloc(32, &buffer) != QW_RUNTIME_OK) {
        return 2;
    }
    if (!buffer.data || buffer.size < 32) {
        return 3;
    }
    qw_runtime_buffer_free(&buffer);
    if (buffer.data != NULL || buffer.size != 0) {
        return 4;
    }

    if (strcmp(qw_runtime_status_string(QW_RUNTIME_OK), "ok") != 0) {
        return 5;
    }
    if (strcmp(qw_runtime_status_string(QW_RUNTIME_INVALID_ARGUMENT),
               "invalid argument") != 0) {
        return 6;
    }
    if (strcmp(qw_runtime_status_string(QW_RUNTIME_OUT_OF_MEMORY),
               "out of memory") != 0) {
        return 7;
    }
    if (strcmp(qw_runtime_status_string(QW_RUNTIME_RESOURCE_LIMIT),
               "resource limit") != 0) {
        return 8;
    }

    if (qw_runtime_buffer_alloc(4, NULL) != QW_RUNTIME_INVALID_ARGUMENT) {
        return 9;
    }
    if (qw_runtime_buffer_alloc(0, &buffer) != QW_RUNTIME_OK ||
        buffer.data != NULL || buffer.size != 0) {
        return 10;
    }

    return 0;
}
