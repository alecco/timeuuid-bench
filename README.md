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

```
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_trivial       2.34 ns         2.33 ns    294829709
BM_ori           4.19 ns         4.19 ns    165290618
BM_kostja        3.74 ns         3.73 ns    185026775
```
