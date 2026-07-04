# AfterQ

AfterQ is a Rust manifest resolver with a compact binary input format, a
libFuzzer-compatible harness, deterministic parser rules, include expansion,
memo trace decoding, and regression-oriented corpus seeds. The project is kept
small so fuzzing reaches real parser and resolver logic quickly.

## Format

Input begins with `AFQ1`, then a sequence of records. Each record has a one-byte
tag, a little-endian length, and a payload. Tag `1` defines a field. Tag `2`
adds an include target. Tag `3` defines memo trace data. Unknown records are
ignored so fuzzing can mutate around valid structures without losing coverage.

## Harness

`fuzz/resolve_fuzzer.rs` exports `LLVMFuzzerTestOneInput` and calls
`afterq::run_fuzz_input`. The harness accepts arbitrary bytes, parses valid
records, and resolves include targets. Corpus file `memo_trace_oob.bin` reaches
the memo expansion path.

## Local Commands

```sh
cargo test
cargo run -- fuzz/corpus/resolve_fuzzer/memo_trace_oob.bin
```

ClusterFuzzLite uses `.clusterfuzzlite/build.sh` to build `resolve_fuzzer`.

## Bug

The memo include path allocates a scratch buffer sized exactly to the decoded
memo payload and then appends a fixed marker after the payload. A crafted
`memo:#expand#...` include reaches a heap write past the allocation while normal
short includes and unknown includes avoid the path.
