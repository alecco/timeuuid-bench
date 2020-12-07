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
#include <tmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>

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

inline int compare_ori(bytes_view o1, bytes_view o2) {
    auto compare_pos = [&] (unsigned pos, int mask, int ifequal) {
        int d = (o1[pos] & mask) - (o2[pos] & mask);
        return d ? d : ifequal;
    };  
    return compare_pos(6, 0xf,
        compare_pos(7, 0xff,
            compare_pos(4, 0xff,
                compare_pos(5, 0xff,
                    compare_pos(0, 0xff,
                        compare_pos(1, 0xff,
                            compare_pos(2, 0xff,
                                compare_pos(3, 0xff, 0))))))));
}

inline int compare_kostja(bytes_view o1, bytes_view o2) {
    // Scylla and Cassandra use a standard UUID memory layout:
    // 4 bytes    2 bytes    2 bytes               2 bytes                              6 bytes
    // time_low - time_mid - time_hi_and_version - clock_seq_hi_and_res+clock_seq_low - node
    //
    // Reorder bytes to allow for quick integer compare.
    auto read_msb = [](bytes_view o) -> uint64_t {
        auto msb = read_be<uint64_t>(o.begin());
        auto ret = ((msb & 0x0FFF) << 48) | ((msb & 0xFFFF0000) << 32) | (msb >> 32);
        return ret;
    };
    auto read_lsb = [](bytes_view o) -> uint64_t {
        uint64_t lsb = read_be<uint64_t>(o.begin() + sizeof(uint64_t));
        // Match the order of int8 compare.
        return lsb ^ 0x8080808080808080;
    };
    auto tri_compare_uint64_t = [](uint64_t a, uint64_t b) -> int {
        auto ret = a < b ? -1 : a != b;
        return ret;
    };
    auto res = tri_compare_uint64_t(read_msb(o1), read_msb(o2));
    if (res == 0) {
        res = tri_compare_uint64_t(read_lsb(o1), read_lsb(o2));
    }
    return res;
}

inline int compare_vec(bytes_view o1, bytes_view o2) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i d1 = *reinterpret_cast<const __m128i*>(o1.begin());
    const __m128i d2 = *reinterpret_cast<const __m128i*>(o2.begin());
    // XXX find actual shuffle indexes
    const __m128i shuf = _mm_setr_epi8(0, 1, 2, 3, 11, 5, 15, 7, 8, 9, 10, 4, 12, 13, 14, 6);
    const __m128i d1s = _mm_shuffle_epi8(d1, shuf);
    const __m128i d2s = _mm_shuffle_epi8(d2, shuf);

    const __m128i sub = _mm_sub_epi64(d1s, d2s);  // differences high ; low
    // sub     diff hi   |    diff lo
    //         if nonzero    if nonzero
    // XXX continue here
    // const __m128i iszm = _mm_cmpeq_epi64(zero, sub);
    const int32_t msk8 = _mm_movemask_epi8(sub); // XXX
    return (msk8 >> 16 & msk8) + 1234;  // XXX extract 2 bits: sign and bit 0

    // now check diff higher non zero, then lower
    // ret -1 0 1

    // (do not cross whole ints to ALU)
    // operate in SSE then extract mask
    // other
    // const __m128 x = _mm_movehl_ps (sub,
    // __m128i dst = _mm_unpackhi_epi64 (__m128i a, __m128i b);  // dst:  b_hi:a_hi
    // uint64_t sub_hi = _mm_extract_epi64(sub, 1);
    // uint64_t sub_lo = _mm_cvtsi128_si64(sub);
    // const int32_t islt = _mm_movemask_epi8(_mm_cmpgt_epi64(d2s, d1s)); // XXX inverted GT?
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
static void BM_vec(benchmark::State& state) {
    dummy = 0;
    for (auto _ : state) {
        reset_x1();
        dummy += compare_vec(tuuid1, tuuid2);
    }
}

// Register the function as a benchmark
BENCHMARK(BM_trivial);
BENCHMARK(BM_ori);
BENCHMARK(BM_kostja);
BENCHMARK(BM_vec);
// Run the benchmark
BENCHMARK_MAIN();
