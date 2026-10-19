#pragma once

#include <grpcpp/grpcpp.h>

#include "dab_grpc_api.pb.h"
#include "grpc_adapter_interfaces.h"

namespace dab::grpc {

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

} // namespace dab::grpc
