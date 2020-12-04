// Code based on benchmark by Daniel Lemire
// and ScyllaDB implementations of Cassandra's timeuuid
// by Avi (original Cassandra port from Java)
// and Kostja (refactored Cassandra fix)
//
#include <cassert>
#include <chrono>
#include <cfloat>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <string_view>

const size_t repeat = 100000;

using bytes_view = std::basic_string_view<int8_t>;

using test_fn = std::function<int(bytes_view, bytes_view)>;

int expected;

int timeuuid_compare_bytes_noop(bytes_view o1, bytes_view o2) {
    return expected;
}

inline int timeuuid_compare_bytes_ori(bytes_view o1, bytes_view o2) {
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

inline int timeuuid_compare_bytes_kostja(bytes_view o1, bytes_view o2) {
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
};

// 4 bytes    2 bytes    2 bytes               2 bytes                              6 bytes
// time_low - time_mid - time_hi          - clock_seq_hi_and_res+clock_seq_low - node
//                         and                  and
//                       version
//              tlow     tmid  thi/v cseq/node
int8_t x1[] = {0,0,0,0,  0,0,   0,0,  0,0,0,0,0,0,0,0};
int8_t x2[] = {1,0,0,1,  0,0,   0,0,  0,0,0,0,0,0,0,0};
bytes_view tuuid1(x1, std::size(x1));
bytes_view tuuid2(x2, std::size(x2));

struct t_result time_it_ns(test_fn function, size_t repeat, int expect) {
    std::chrono::high_resolution_clock::time_point t1, t2;
    double average = 0;
    double min_value = DBL_MAX;
    for (size_t i = 0; i < repeat; i++) {
        t1 = std::chrono::high_resolution_clock::now();
        int tcmp = function(tuuid1, tuuid2);
        t2 = std::chrono::high_resolution_clock::now();
        assert(tcmp == expect); // check return is valid and avoid optimizing away
        double dif = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
        average += dif;
        min_value = min_value < dif ? min_value : dif;
    }
    average /= repeat;
    return t_result{min_value, average};
}

void process(int repeat) {
  auto pretty_print = [](std::string name,
                                 t_result result) {
    printf("%-23s: min %3.2f  average %3.2f\n", name.data(), result.min, result.average);
  };
  expected = timeuuid_compare_bytes_ori(tuuid1, tuuid2);
  pretty_print("ori (8 bytes)", time_it_ns(timeuuid_compare_bytes_ori, repeat, expected));
  pretty_print("noop", time_it_ns(timeuuid_compare_bytes_noop, repeat, expected));
  pretty_print("kostja's fix (16 bytes)", time_it_ns(timeuuid_compare_bytes_kostja, repeat, expected));
}

int main(int argc, char **argv) {
    process(repeat);
}
