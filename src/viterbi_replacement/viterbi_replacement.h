#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dab::replacement {

constexpr std::size_t kConstraintLength = 7;
constexpr std::size_t kCodeRate = 4;

class ViterbiReplacementDecoder {
public:
    explicit ViterbiReplacementDecoder(std::size_t traceback_length = 96);

    void set_traceback_length(std::size_t traceback_length);
    std::size_t traceback_length() const noexcept;

    std::vector<uint8_t> decode_soft(
        const std::vector<int16_t>& soft_symbols,
        const std::vector<uint8_t>& puncture_code = {},
        std::size_t requested_output_bits = 0) const;

    std::vector<uint8_t> decode_bits(
        const std::vector<uint8_t>& hard_bits,
        const std::vector<uint8_t>& puncture_code = {},
        std::size_t requested_output_bits = 0) const;

private:
    static constexpr std::size_t kStateCount = 1u << (kConstraintLength - 1u);
    static constexpr std::array<uint8_t, 4> kPolynomials = {109u, 79u, 83u, 109u};

    std::size_t traceback_length_;

    static std::vector<int16_t> depuncture_soft(
        const std::vector<int16_t>& soft_symbols,
        const std::vector<uint8_t>& puncture_code,
        std::size_t requested_output_bits);

    static std::vector<uint8_t> depuncture_bits(
        const std::vector<uint8_t>& hard_bits,
        const std::vector<uint8_t>& puncture_code,
        std::size_t requested_output_bits);

    static uint32_t next_state(uint32_t state, uint8_t input_bit);
    static uint8_t generator_output(uint32_t state, uint8_t input_bit, uint8_t generator);
    static uint8_t pack_bits_to_byte(const std::vector<uint8_t>& bits, std::size_t index);
    static std::vector<uint8_t> bits_to_bytes(const std::vector<uint8_t>& decoded_bits);
};

} // namespace dab::replacement
