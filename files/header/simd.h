#ifndef SIMD_H_
#define SIMD_H_

#include <stdint.h>
#include <vector>
#include <iostream>

namespace simd {
    void ComputechunkSimd(const int* data, std::vector<uint64_t>* chunkrow, int axis);
}

#endif // SIMD_H_