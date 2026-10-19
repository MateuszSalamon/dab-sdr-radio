# DAB gRPC testing interfaces

This directory contains a parallel, non-invasive gRPC test surface for the existing DAB radio signal-processing blocks.

The goal is to provide remote-call access to the major processing units without changing the behavior of the original source tree. The live project remains unchanged unless the optional `DAB_ENABLE_GRPC` flag is enabled at configure time.

Structure:
- `proto/dab_grpc_api.proto` defines the service contracts.
- `src/grpc_adapter_interfaces.h` contains adapter bridges to the live source objects.
- `src/ofdm_service.h`, `src/fic_service.h`, and `src/basic_radio_service.h` expose one gRPC service per major processing block.
- `src/grpc_server_main.cpp` runs a lightweight gRPC test server.

Build:
  cmake -S . -B build -DDAB_ENABLE_GRPC=ON
  cmake --build build --target dab_grpc_server

The default build still remains unchanged because the option is OFF unless explicitly enabled.
