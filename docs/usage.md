# quirewise-runtime usage guide

This guide covers the shared contract in `Quirewise::Runtime`: the status code
space, the format vocabulary, conversion options and reports, the page facade
and the small runtime helpers.

## 1. Build and integrate

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/prefix
```

Consumer CMake:

```cmake
find_package(QuirewiseRuntime CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE Quirewise::Runtime)
```

There is no initialisation or teardown call. Most Quirewise packages link
`Quirewise::Runtime` publicly, so consumers often get the target transitively.

## 2. Status model

`QwStatus` is the shared code space. Package APIs return it as `int`:

| Value | Name | Meaning |
|---|---|---|
| 0 | `QW_STATUS_OK` | success |
| -1 | `QW_STATUS_ERR_INVALID_ARGUMENT` | null pointer, empty buffer, impossible argument |
| -2 | `QW_STATUS_ERR_OUT_OF_MEMORY` | allocation failed |
| -3 | `QW_STATUS_ERR_IO` | file or stream I/O failed |
| -4 | `QW_STATUS_ERR_FORMAT` | malformed or truncated data |
| -5 | `QW_STATUS_ERR_UNSUPPORTED` | valid input the native route does not handle |
| -6 | `QW_STATUS_ERR_DECODE` | decoded bytes failed validation |
| -8 | `QW_STATUS_ERR_EMPTY` | no payload at all |
| -9 | `QW_STATUS_ERR_EXTERNAL_DECODER_REQUIRED` | needs a decoder outside the native route |
| -11 | `QW_STATUS_ERR_RESOURCE_LIMIT` | an enforced cap was exceeded |

Concrete packages expose their own enum (for example `QwAudioStatus`,
`QwSubtitleStatus`) mapped from `QwStatus`, so a value never changes meaning
across APIs. Compare against `QW_STATUS_OK` explicitly and never invent the
unused values `-7` or `-10`.

## 3. Format IDs

`QwFormatId` is an append-only vocabulary of 150 concrete IDs plus
`QW_FORMAT_COUNT`. It is a naming contract, not a registry: a package decides
which IDs it accepts as input and which as output.

```c
QwInputFormat input = QW_FORMAT_PDF;
QwOutputFormat output = QW_FORMAT_PDF; /* aliases of QwFormatId */
```

`QwInputFormat` and `QwOutputFormat` are typedefs of `QwFormatId`, so the same
constant can be passed to both APIs. Whether an ID is valid for a direction is
documented by the package that implements it.

Rules that matter for persistence and IPC:

- An ID written by a newer build may be unknown to older code: keep a default.
- Never renumber, remove or reorder IDs; additions go before `QW_FORMAT_COUNT`.
- `QW_FORMAT_UNKNOWN` (`0`) means "unknown/undetected", not "any".

## 4. Recipe: prepare conversion options

`QwConvertOptions` has 18 fields, all optional. Zero means "default" for numbers
and `NULL` means "not set" for pointers. Start from the macro so new fields are
picked up automatically:

```c
#include <quirewise/format.h>

QwConvertOptions options = QW_CONVERT_OPTIONS_INIT;
const char *ekey = NULL; /* fill in when the input needs an EKey */

options.page_mode = 0;                 /* keep the default page mode */
options.pdfa_level = QW_PDFA_2B;       /* or QW_PDFA_NONE */
options.optimize = 1;                  /* apply /ebook during PDF/A */
options.audio_track = 2u;              /* 1-based; 0 selects the best track */
options.zip_password = "secret";       /* ZIP/CBZ and 7z/CB7 inputs */
options.qmc_ekey = ekey;               /* QMC STag/MusicEx, KGM v5 */
options.scan_binarize = QW_SCAN_BINARIZE_OTSU;
options.scan_split_mode = QW_SCAN_SPLIT_NONE;
```

Two habits keep this safe across upgrades:

- Assign field by field; use the macro rather than your own brace initializer.
- Recompile against the header you link: fields and the initializer grow by
  appending, and a shorter app-side initializer leaves the new tail zeroed.

## 5. Recipe: read a conversion report

`QwConvertReport` is a plain output struct filled in by the converting package;
`data`, `size` and `options` are the inputs from the previous steps:

```c
#include <quirewise/format.h>

QwConvertReport report = {0};
uint8_t *pdf = NULL;
size_t pdf_size = 0u;
int status = qw_convert_to_pdf_memory(data, size, &options, &pdf, &pdf_size,
                                      &report);

if (status != QW_STATUS_OK) {
    fprintf(stderr, "conversion failed at stage %d: %.192s\n",
            (int)report.stage, report.note);
    return 1;
}
printf("%zu page(s), %zu output byte(s), %zu skipped, format id %d\n",
       report.pages_written, report.output_size, report.skipped_pages,
       (int)report.format);
