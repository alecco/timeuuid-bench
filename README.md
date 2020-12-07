# timeuuid-bench

Benchmark different implementations of Cassandra's timeuuid  comparisons

## Install deps

```
dnf install google-benchmark-devel
dnf install cpupowerutils
```


## Build
```
cmake -B build .
cmake --build build
```

## Run
```
sudo cpupower frequency-set -g performance  # Disable CPU frequency scaling
./build/benchmark
sudo cpupower frequency-set -g powersave    # Re-enable CPU frequency scaling
```

## Notes

There's a trivial implementation for base reference (cost of anti-optimizations).

## Compilers

clang++ vectorizes aggressively but performs slightly slower.
g++ does not vectorize but seems to focus on data dependencies and produces faster code.

## Typical output

### GCC 10.2.1

```
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_trivial       2.49 ns         2.49 ns    270171299
BM_ori           4.31 ns         4.30 ns    159428276
BM_kostja        3.91 ns         3.90 ns    178458727
```


### Clang 11.0.0

```
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_trivial       2.30 ns         2.29 ns    300451685
BM_ori           4.16 ns         4.16 ns    166757347
BM_kostja        7.62 ns         7.61 ns     87890182
```
