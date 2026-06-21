#include "simd.h"
#include <cstring> // Für std::memcpy

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "simd.cc"
#include <hwy/foreach_target.h>

#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();
namespace simd {
    namespace HWY_NAMESPACE {
        namespace hn = hwy::HWY_NAMESPACE;

        // Templated Helper für maximale Compiler-Optimierung der inneren Schleifen
        template <int axis>
        void Processgrid(const int* data, std::vector<uint64_t>& chunkrow) {
            const hn::ScalableTag<int32_t> d;
            auto v_zero = hn::Zero(d);

                // Bei Achse 0 ist 'x' die Bit-Position. 
                // Das ist der Idealfall: Wir können die Bits direkt schieben und verodern.
                for (int z = 0; z < 64; ++z) {
                    for (int y = 0; y < 64; ++y) {
                        uint64_t row_val = 0;

                        for (size_t x = 0; x < 64; x += hn::Lanes(d)) {
                            int u = x + y * 64 + z * 64 * 64;
                            auto v = hn::Load(d, &data[u]);
                            auto mask = hn::Ne(v, v_zero);

                            // Highway packt Lanes in Bits. 8 Bytes (=64 bit) reichen für jede aktuelle Architektur aus.
                            uint8_t mask_bytes[8] = { 0 };
                            hn::StoreMaskBits(d, mask, mask_bytes);

                            uint64_t lane_bits = 0;
                            std::memcpy(&lane_bits, mask_bytes, 8); // Kopiere gepackte Bits in einen 64-Bit Integer

                            // Die gepackten Bits an die richtige x-Position schieben und sammeln
                            row_val |= (lane_bits << x);
                        }
                        // NACH der x-Schleife gesammelt in den Speicher schreiben (sehr schnell)
                        chunkrow[z * 64 + y] |= row_val;
                    }
                }
           
        }

        // Der Wrapper, der zur Laufzeit basierend auf `axis` das richtige Template instanziiert
        void Implementation(const int* data, std::vector<uint64_t>* chunkrow, int axis) {
            if (axis == 0) Processgrid<0>(data, *chunkrow);
            else if (axis == 1) Processgrid<1>(data, *chunkrow);
            else if (axis == 2) Processgrid<2>(data, *chunkrow);
        }

    } // namespace HWY_NAMESPACE
} // namespace simd
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace simd {
    HWY_EXPORT(Implementation);

    void ComputechunkSimd(const int* data, std::vector<uint64_t>* chunkrow, int axis) {
        // HWY_DYNAMIC_DISPATCH springt automatisch in die schnellste, vom Prozessor unterstütze Version
        HWY_DYNAMIC_DISPATCH(Implementation)(data, chunkrow, axis);
    }
} // namespace simd
#endif