```

- `note` is a fixed 192-byte buffer; treat it as a short diagnostic, not as a
  complete error channel. It may be truncated and may be empty on success.
- `stage` (`QwConvertStage`) says where the last attempt stopped: `NONE`,
  `PROBE`, `OPEN`, `PLAN`, `EXECUTE` or `PUBLISH`.
- `format` / `output_format` echo the IDs involved; `pages_written`,
  `output_size` and `skipped_pages` are counts produced by the conversion.
- Zero-initialise the struct before the call and read it only after return.

The example above calls `Quirewise::Document`, which includes this header. If
you only link `Quirewise::Runtime`, this call is unavailable; the types here
are the contract, not the converter.

## 6. Recipe: provide a page source

`QwDocument` describes either PDF bytes or a lazily decoded page source. A page
source keeps memory bounded for large books: pages are acquired on demand and
released immediately after use.

```c
#include <quirewise/document.h>

/* Application state; the converter only sees the void *context. */
typedef struct PageStore PageStore;

static int page_acquire(void *context, size_t index, QwPageImage *out_page)
{
    PageStore *store = (PageStore *)context;

    if (index >= store->count) return 1; /* nonzero aborts the conversion */
    out_page->data = store->pages[index].pixels;  /* borrowed */
    out_page->size = store->pages[index].size;
    out_page->kind = QW_PAGE_IMAGE_RGB24;
    out_page->width = store->pages[index].width;
    out_page->height = store->pages[index].height;
    return 0;
}

static void page_release(void *context, QwPageImage *page)
{
    (void)context; (void)page; /* the store owns the pixels; nothing to free */
}

QwDocument document = {0};
document.kind = QW_DOCUMENT_PAGE_SOURCE;
document.page_count = store.count;
document.acquire_page = page_acquire;
document.release_page = page_release;
document.context = &store;
```

Contract details:

- `QwPageImage::kind` is one of `QW_PAGE_IMAGE_JPEG`, `..._CCITT`, `..._GRAY8`,
  `..._RGB24`, `..._JP2`, `..._JP2_ALPHA`, `..._JP2_ALPHA_PREMULTIPLIED`.
  `width` and `height` are required for every kind except JPEG, where they may be
  read from the stream.
- `acquire_page` returns `0` on success; any nonzero code aborts the conversion.
  `release_page` is called once after every acquisition, including failed ones.
- PDF inputs use `kind = QW_DOCUMENT_PDF_BYTES` with `pdf_data` / `pdf_size` and
  no callbacks; those bytes stay owned by the caller. Set
  `QW_DOCUMENT_REQUIRE_PDF_1_5` in `flags` if the writer must emit PDF 1.5+.
- `dispose` is an optional hook the producer can use to release `context`.

## 7. Recipe: own a returned buffer

`QwRuntimeBuffer` is a two-field pair used by helpers that return allocated
memory:

```c
#include <quirewise/runtime.h>

QwRuntimeBuffer buffer = {0};

if (qw_runtime_buffer_alloc(4096u, &buffer) != QW_RUNTIME_OK) {
    return 1;
}
/* use buffer.data for buffer.size bytes */
qw_runtime_buffer_free(&buffer); /* data == NULL, size == 0 afterwards */
```

`qw_runtime_buffer_alloc(0, &buffer)` succeeds with `data == NULL` and
`size == 0`, which makes "allocate N, where N may be 0" branches unnecessary.
Freeing is idempotent: `qw_runtime_buffer_free` accepts `NULL` and zeroed
structs. This is not a general-purpose allocator and not a growable buffer;
package outputs that are `malloc`-owned use the package's own release function.

## 8. ABI and versioning

- `QUIREWISE_RUNTIME_ABI_VERSION` is `1`; `qw_runtime_abi_version()` returns it.
  Compare the two when loading a shared library dynamically.
- Enumerators are diff-stable: append only. `QwStatus` keeps its gaps, and
  `QwFormatId` never renumbers.
- Structs are plain C aggregates with appended fields (`QwConvertReport::stage`
  is documented as appended). Do not mix headers and binaries from different
  releases; rebuild consumers with the library.
- `QW_API` controls symbol visibility: define `QW_STATUS_BUILD_SHARED` for a
  shared build and `QW_STATUS_EXPORTS` while building the library itself. Static
  builds expand it to nothing.

## 9. Memory and threading

- All types here are value types; the package holds no global state, so the
  helpers are safe to call from any thread.
- `qw_runtime_buffer_alloc` and `qw_runtime_buffer_free` use the process heap;
  each buffer must be freed exactly once, by the same allocator/runtime.
- `QwDocument` callbacks are invoked synchronously by the consumer; the context
  and any page buffers must stay valid until conversion returns.
- `qw_runtime_status_string` returns string literals and never allocates.

## 10. Limits and gotchas

- `QwConvertReport::note` holds 192 bytes including the terminating NUL.
- `QwConvertOptions` may be `NULL` in APIs that accept a pointer; `NULL` means
  "all defaults", not the same as uninitialised fields.
- `QwDocument` has no ownership of `pdf_data`, page data or `context`; the
  consumer must not free them through the library.

## 11. Related packages

- `Quirewise::Document` implements conversion and includes these headers
  publicly; it maps failures to `QwStatus` and fills `QwConvertReport`.
- `Quirewise::Audio`, `Quirewise::Subtitle` and the other format packages expose
  their own status enums derived from `QwStatus` and their own option structs.
- `Quirewise::Pipeline` and the application link this package transitively and
  use `QwFormatId` to route inputs to the right reader.

## 12. License

MIT. See `LICENSE`.
