#![no_main]

#[path = "../src/lib.rs"]
mod afterq;

#[no_mangle]
pub extern "C" fn LLVMFuzzerTestOneInput(data: *const u8, size: usize) -> i32 {
    if data.is_null() {
        return 0;
    }
    let bytes = unsafe { std::slice::from_raw_parts(data, size) };
    afterq::run_fuzz_input(bytes);
    0
}
