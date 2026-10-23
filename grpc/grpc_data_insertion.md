# gRPC data insertion and live runtime bridging

This document explains how data is injected into the live DAB processing pipeline through gRPC without altering the normal in-process execution flow.

## Goal

The project keeps the original runtime objects (`OFDM_Demod`, `FIC_Decoder`, `BasicRadio`) intact and exposes them through gRPC services. Remote clients send data to the server, the server converts it to native C++ types used by the processing code, and then calls the same live object methods that the application would normally call in-process.

This preserves the existing behavior and allows external testing while keeping the program flow unchanged.

## High-level flow

1. A client sends a protobuf request to the gRPC server.
2. The server deserializes the request into a `std::vector`.
3. A service implementation calls a bridge object.
4. The bridge converts the request data into the exact runtime types expected by the library.
5. The bridge invokes the live processing object (`Process`, `DecodeFIBGroup`, etc.).
6. The server returns the result payload and status.

## Protobuf contract

The public API is defined in:

- `grpc/proto/dab_grpc_api.proto`

The generated service interfaces define operations such as:

- `ProcessBlock`
- `GetStatus`
- `DecodeFIBGroup`
- `ProcessBits`
- `GetAudioChannel`
- `GetDataPacketChannel`

Each request carries the data to be processed, and the response returns status and the result payload.

## Service layer

The server-side service classes live in:

- `grpc/src/ofdm_service.h`
- `grpc/src/fic_service.h`
- `grpc/src/basic_radio_service.h`

Each service is implemented as a gRPC server class that inherits from the generated protobuf service base class.

Example pattern:

```cpp
grpc::Status ProcessBlock(::grpc::ServerContext* /*context*/,
    const ::dab::grpc::ProcessBlockRequest* request,
    ::dab::grpc::ProcessBlockResponse* response) override {
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
```

This is the actual insertion point at the network boundary: the request is converted to native C++ containers and passed to the runtime bridge.

## Bridge layer

The indirection layer lives in:

- `grpc/src/grpc_adapter_interfaces.h`

It defines interfaces such as:

- `IOfdmBridge`
- `IFICBridge`
- `IBasicRadioBridge`

The bridge pattern is important because it separates:

- network protocol concerns,
- runtime object concerns,
- and the original application logic.

The bridge API is intentionally small and stable:

```cpp
struct IOfdmBridge {
    virtual std::string GetStatus() const = 0;
    virtual bool ProcessBlock(const std::vector<float>& iq_samples, std::vector<int32_t>* output_bits) = 0;
    virtual float GetFineFrequencyOffset() const = 0;
    virtual float GetCoarseFrequencyOffset() const = 0;
    virtual int32_t GetTotalFramesRead() const = 0;
};
```

The default implementations are no-op adapters (`NoOpOfdmBridge`, `NoOpFICBridge`, `NoOpBasicRadioBridge`) so the server starts safely without a live runtime binding.

## Live runtime injection

The live adapter classes implement the bridge by calling the real processing objects:

- `LiveOfdmBridge`
- `LiveFICBridge`
- `LiveBasicRadioBridge`

These bridge classes accept real runtime instances, such as:

- `std::shared_ptr<OFDM_Demod>`
- `std::shared_ptr<FIC_Decoder>`
- `std::shared_ptr<BasicRadio>`

Example from the OFDM bridge:

```cpp
bool ProcessBlock(const std::vector<float>& iq_samples, std::vector<int32_t>* output_bits) override {
    if (!impl_ || !output_bits) return false;

    std::vector<std::complex<float>> in_block;
    for (size_t i = 0; i + 1 < iq_samples.size(); i += 2) {
        in_block.emplace_back(iq_samples[i], iq_samples[i + 1]);
    }

    impl_->Process(tcb::span<const std::complex<float>>(in_block));
    const auto frame_bits = impl_->GetFrameDataBits();
    for (const auto bit : frame_bits) {
        output_bits->push_back(static_cast<int32_t>(bit));
    }
    return true;
}
```

This is the key insertion mechanism: the data is shaped into the same `std::complex<float>` and `viterbi_bit_t` types the library already uses, then the original processing method is invoked directly.

## How the in-process program flow is preserved

The bridge does not replace the original implementation with a remote-only path. Instead it calls the same live logic used by the app:

- `OFDM_Demod::Process(...)`
- `FIC_Decoder::DecodeFIBGroup(...)`
- `BasicRadio::Process(...)`

The server is not reimplementing the algorithms. It is forwarding data into those existing objects.

As a result:

- the runtime algorithm stays unchanged,
- the original code path remains active,
- remote clients can trigger the same logic for testing,
- and the normal local flow is not broken.

## Runtime binding helper

The live objects are created in the helper file:

- `grpc/src/live_bindings.h`

This file creates the real runtime instances used by the server:

```cpp
inline std::shared_ptr<FIC_Decoder> CreateLiveFIC(const DAB_Parameters& params) {
    return std::make_shared<FIC_Decoder>(params.nb_fib_cif_bits, params.nb_fibs_per_cif);
}

inline std::shared_ptr<BasicRadio> CreateLiveBasicRadio(const DAB_Parameters& params, size_t nb_threads = 0) {
    return std::make_shared<BasicRadio>(params, nb_threads);
}
```

The main server then wires those runtime objects into the live bridges:

```cpp
const auto params = get_dab_parameters(1);
auto fic_bridge = std::make_shared<dab::grpc::LiveFICBridge>(dab::grpc::CreateLiveFIC(params));
auto radio_bridge = std::make_shared<dab::grpc::LiveBasicRadioBridge>(dab::grpc::CreateLiveBasicRadio(params, 2));
```

## Server startup

The server is started in:

- `grpc/src/grpc_server_main.cpp`

It creates a gRPC listener and registers the services. This is the component that exposes the injected runtime to remote callers.

## Summary

The gRPC insertion mechanism is realized as a layered bridge:

- protobuf request arrives at the gRPC service,
- request data is converted into native vectors,
- the service invokes a bridge implementation,
- the bridge converts the payload to the project’s native types,
- the bridge calls the same live C++ processing object already used by the application,
- the result is returned to the client.

This gives remote test access without replacing or breaking the existing application flow.
