# Image-View Changelog

## v0.1.x - **The PNG Update**

Incremental support for PNG image types is being added

### v0.1.4

* Added support for transparency ancillary chunks:
  * `tRNS` chunks can be processed through the `png_tRNS_*()` functions

### v0.1.3

* Added command-line option for a deep scan:
  * A normal scan (options `-s` or `--scan`) only reveal the chunks and their length
  * A deep scan reveals information pertinent to each chunk (first 10 palette entries, textual/date information, etc.)

### v0.1.2

* Added support for remaining critical chunks:
  * `PLTE` chunks can be processed through the `png_PLTE_*()` functions
  * `IDAT` chunks can be processed through the `png_IDAT_*()` functions
  * `IEND` chunks can be validated with the `png_IEND_valid()` function

### v0.1.1

* Added extraction for other image attributes:
  * Bit depth with `png_attr_bit_depth()`
  * Color type with `png_attr_col_type()`
  * Compression method with `png_attr_comp_method()`
  * Filter method with `png_attr_filter_method()`
  * Interlace method with `png_attr_il_method()`
* Added ability to quickly extract timestamp from `tIME` chunks in ISO 8601 format with `png_tIME_iso8601()`
* Added support for two new chunks:
  1. `pHYs` chunks are supported through `png_pHYs_*()` functions
  2. `zTXt` chunks are supported through `png_zTXt_*()` functions
* Other various improvements were made:
  * Memory management was cleaned up
  * Logging was enhanced and messaging was removed, to be implemented elsewhere

### v0.1.0

* Scan through PNG chunks with `png_open()`, `png_chunk_next()`; online documentation coming soon
* Some textual metadata supported:
  1. `tEXt` chunks are supported through `png_tEXt_*()` functions
  2. `iTXt` chunks are partially supported for keyword, language and text fields through `png_iTXt_*()` functions
  3. `tIME` chunks are supported for extracting each timestamp attribute through `png_tIME_*()` functions
* Easy attribute extraction for image width and height: `png_attr_w()` and `png_attr_h()`, respectively

## v0.0.0

* No features have been implemented yet
