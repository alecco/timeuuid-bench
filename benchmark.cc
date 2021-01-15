// Code based on benchmark by Daniel Lemire
// and ScyllaDB implementations of Cassandra's timeuuid
// by Avi (original Cassandra port from Java)
// and Kostja (refactored Cassandra fix)
//
#include <benchmark/benchmark.h>   // Google benchmark
#include <cassert>
#include <chrono>
#include <cfloat>
#include <cstdlib>
#include <endian.h>
#include <iostream>
#include <stdio.h>
#include <string_view>

using bytes_view = std::basic_string_view<int8_t>;

inline uint64_t be_to_cpu(uint64_t x) noexcept { return be64toh(x); }

template <typename T>
inline
T
read_be(const signed char* p) noexcept {
    T datum = 0;
    std::copy_n(p, sizeof(T), reinterpret_cast<char*>(&datum));
    return be_to_cpu(datum);
}

// Original method name timeuuid_compare_bytes()

inline int compare_trivial(bytes_view o1, bytes_view o2) {
    return o1[0] & o2[0];
}

enum class lexicographical_relation : int8_t {
    before_all_prefixed,
    before_all_strictly_prefixed,
    after_all_prefixed
};

template <typename InputIt1, typename InputIt2, typename Compare>
int lexicographical_tri_compare(InputIt1 first1, InputIt1 last1,
    InputIt2 first2, InputIt2 last2,
    Compare comp,
    lexicographical_relation relation1 = lexicographical_relation::before_all_strictly_prefixed,
    lexicographical_relation relation2 = lexicographical_relation::before_all_strictly_prefixed) {
    while (first1 != last1 && first2 != last2) {
        auto c = comp(*first1, *first2);
        if (c) {
            return c;
        }
        ++first1;
        ++first2;
    }
    bool e1 = first1 == last1;
    bool e2 = first2 == last2;
    if (e1 == e2) {
        return static_cast<int>(relation1) - static_cast<int>(relation2);
    }
    if (e2) {
        return relation2 == lexicographical_relation::after_all_prefixed ? -1 : 1;
    } else {
        return relation1 == lexicographical_relation::after_all_prefixed ? 1 : -1;
    }
}

inline int compare_ori(bytes_view o1, bytes_view o2) {
    auto compare_pos = [&] (unsigned pos, int mask, int ifequal) {
        int d = (o1[pos] & mask) - (o2[pos] & mask);
        return d ? d : ifequal;
    };
    int res = compare_pos(6, 0xf,
        compare_pos(7, 0xff,
            compare_pos(4, 0xff,
                compare_pos(5, 0xff,
                    compare_pos(0, 0xff,
                        compare_pos(1, 0xff,
                            compare_pos(2, 0xff,
                                compare_pos(3, 0xff, 0))))))));
    if (res != 0) {
        return res;
    }
    return lexicographical_tri_compare(
        o1.begin(), o1.end(), o2.begin(), o2.end(), [] (const int8_t& a, const int8_t& b) { return a - b; });
}

// Read 8 most significant bytes of timeuuid from serialized bytes
inline uint64_t read_msb(bytes_view b) {
    // cast to unsigned to avoid sign-compliment during shift.
    auto u64 = [](uint8_t i) -> uint64_t { return i; };
    // Scylla and Cassandra use a standard UUID memory layout for MSB:
    // 4 bytes    2 bytes    2 bytes
    // time_low - time_mid - time_hi_and_version
    //
    // The storage format uses network byte order.
    // Reorder bytes to allow for an integer compare.
    return u64(b[6] & 0xf) << 56 | u64(b[7]) << 48 |
           u64(b[4]) << 40 | u64(b[5]) << 32 |
           u64(b[0]) << 24 | u64(b[1]) << 16 |
           u64(b[2]) << 8  | u64(b[3]);
}

inline uint64_t read_lsb(bytes_view b) {
    auto u64 = [](uint8_t i) -> uint64_t { return i; };
    return (u64(b[8]) << 56 | u64(b[9]) << 48 |
           u64(b[10]) << 40 | u64(b[11]) << 32 |
           u64(b[12]) << 24 | u64(b[13]) << 16 |
           u64(b[14]) << 8  | u64(b[15])) ^ 0x8080808080808080;
}

inline int compare_kostja(bytes_view o1, bytes_view o2) {
    auto tri_compare_uint64_t = [](uint64_t a, uint64_t b) -> int {
        return a < b ? -1 : a != b;
    };
    auto res = tri_compare_uint64_t(read_msb(o1), read_msb(o2));
    if (res == 0) {
        res = tri_compare_uint64_t(read_lsb(o1), read_lsb(o2));
    }
    return res;
}


// Global setup
int8_t x1[16] = {0,};
int8_t x2[16] = {0,};
bytes_view tuuid1(x1, std::size(x1));
bytes_view tuuid2(x2, std::size(x2));
int dummy;

inline void reset_x1() {
    // sometimes is the same to force branch mispredictions
    if (!(dummy & 1)) {
        std::fill(x1, x1 + 16, dummy);
    }
}

static void BM_trivial(benchmark::State& state) {
    dummy = 0;
    for (auto _ : state) {
        reset_x1();
        dummy += compare_trivial(tuuid1, tuuid2);
    }
}
static void BM_ori(benchmark::State& state) {
    dummy = 0;
    for (auto _ : state) {
        reset_x1();
        dummy += compare_ori(tuuid1, tuuid2);
    }
}
static void BM_kostja(benchmark::State& state) {
    dummy = 0;
    for (auto _ : state) {
        reset_x1();
        dummy += compare_kostja(tuuid1, tuuid2);
    }
}

// Register the function as a benchmark
BENCHMARK(BM_trivial);
BENCHMARK(BM_ori);
BENCHMARK(BM_kostja);
// Run the benchmark
BENCHMARK_MAIN();
