# quirewise-runtime

Versioned runtime contract shared by the independent Quirewise packages.

CMake target: `Quirewise::Runtime`. The package is header-only.

## Public headers

- `quirewise/status.h` — the shared `QwStatus` code space (OK, invalid argument,
  out of memory, I/O, format, unsupported, decode, empty, external decoder
  required, resource limit) and `qw_buffer_free`.
- `quirewise/format.h` — the `QwFormatId` vocabulary (~150 stable IDs for input
  and output formats), `QwConvertOptions`, `QwConvertStage` and
  `QwConvertReport`. IDs are append-only so existing numeric values never move.
- `quirewise/document.h` — the borrowed page/document facade used by decoders
  and PDF writers (`QwPageImage`, `QwDocument`, page acquire/release callbacks).
- `quirewise/runtime.h` — ABI version and small helpers.

## Limits

The package contains no codec or converter; it only defines the shared contract.
`QwConvertReport::note` is a fixed 192-byte buffer and truncated reports are
expected. `QwConvertOptions` fields are optional; zero-initialise with
`QW_CONVERT_OPTIONS_INIT`.
