#!/bin/bash -eu
cd "$SRC"
rustc ${RUSTFLAGS:-} --edition=2021 -C debuginfo=1 -C opt-level=1 -C overflow-checks=on fuzz/resolve_fuzzer.rs \
  -C link-arg="$LIB_FUZZING_ENGINE" \
  -o "$OUT/resolve_fuzzer"
