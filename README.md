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
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_trivial       2.52 ns         2.52 ns    275172756
BM_ori           4.25 ns         4.25 ns    163576272
BM_kostja        4.40 ns         4.40 ns    156795515
```


### Clang 11.0.0

```
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_trivial       2.76 ns         2.76 ns    250890298
BM_ori           4.84 ns         4.83 ns    145255328
BM_kostja        8.48 ns         8.47 ns     82213166
```
