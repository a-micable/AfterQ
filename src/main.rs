use std::env;
use std::fs;

fn main() {
    let Some(path) = env::args().nth(1) else {
        eprintln!("usage: afterq <input>");
        std::process::exit(2);
    };
    let data = fs::read(path).expect("read input");
    afterq::run_fuzz_input(&data);
}
