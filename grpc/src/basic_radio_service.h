#pragma once

#include <grpcpp/grpcpp.h>

#include "dab_grpc_api.pb.h"
#include "grpc_adapter_interfaces.h"

namespace dab::grpc {

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
