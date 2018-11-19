# Rust version

Almost the same as the c++ version but in rust. Due to time constraints, only the global vector sort has been implemented.

To use it, you need an up to date rust version:

```
curl https://sh.rustup.rs -sSf | sh
```

Then it is the same cli parameters as the c++ version. You can even discover the paremeters with the `--help` option:

```
cargo run --release -- --help
```

To get the top 10:

```
cargo run --release -- top 10 hn_logs.tsv
```
