# quirewise-runtime

The shared ABI contract for the Quirewise packages: status codes, the format ID
vocabulary, conversion options/report types, the borrowed page/document facade
and two small buffer helpers.

`Quirewise::Runtime` is an independent, MIT-licensed C11 library, installed as
the `QuirewiseRuntime` CMake package (target `Quirewise::Runtime`). Most
Quirewise packages build on these declarations, and the package can be consumed
on its own without the Quirewise application.

## What it does

- Defines `QwStatus`, the status code space shared by package APIs (OK, invalid
  argument, out of memory, I/O, format, unsupported, decode, empty, external
  decoder required, resource limit).
- Defines `QwFormatId`, an append-only format vocabulary with 150 concrete IDs
  plus `QW_FORMAT_COUNT`, and the `QwInputFormat` / `QwOutputFormat` aliases
  used for direction capability.
- Defines the conversion contract: `QwConvertOptions` (18 fields, plus
  `QW_CONVERT_OPTIONS_INIT`), `QwConvertStage` and `QwConvertReport`.
- Defines the owned-output helpers `QwRuntimeBuffer`,
  `qw_runtime_buffer_alloc` and `qw_runtime_buffer_free`, the ABI version and
  status strings in `QwRuntimeStatus`.
- Defines the borrowed page facade `QwPageImage`, `QwDocument`, the
  acquire/release callbacks and `QW_DOCUMENT_REQUIRE_PDF_1_5`.
- Provides the `QW_API` shared-library export macro used by the public headers.

It contains **no codec, converter, detector or writer**. `QwFormatId` is a
vocabulary, not a registry: deciding which IDs an application supports is the
job of the packages that implement them. The package does no file I/O and never
interprets a format itself.

## Requirements

- C11 compiler and CMake 3.21 or newer.
- No package or third-party dependencies.
- CTest is used for the test suite.

## Build, test and install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/prefix
```

Shared builds use `-DQUIREWISE_BUILD_SHARED=ON`; for a non-default prefix, pass
`-DCMAKE_PREFIX_PATH` when configuring your own project. `Quirewise::Runtime` is
a compiled static or shared library (`src/runtime.c`), not header-only.

## Use it from CMake

```cmake
find_package(QuirewiseRuntime CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE Quirewise::Runtime)
```

Most projects get this target transitively: other Quirewise packages list
`Quirewise::Runtime` as a public dependency.

## Quick start

```c
#include <quirewise/format.h>
#include <quirewise/runtime.h>
#include <quirewise/status.h>

#include <stdio.h>

int main(void)
{
    QwRuntimeBuffer buffer = {0};
    QwConvertOptions options = QW_CONVERT_OPTIONS_INIT;
    QwConvertReport report = {0};

    printf("runtime ABI %u\n", qw_runtime_abi_version());
    printf("status text: %s\n", qw_runtime_status_string(QW_RUNTIME_OK));

    if (qw_runtime_buffer_alloc(64u, &buffer) != QW_RUNTIME_OK) {
        fputs("buffer allocation failed\n", stderr);
        return 1;
    }
    buffer.data[0] = 0x51u; /* the caller owns and may modify the bytes */
    printf("buffer: %zu byte(s)\n", buffer.size);
    qw_runtime_buffer_free(&buffer); /* clears data and size */

    options.audio_track = 0u;            /* 0 selects the default audio track */
    options.pdfa_level = QW_PDFA_NONE;   /* explicit default */
    report.format = QW_FORMAT_PDF;
    report.stage = QW_CONVERT_STAGE_OPEN;
    printf("options/report ready; note capacity=%zu, track=%zu\n",
           sizeof report.note, options.audio_track);
    return 0;
}
```

The example compiles with only `Quirewise::Runtime`: status codes, format IDs,
options and report types are plain C enums and structs.

## API overview

| Header | Purpose |
|---|---|
| `quirewise/status.h` | `QwStatus` code space and the `QW_API` export macro |
| `quirewise/format.h` | `QwFormatId`, `QwInputFormat`/`QwOutputFormat`, conversion options, stage and report |
| `quirewise/document.h` | `QwPageImage`, `QwDocument` and the page acquire/release contract |
| `quirewise/runtime.h` | ABI version, `QwRuntimeBuffer` helpers and `QwRuntimeStatus` strings |

| Task | API |
|---|---|
| Report success or failure | `QwStatus` (returned as `int` by package APIs) |
| Identify a format | `QwFormatId` constants, `QW_FORMAT_COUNT` |
| Configure a conversion | `QwConvertOptions`, `QW_CONVERT_OPTIONS_INIT`, `QwPdfaLevel`, `QwScanBinarizeMode`, `QwScanSplitMode` |
| Read a conversion outcome | `QwConvertReport` (`pages_written`, `output_size`, `skipped_pages`, `note`, `stage`) |
| Hand pages to a writer | `QwDocument`, `QwPageImage`, `QwPageAcquireFn`, `QwPageReleaseFn` |
| Own a returned buffer | `qw_runtime_buffer_alloc`, `qw_runtime_buffer_free` |
| Check ABI / text | `qw_runtime_abi_version`, `QUIREWISE_RUNTIME_ABI_VERSION`, `qw_runtime_status_string` |

## Behavior and limits

- `QwStatus` values are stable and deliberately non-contiguous: `0`, `-1` to
  `-6`, then `-8`, `-9` and `-11`. The missing values (`-7`, `-10`) are unused
  and must not be reused or invented by consumers.
- `QwFormatId` is append-only. New formats are added before `QW_FORMAT_COUNT`;
  existing numeric values never move, so stored IDs stay readable.
- `QwInputFormat` and `QwOutputFormat` are aliases of `QwFormatId`: the same ID
  can be an input in one package and an output in another, and capability is
  registered per direction by the implementing package.
- `QwConvertOptions` fields are optional. Zero-initialise with
  `QW_CONVERT_OPTIONS_INIT` and override only what you need; `0` means "default"
  for every numeric field and `NULL` means "not set" for pointers. New fields
  are appended, so the initializer must keep covering the whole struct.
- `QwConvertReport` is a plain output struct filled by the converting package.
  `note` is a fixed 192-byte buffer, so long messages are truncated by design.
- `QwDocument` and `QwPageImage` are **borrowed** views. PDF bytes stay owned by
  the caller, and a page source must keep its callbacks, context and page
  buffers valid until conversion returns. `release_page` is called once after
  every `acquire_page`, including acquisitions that returned an error.
- `qw_runtime_buffer_alloc(0, &buffer)` succeeds with `data == NULL` and
  `size == 0`. The helper is a convenience, not a general allocator: bytes are
  released with `qw_runtime_buffer_free`, which accepts `NULL` and zeroed
  structs.
- `QW_RUNTIME_RESOURCE_LIMIT` is part of the enum for helpers that enforce caps;
  the current buffer helpers do not return it.
- `QW_API` expands to `__declspec(dllexport/dllimport)` on Windows and to
  default visibility on other platforms when `QW_STATUS_BUILD_SHARED` is
  defined; static builds expand it to nothing.

## Testing

`ctest` runs an ABI test that checks the version macro, buffer allocation and
zero-size behavior, `NULL` argument rejection, and the exact text of every
`QwRuntimeStatus` string.

## Documentation

See [docs/usage.md](docs/usage.md) for the status model, option/report recipes,
the page facade contract and versioning rules.

## License

MIT. See [LICENSE](LICENSE).
