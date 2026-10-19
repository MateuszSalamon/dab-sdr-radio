#pragma once

#include <memory>
#include <vector>
#include <string>

#include "ofdm/ofdm_demodulator.h"
#include "dab/fic/fic_decoder.h"
#include "basic_radio/basic_radio.h"
#include "dab/constants/dab_parameters.h"

namespace dab::grpc {

struct IOfdmBridge {
    virtual ~IOfdmBridge() = default;
    virtual std::string GetStatus() const = 0;
    virtual bool ProcessBlock(const std::vector<float>& iq_samples, std::vector<int32_t>* output_bits) = 0;
    virtual float GetFineFrequencyOffset() const = 0;
    virtual float GetCoarseFrequencyOffset() const = 0;
    virtual int32_t GetTotalFramesRead() const = 0;
};

struct IFICBridge {
    virtual ~IFICBridge() = default;
    virtual bool DecodeFIBGroup(const std::vector<int32_t>& encoded_bits, int32_t cif_index, std::vector<int32_t>* decoded_bytes) = 0;
    virtual std::string GetStatus() const = 0;
};

struct IBasicRadioBridge {
    virtual ~IBasicRadioBridge() = default;
    virtual bool ProcessBits(const std::vector<int32_t>& bits) = 0;
    virtual bool HasAudioChannel(int32_t subchannel_id) const = 0;
    virtual bool HasDataPacketChannel(int32_t subchannel_id) const = 0;
    virtual std::string GetStatus() const = 0;
};

class NoOpOfdmBridge final : public IOfdmBridge {
public:
    std::string GetStatus() const override { return "not-configured"; }
    bool ProcessBlock(const std::vector<float>&, std::vector<int32_t>*) override { return false; }
    float GetFineFrequencyOffset() const override { return 0.0f; }
    float GetCoarseFrequencyOffset() const override { return 0.0f; }
    int32_t GetTotalFramesRead() const override { return 0; }
};

class NoOpFICBridge final : public IFICBridge {
public:
    bool DecodeFIBGroup(const std::vector<int32_t>&, int32_t, std::vector<int32_t>*) override { return false; }
    std::string GetStatus() const override { return "not-configured"; }
};

class NoOpBasicRadioBridge final : public IBasicRadioBridge {
public:
    bool ProcessBits(const std::vector<int32_t>&) override { return false; }
    bool HasAudioChannel(int32_t) const override { return false; }
    bool HasDataPacketChannel(int32_t) const override { return false; }
    std::string GetStatus() const override { return "not-configured"; }
};

class LiveOfdmBridge final : public IOfdmBridge {
public:
    explicit LiveOfdmBridge(std::shared_ptr<OFDM_Demod> impl) : impl_(std::move(impl)) {}

    std::string GetStatus() const override { return impl_ ? "live-ofdm" : "unbound"; }

    bool ProcessBlock(const std::vector<float>& iq_samples, std::vector<int32_t>* output_bits) override {
        if (!impl_ || !output_bits) return false;
        output_bits->clear();
        if (iq_samples.empty()) return false;

        std::vector<std::complex<float>> in_block;
        in_block.reserve(iq_samples.size() / 2);
        for (size_t i = 0; i + 1 < iq_samples.size(); i += 2) {
            in_block.emplace_back(iq_samples[i], iq_samples[i + 1]);
        }

        impl_->Process(tcb::span<const std::complex<float>>(in_block));
        const auto frame_bits = impl_->GetFrameDataBits();
        output_bits->reserve(frame_bits.size());
        for (const auto bit : frame_bits) {
            output_bits->push_back(static_cast<int32_t>(bit));
        }
        return true;
    }

    float GetFineFrequencyOffset() const override { return impl_ ? impl_->GetFineFrequencyOffset() : 0.0f; }
    float GetCoarseFrequencyOffset() const override { return impl_ ? impl_->GetCoarseFrequencyOffset() : 0.0f; }
    int32_t GetTotalFramesRead() const override { return impl_ ? static_cast<int32_t>(impl_->GetTotalFramesRead()) : 0; }

private:
    std::shared_ptr<OFDM_Demod> impl_;
};

class LiveFICBridge final : public IFICBridge {
public:
    explicit LiveFICBridge(std::shared_ptr<FIC_Decoder> impl) : impl_(std::move(impl)) {}

    bool DecodeFIBGroup(const std::vector<int32_t>& encoded_bits, int32_t cif_index, std::vector<int32_t>* decoded_bytes) override {
        if (!impl_ || !decoded_bytes) return false;

        std::vector<viterbi_bit_t> encoded;
        encoded.reserve(encoded_bits.size());
        for (const auto bit : encoded_bits) {
            encoded.push_back(static_cast<viterbi_bit_t>(bit));
        }

        impl_->DecodeFIBGroup(tcb::span<const viterbi_bit_t>(encoded), static_cast<size_t>(cif_index));
        decoded_bytes->clear();
        return true;
    }

    std::string GetStatus() const override { return impl_ ? "live-fic" : "unbound"; }

private:
    std::shared_ptr<FIC_Decoder> impl_;
};

class LiveBasicRadioBridge final : public IBasicRadioBridge {
public:
    explicit LiveBasicRadioBridge(std::shared_ptr<BasicRadio> impl) : impl_(std::move(impl)) {}

    bool ProcessBits(const std::vector<int32_t>& bits) override {
        if (!impl_ || bits.empty()) return false;

        std::vector<viterbi_bit_t> frame_bits;
        frame_bits.reserve(bits.size());
        for (const auto bit : bits) {
            frame_bits.push_back(static_cast<viterbi_bit_t>(bit));
        }

        impl_->Process(tcb::span<const viterbi_bit_t>(frame_bits));
        return true;
    }

    bool HasAudioChannel(int32_t subchannel_id) const override {
        if (!impl_) return false;
        return impl_->Get_Audio_Channel(static_cast<subchannel_id_t>(subchannel_id)) != nullptr;
    }

    bool HasDataPacketChannel(int32_t subchannel_id) const override {
        if (!impl_) return false;
        return impl_->Get_Data_Packet_Channel(static_cast<subchannel_id_t>(subchannel_id)) != nullptr;
    }

    std::string GetStatus() const override { return impl_ ? "live-basic-radio" : "unbound"; }

private:
    std::shared_ptr<BasicRadio> impl_;
};

} // namespace dab::grpc
