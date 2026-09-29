# Image decoding limits

The diffusion addon's `image_codec::decodeImage` accepts PNG and JPEG input for img2img, video guidance, ESRGAN upscaling, and ABot-World scene creation. It returns an empty `sd_image_t` for invalid or over-limit input with a failure reason; callers report an `InvalidArgument` error.

Before full decode, the wrapper checks the format signature, rejects compressed input above 50 MiB, reads dimensions with `stbi_info_from_memory`, and rejects images above 16384 pixels on either edge or 67,108,864 total pixels (64 Mi pixels). PNG compressed data must inflate to exactly the size declared by its header; progressive JPEGs are limited to 32 scans. High-memory sources (16-bit PNG and four-component JPEG) have a 128 MiB source-data budget. The decoder requests three RGB channels and confirms the dimensions returned by the full decode match the inspected header. Multi-reference img2img and video jobs retain at most 134,217,728 decoded input pixels (128 Mi pixels) across their input images. Single-image img2img, ESRGAN, and ABot-World use the per-image limit. The limits apply to the native entry points as well as the JavaScript API. ESRGAN also rejects outputs above 64 Mi pixels or 16,384 pixels on either edge before upscaling.

The decoder compiles only PNG and JPEG loaders; HDR, TGA, PSD, PIC, PNM, GIF, and BMP are disabled. `STBI_MAX_DIMENSIONS` is set to 16384 as an additional bound within stb. Decoded buffers use `image_codec::FreeDeleter` so exceptions release them.

`encodeToPng` and `encodeToJpeg` separately reject null data, invalid dimensions or channels, and unsupported values for their encoder APIs. Encoding is not subject to the input byte or pixel limits above.

The C++ regression tests in `test/unit/test_image_decoding_limits.cpp` cover PNG and JPEG decoding, unsupported formats, compressed size, inflated PNG size, JPEG scans, pixel and remaining-job budgets, and encoding validation. Run them with `npm run test:cpp`.

Report suspected vulnerabilities through the repository's `SECURITY.md` disclosure process.
