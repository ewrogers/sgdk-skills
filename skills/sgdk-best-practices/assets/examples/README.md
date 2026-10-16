# Small C and SDK contract examples

These are buildable examples, not a playable Lab or a game framework. Adapt the
pattern that solves your problem; there is no art, audio or gameplay prescription.

| Example | Problem it addresses |
| --- | --- |
| `checked.c` | Promoted integer intermediates, validated shifts, deliberate wrap and byte-format decoding without struct casts. |
| `pair.c` | Two sprite acquisitions that either both succeed or roll back, with private handles and repeatable cleanup. |
| `upload.c` | A short-lived palette source copied into SGDK's DMA queue, with recoverable failure and paired interrupt protection. |

`inc/` documents the public contracts. `checks.c` has small executable boundary
cases. Each implementation includes its own header first. The portable module
uses fixed-width host equivalents; SDK builds use the real SGDK types.

## Run the portable example

With a C17 compiler, from this folder:

```sh
cc -std=c17 -O2 -Wall -Wextra -Werror -DBP_HOST_MAIN -Iinc src/checked.c src/checks.c -o checks
./checks
```

It prints `C contracts OK`. On Windows, use Clang (`clang` in place of `cc`),
output `checks.exe` and run `.\checks.exe`. The portable example does not need
Docker, the repository test suite or another installed skill.

## Compile the SDK adapters

```sh
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest make release
```

Use `make clean` before switching to `make debug`. Both builds compile the
sprite and DMA adapters against the actual SDK. The tiny `main.c` returns the
portable check result for a direct-entry execution harness; `out/rom.bin` is
not an interactive visual test. A normal game supplies its own main loop.

Before using `pair.c`, initialize SGDK's sprite engine, provide long-lived
sprite definitions and own its cleanup. Before using `upload.c`, normal SDK
startup must have initialized DMA, and the caller must own the selected palette
slot. It expects sixteen aligned readable `u16` colors. Do not call this
foreground adapter from an ISR.

The repository additionally runs optimized host contracts, UBSan on Unix,
SDK release/debug builds and actual M68000 queue tests on ARM64 and AMD64.
These distinguish queue storage from real hardware transfers: no visual,
interrupt-timing or audio claim follows from a successful contract test.
