# Replacement Viterbi decoder

This directory contains a standalone, portable Viterbi decoder implementation that is intentionally separated from the repository's current DAB decoder. It is meant to provide a clear replacement implementation for testing, comparison, and validation without altering the original processing path in the production tree.

## Purpose

The original project contains a highly tuned Viterbi implementation that is optimized for the DAB code path and architecture-specific SIMD targets. This replacement implementation keeps the same overall design intent:

- soft-decision input handling,
- convolutional decoding using the Viterbi algorithm,
- puncturing and depuncturing support,
- a configurable traceback window,
- byte-oriented output.

The key difference is that the implementation is intentionally self-contained and easier to inspect and modify.

## Folder contents

- `viterbi_replacement.h` defines the public replacement decoder interface.
- `viterbi_replacement.cpp` implements the decoder logic.
- `CMakeLists.txt` exposes the implementation as a small standalone library target.

## Operational concept

The decoder works as follows:

1. Input soft values are collected as signed integer samples.
2. Optional puncture metadata is applied to restore the unpunctured symbol stream.
3. The trellis is walked forward, state-by-state, using branch metrics that compare expectation and measurement.
4. The best path is preserved for each state over the traceback window.
5. The final state is selected and the decoded bits are reconstructed and packed into bytes.

This preserves the standard Viterbi flow while making the implementation simpler and more portable than the architecture-specific decoder already used in the project.

## Why this was added

The repository already contains a production-tuned Viterbi decoder under `src/dab/algorithms/`. This replacement lives in a separate folder so it can be used for:

- comparison against the existing decoder,
- validation of algorithmic behavior,
- test harnesses and remote-call experimentation,
- future decoder evolution without disturbing the current pipeline.

## Build status

The replacement is exposed as a library target named `viterbi_replacement` and is included via the repository CMake tree under `src/`.

## Notes

This implementation is intentionally not a surgical in-place rewrite of the existing production decoder. It is a separate code path placed in a different folder so the original DAB processing flow remains unchanged while still providing a practical, replaceable decoder implementation.
