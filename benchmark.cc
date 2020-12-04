// Code based on benchmark by Daniel Lemire
// and ScyllaDB implementations of Cassandra's timeuuid
// by Avi (original Cassandra port from Java)
// and Kostja (refactored Cassandra fix)
//
#include <cassert>
#include <chrono>
#include <cfloat>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string_view>

size_t repeat  = 10000;

using bytes_view = std::basic_string_view<int8_t>;

// Original method name timeuuid_compare_bytes()

inline int compare_trivial(bytes_view o1, bytes_view o2) {
    return *reinterpret_cast<const int8_t*>(o1.begin());
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
        auto msb = *reinterpret_cast<const uint64_t*>(o.begin());
        auto ret = ((msb & 0x0FFF) << 48) | ((msb & 0xFFFF0000) << 32) | (msb >> 32);
        return ret;
    };
    auto read_lsb = [](bytes_view o) -> uint64_t {
        uint64_t lsb = *reinterpret_cast<const uint64_t*>(o.begin() + sizeof(uint64_t));
        // Match the order of int8 compare.
        return lsb ^ 0x8080808080808080;
    };
    auto tri_compare_uint64_t = [](uint64_t a, uint64_t b) -> int {
        auto ret = a < b ? -1 : a != b;
        return ret;
    };
    auto res = tri_compare_uint64_t(read_msb(o1), read_msb(o2));
    if (res != 0) {
        return res;
    }
    return tri_compare_uint64_t(read_lsb(o1), read_lsb(o2));
}

struct t_result {
    double min;
    double average;
    uint64_t cksum;
};

struct t_result time_it_ns(int (*function)(bytes_view, bytes_view), int base, size_t repeat) {
    int8_t x1[16] = {0,};
    int8_t x2[16] = {0,};
    bytes_view tuuid1(x1, std::size(x1));
    bytes_view tuuid2(x2, std::size(x2));
    uint32_t sum1 = 0, sum2 = 0;

    // NOTE: To minimize measurement error, run a case 100x longer than base timer cost
    //       If base noop is 13 ns, run at least a 1300 times
    size_t case_loop = base * 100;

    std::chrono::high_resolution_clock::time_point t1, t2;
    double average = 0;
    double min_value = DBL_MAX;
    for (size_t i = 0; i < repeat; i++) {
        t1 = std::chrono::high_resolution_clock::now();    // Start

        for (size_t j = 0; j < case_loop; j++) {
            x1[j % 6] = x2[(6 - (j % 6))] = 1; // flip a bit
            int res = function(tuuid1, tuuid2);

            sum1 = (sum1 + res)  % 0xFFFF;
            sum2 = (sum2 + sum1) % 0xFFFF;
            x1[j % 6] = x2[(6 - (j % 6))] = 0; // unflip the bit
        }

        t2 = std::chrono::high_resolution_clock::now();     // End
        double dif = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
        average += dif;
        min_value = min_value < dif ? min_value : dif;
    }
    average /= repeat;
    return t_result{min_value/case_loop, average/case_loop, sum2 << 16 | sum1};
}
struct t_result time_noop_ns(size_t repeat) {
    std::chrono::high_resolution_clock::time_point t1, t2;
    double average = 0;
    double min_value = DBL_MAX;
    for (size_t i = 0; i < repeat; i++) {
        t1 = std::chrono::high_resolution_clock::now();
        t2 = std::chrono::high_resolution_clock::now();
        double dif = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
        average += dif;
        min_value = min_value < dif ? min_value : dif;
    }
    average /= repeat;
    return t_result{min_value, average};
}

void process(int repeat) {
    auto pretty_print = [](std::string name, t_result result) {
        printf("%-23s: min %3.2f  average %3.2f (cksum %d)\n", name.data(), result.min, result.average, result.cksum);
    };
    int base_ns = static_cast<int>(time_noop_ns(repeat).min);
    printf("base_ns chrono %d\n", base_ns);
    pretty_print("trivial function",          time_it_ns(compare_trivial, base_ns, repeat));
    pretty_print("original broken (8 bytes)", time_it_ns(compare_ori,     base_ns, repeat));
    pretty_print("fixed (full 16 bytes)",     time_it_ns(compare_kostja,  base_ns, repeat));
}

int main(int argc, char **argv) {
    if (argc == 2) {
        repeat = std::atoll(argv[1]);
    }
    process(repeat);
}
