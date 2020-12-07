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
BM_trivial      0.689 ns        0.688 ns    972742139
BM_ori           2.48 ns         2.48 ns    275273446
BM_kostja        2.62 ns         2.62 ns    262103218
```


### Clang 11.0.0

```
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_trivial      0.579 ns        0.578 ns   1000000000
BM_ori           2.39 ns         2.38 ns    291821068
BM_kostja        6.27 ns         6.27 ns    103323181
```
