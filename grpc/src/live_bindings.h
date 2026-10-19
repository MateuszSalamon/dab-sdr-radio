#include <memory>
#include <string>
#include <vector>

#include "grpc_adapter_interfaces.h"
#include "ofdm/ofdm_demodulator.h"
#include "dab/fic/fic_decoder.h"
#include "basic_radio/basic_radio.h"
#include "dab/constants/dab_parameters.h"

namespace dab::grpc {

inline std::shared_ptr<OFDM_Demod> CreateLiveOFDM(const DAB_Parameters& params) {
    (void)params;
    return nullptr;
}

inline std::shared_ptr<FIC_Decoder> CreateLiveFIC(const DAB_Parameters& params) {
    return std::make_shared<FIC_Decoder>(params.nb_fib_cif_bits, params.nb_fibs_per_cif);
}

inline std::shared_ptr<BasicRadio> CreateLiveBasicRadio(const DAB_Parameters& params, size_t nb_threads = 0) {
    return std::make_shared<BasicRadio>(params, nb_threads);
}

} // namespace dab::grpc
