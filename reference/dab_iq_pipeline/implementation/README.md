# DAB IQ-to-GUI implementation reference

This directory contains extracted C++ source files from the repository, grouped to follow the signal-processing path from raw IQ samples through OFDM demodulation, FIC/FIG control decoding, and into the basic radio presentation layer.

The files are copied from the original project and kept here for easier review.

## Files included

- `ofdm_demodulator.h`
- `ofdm_demodulator_threads.h`
- `ofdm_demodulator_threads.cpp`
- `fic_decoder.h`
- `fic_decoder.cpp`
- `fig_processor.h`
- `radio_fig_handler.h`
- `basic_radio.cpp`
- `basic_audio_channel.cpp`
- `basic_slideshow.cpp`

## Reading order

1. `ofdm_demodulator.h`
2. `ofdm_demodulator_threads.h`
3. `ofdm_demodulator_threads.cpp`
4. `fic_decoder.h`
5. `fic_decoder.cpp`
6. `fig_processor.h`
7. `radio_fig_handler.h`
8. `basic_radio.cpp`
9. `basic_audio_channel.cpp`
10. `basic_slideshow.cpp`

These are direct implementation excerpts from the repository and are intended for easier code review and learning.
