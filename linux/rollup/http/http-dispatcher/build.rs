extern crate cc;

fn main() {
    println!("cargo:rerun-if-changed=src/rollup/bindings.c,src/rollup/bindings.h");
    cc::Build::new()
        .file("src/rollup/bindings.c")
        .compile("libbindings.a");
    println!("cargo:rustc-link-lib=bindings");
}
