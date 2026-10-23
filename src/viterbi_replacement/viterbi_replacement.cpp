#include "viterbi_replacement.h"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

namespace dab::replacement {
namespace {

constexpr int16_t kSoftDecisionLow = -127;
constexpr int16_t kSoftDecisionHigh = 127;
constexpr int16_t kSoftDecisionPunctured = 0;

inline int16_t branch_metric_for_value(int16_t value, int16_t expected_value) {
    const int16_t diff = value - expected_value;
    return static_cast<int16_t>(std::abs(static_cast<int>(diff)));
}

} // namespace

ViterbiReplacementDecoder::ViterbiReplacementDecoder(std::size_t traceback_length)
    : traceback_length_(traceback_length == 0 ? 96u : traceback_length) {}

void ViterbiReplacementDecoder::set_traceback_length(std::size_t traceback_length) {
    traceback_length_ = traceback_length == 0 ? 96u : traceback_length;
}

std::size_t ViterbiReplacementDecoder::traceback_length() const noexcept {
    return traceback_length_;
}

std::vector<uint8_t> ViterbiReplacementDecoder::decode_bits(
    const std::vector<uint8_t>& hard_bits,
    const std::vector<uint8_t>& puncture_code,
    std::size_t requested_output_bits) const {
    const auto depunctured = depuncture_bits(hard_bits, puncture_code, requested_output_bits);
    std::vector<int16_t> soft_values;
    soft_values.reserve(depunctured.size());
    for (const auto bit : depunctured) {
        soft_values.push_back(bit == 0u ? kSoftDecisionLow : kSoftDecisionHigh);
    }
    return decode_soft(soft_values, {}, requested_output_bits);
}

std::vector<uint8_t> ViterbiReplacementDecoder::decode_soft(
    const std::vector<int16_t>& soft_symbols,
    const std::vector<uint8_t>& puncture_code,
    std::size_t requested_output_bits) const {
    const auto depunctured = depuncture_soft(soft_symbols, puncture_code, requested_output_bits);
    const std::size_t total_symbol_steps = depunctured.size() / kCodeRate;
    if (total_symbol_steps == 0u) {
        return {};
    }

    const std::size_t state_count = kStateCount;
    std::vector<int32_t> path_metrics(state_count, std::numeric_limits<int32_t>::max() / 4);
    std::vector<uint32_t> previous_state(total_symbol_steps * state_count, 0u);
    std::vector<uint8_t> previous_input(total_symbol_steps * state_count, 0u);

    path_metrics[0] = 0;

    for (std::size_t step = 0; step < total_symbol_steps; ++step) {
        const std::size_t step_offset = step * state_count;
        std::vector<int32_t> next_metrics(state_count, std::numeric_limits<int32_t>::max() / 4);
        std::vector<uint32_t> next_previous_state(state_count, 0u);
        std::vector<uint8_t> next_previous_input(state_count, 0u);

        for (uint32_t state = 0; state < state_count; ++state) {
            const int32_t current_metric = path_metrics[state];
            if (current_metric == std::numeric_limits<int32_t>::max() / 4) {
                continue;
            }

            for (uint8_t input_bit = 0; input_bit <= 1; ++input_bit) {
                const uint32_t next_state_value = next_state(state, input_bit);
                uint8_t encoded[4] = {0u, 0u, 0u, 0u};
                for (std::size_t i = 0; i < kCodeRate; ++i) {
                    encoded[i] = generator_output(state, input_bit, kPolynomials[i]);
                }

                int32_t branch_metric = 0;
                for (std::size_t i = 0; i < kCodeRate; ++i) {
                    const std::size_t symbol_index = step * kCodeRate + i;
                    const int16_t expected_value = encoded[i] == 0u ? kSoftDecisionLow : kSoftDecisionHigh;
                    const int16_t soft_value = depunctured[symbol_index];
                    branch_metric += branch_metric_for_value(soft_value, expected_value);
                }

                const int32_t candidate_metric = current_metric + branch_metric;
                if (candidate_metric < next_metrics[next_state_value]) {
                    next_metrics[next_state_value] = candidate_metric;
                    next_previous_state[next_state_value] = state;
                    next_previous_input[next_state_value] = input_bit;
                }
            }
        }

        path_metrics = std::move(next_metrics);
        for (std::size_t state = 0; state < state_count; ++state) {
            previous_state[step_offset + state] = next_previous_state[state];
            previous_input[step_offset + state] = next_previous_input[state];
        }
    }

    uint32_t best_state = 0u;
    int32_t best_metric = path_metrics[0];
    for (uint32_t state = 1; state < state_count; ++state) {
        if (path_metrics[state] < best_metric) {
            best_metric = path_metrics[state];
            best_state = state;
        }
    }

    std::vector<uint8_t> decoded_bits;
    decoded_bits.reserve(total_symbol_steps);
    for (std::size_t step = total_symbol_steps; step > 0u; --step) {
        const std::size_t step_index = (step - 1u) * state_count;
        const uint8_t bit = previous_input[step_index + best_state];
        decoded_bits.push_back(bit);
        best_state = previous_state[step_index + best_state];
    }

    std::reverse(decoded_bits.begin(), decoded_bits.end());
    return bits_to_bytes(decoded_bits);
}

std::vector<int16_t> ViterbiReplacementDecoder::depuncture_soft(
    const std::vector<int16_t>& soft_symbols,
    const std::vector<uint8_t>& puncture_code,
    std::size_t requested_output_bits) {
    if (puncture_code.empty()) {
        if (requested_output_bits == 0u || requested_output_bits >= soft_symbols.size()) {
            return soft_symbols;
        }
        return std::vector<int16_t>(soft_symbols.begin(), soft_symbols.begin() + static_cast<std::ptrdiff_t>(requested_output_bits));
    }

    std::vector<int16_t> depunctured;
    depunctured.reserve(std::max<std::size_t>(soft_symbols.size(), requested_output_bits));

    std::size_t input_index = 0u;
    std::size_t puncture_index = 0u;
    while (input_index < soft_symbols.size()) {
        const std::size_t block_length = static_cast<std::size_t>(puncture_code[puncture_index % puncture_code.size()]);
        const std::size_t unpunctured_length = kCodeRate - block_length;

        for (std::size_t i = 0; i < block_length && input_index < soft_symbols.size(); ++i) {
            depunctured.push_back(soft_symbols[input_index++]);
        }

        for (std::size_t i = 0; i < unpunctured_length; ++i) {
            depunctured.push_back(kSoftDecisionPunctured);
        }

        ++puncture_index;
        if (requested_output_bits != 0u && depunctured.size() >= requested_output_bits) {
            depunctured.resize(requested_output_bits);
            break;
        }
    }

    return depunctured;
}

std::vector<uint8_t> ViterbiReplacementDecoder::depuncture_bits(
    const std::vector<uint8_t>& hard_bits,
    const std::vector<uint8_t>& puncture_code,
    std::size_t requested_output_bits) {
    if (puncture_code.empty()) {
        if (requested_output_bits == 0u || requested_output_bits >= hard_bits.size()) {
            return hard_bits;
        }
        return std::vector<uint8_t>(hard_bits.begin(), hard_bits.begin() + static_cast<std::ptrdiff_t>(requested_output_bits));
    }

    std::vector<uint8_t> depunctured;
    depunctured.reserve(std::max<std::size_t>(hard_bits.size(), requested_output_bits));
    std::size_t input_index = 0u;
    std::size_t puncture_index = 0u;

    while (input_index < hard_bits.size()) {
        const std::size_t block_length = static_cast<std::size_t>(puncture_code[puncture_index % puncture_code.size()]);
        const std::size_t unpunctured_length = kCodeRate - block_length;

        for (std::size_t i = 0; i < block_length && input_index < hard_bits.size(); ++i) {
            depunctured.push_back(hard_bits[input_index++]);
        }

        for (std::size_t i = 0; i < unpunctured_length; ++i) {
            depunctured.push_back(0u);
        }

        ++puncture_index;
        if (requested_output_bits != 0u && depunctured.size() >= requested_output_bits) {
            depunctured.resize(requested_output_bits);
            break;
        }
    }

    return depunctured;
}

uint32_t ViterbiReplacementDecoder::next_state(uint32_t state, uint8_t input_bit) {
    const uint32_t mask = static_cast<uint32_t>(kStateCount) - 1u;
    return ((state << 1u) & mask) | static_cast<uint32_t>(input_bit);
}

uint8_t ViterbiReplacementDecoder::generator_output(uint32_t state, uint8_t input_bit, uint8_t generator) {
    uint32_t reg = state;
    uint32_t output = 0u;
    for (std::size_t bit_index = 0; bit_index < kConstraintLength - 1u; ++bit_index) {
        const uint32_t tap = (generator >> bit_index) & 1u;
        if (tap != 0u) {
            output ^= (reg >> bit_index) & 1u;
        }
    }
    output ^= input_bit;
    return static_cast<uint8_t>(output & 1u);
}

uint8_t ViterbiReplacementDecoder::pack_bits_to_byte(const std::vector<uint8_t>& bits, std::size_t index) {
    uint8_t byte = 0u;
    for (std::size_t i = 0; i < 8u; ++i) {
        if (index + i < bits.size()) {
            byte |= static_cast<uint8_t>(bits[index + i] << (7u - i));
        }
    }
    return byte;
}

std::vector<uint8_t> ViterbiReplacementDecoder::bits_to_bytes(const std::vector<uint8_t>& decoded_bits) {
    std::vector<uint8_t> output;
    if (decoded_bits.empty()) {
        return output;
    }

    const std::size_t total_bytes = (decoded_bits.size() + 7u) / 8u;
    output.reserve(total_bytes);
    for (std::size_t i = 0; i < total_bytes; ++i) {
        output.push_back(pack_bits_to_byte(decoded_bits, i * 8u));
    }
    return output;
}

} // namespace dab::replacement
