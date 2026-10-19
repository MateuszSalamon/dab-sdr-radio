#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/reflection.h>

#include "ofdm_service.h"
#include "fic_service.h"
#include "basic_radio_service.h"
#include "grpc_adapter_interfaces.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    const std::string server_address = "0.0.0.0:50051";

    auto ofdm_bridge = std::make_shared<dab::grpc::NoOpOfdmBridge>();
    auto fic_bridge = std::make_shared<dab::grpc::NoOpFICBridge>();
    auto radio_bridge = std::make_shared<dab::grpc::NoOpBasicRadioBridge>();

    dab::grpc::OfdmServiceImpl ofdm_service(ofdm_bridge);
    dab::grpc::FicServiceImpl fic_service(fic_bridge);
    dab::grpc::BasicRadioServiceImpl radio_service(radio_bridge);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&ofdm_service);
    builder.RegisterService(&fic_service);
    builder.RegisterService(&radio_service);

    auto server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "Failed to start gRPC server on " << server_address << std::endl;
        return 1;
    }

    std::cout << "gRPC server listening on " << server_address << std::endl;
    server->Wait();
    return 0;
}
