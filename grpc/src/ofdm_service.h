#pragma once

#include <grpcpp/grpcpp.h>

#include "dab_grpc_api.pb.h"
#include "grpc_adapter_interfaces.h"

namespace dab::grpc {

class OfdmServiceImpl final : public ::dab::grpc::OFDMService::Service {
public:
    explicit OfdmServiceImpl(std::shared_ptr<IOfdmBridge> bridge = std::make_shared<NoOpOfdmBridge>())
        : bridge_(std::move(bridge)) {}

    grpc::Status ProcessBlock(::grpc::ServerContext* /*context*/, const ::dab::grpc::ProcessBlockRequest* request, ::dab::grpc::ProcessBlockResponse* response) override {
        if (!request || !response) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid request");
        }

        std::vector<float> iq_samples(request->iq_samples().begin(), request->iq_samples().end());
        std::vector<int32_t> output_bits;
        const bool ok = bridge_->ProcessBlock(iq_samples, &output_bits);
        response->set_ok(ok);
        response->set_status(bridge_->GetStatus());
        for (const auto bit : output_bits) {
            response->add_bits(bit);
        }
        return grpc::Status::OK;
    }

    grpc::Status GetStatus(::grpc::ServerContext* /*context*/, const ::dab::grpc::GetStatusRequest* /*request*/, ::dab::grpc::GetStatusResponse* response) override {
        if (!response) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid response");
        }
        response->set_state(bridge_->GetStatus());
        response->set_fine_frequency_offset(bridge_->GetFineFrequencyOffset());
        response->set_coarse_frequency_offset(bridge_->GetCoarseFrequencyOffset());
        response->set_total_frames_read(bridge_->GetTotalFramesRead());
        return grpc::Status::OK;
    }

private:
    std::shared_ptr<IOfdmBridge> bridge_;
};

class FicServiceImpl final : public ::dab::grpc::FICService::Service {
public:
    explicit FicServiceImpl(std::shared_ptr<IFICBridge> bridge = std::make_shared<NoOpFICBridge>())
        : bridge_(std::move(bridge)) {}

    grpc::Status DecodeFIBGroup(::grpc::ServerContext* /*context*/, const ::dab::grpc::DecodeFIBGroupRequest* request, ::dab::grpc::DecodeFIBGroupResponse* response) override {
        if (!request || !response) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid request");
        }

        std::vector<int32_t> encoded_bits(request->encoded_bits().begin(), request->encoded_bits().end());
        std::vector<int32_t> decoded_bytes;
        const bool ok = bridge_->DecodeFIBGroup(encoded_bits, request->cif_index(), &decoded_bytes);
        response->set_ok(ok);
        response->set_status(bridge_->GetStatus());
        for (const auto byte : decoded_bytes) {
            response->add_decoded_bytes(byte);
        }
        return grpc::Status::OK;
    }

private:
    std::shared_ptr<IFICBridge> bridge_;
};

class BasicRadioServiceImpl final : public ::dab::grpc::BasicRadioService::Service {
public:
    explicit BasicRadioServiceImpl(std::shared_ptr<IBasicRadioBridge> bridge = std::make_shared<NoOpBasicRadioBridge>())
        : bridge_(std::move(bridge)) {}

    grpc::Status ProcessBits(::grpc::ServerContext* /*context*/, const ::dab::grpc::ProcessBitsRequest* request, ::dab::grpc::ProcessBitsResponse* response) override {
        if (!request || !response) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid request");
        }

        std::vector<int32_t> bits(request->encoded_bits().begin(), request->encoded_bits().end());
        response->set_ok(bridge_->ProcessBits(bits));
        response->set_status(bridge_->GetStatus());
        return grpc::Status::OK;
    }

    grpc::Status GetAudioChannel(::grpc::ServerContext* /*context*/, const ::dab::grpc::AudioChannelRequest* request, ::dab::grpc::AudioChannelResponse* response) override {
        if (!request || !response) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid request");
        }

        const bool found = bridge_->HasAudioChannel(request->subchannel_id());
        response->set_found(found);
        response->set_status(bridge_->GetStatus());
        return grpc::Status::OK;
    }

    grpc::Status GetDataPacketChannel(::grpc::ServerContext* /*context*/, const ::dab::grpc::AudioChannelRequest* request, ::dab::grpc::AudioChannelResponse* response) override {
        if (!request || !response) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid request");
        }

        const bool found = bridge_->HasDataPacketChannel(request->subchannel_id());
        response->set_found(found);
        response->set_status(bridge_->GetStatus());
        return grpc::Status::OK;
    }

private:
    std::shared_ptr<IBasicRadioBridge> bridge_;
};

} // namespace dab::grpc
