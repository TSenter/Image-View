## v0.1.0
**The PNG Update**  
Incremental support for PNG image types is being added
* Scan through PNG chunks with `png_open()`, `png_chunk_next()`; online documentation coming soon
* Some textual metadata supported:
  1. `tEXt` chunks are supported through `png_tEXt_*()` functions
  2. `iTXt` chunks are partially supported for keyword, language and text fields through `png_iTXt_*()` functions
  3. `tIME` chunks are supported for extracting each timestamp attribute through `png_tIME_*()` functions
* Easy attribute extraction for image width and height: `png_attr_w()` and `png_attr_h()`, respectively

## v0.0.0
* No features have been implemented yet