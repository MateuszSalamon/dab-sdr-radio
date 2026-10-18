# DAB IQ-to-GUI Implementation Reference

This directory contains the extracted implementation reference for the DAB receiver pipeline in this repository: from raw IQ samples through OFDM demodulation, FIC/FIG parsing, MSC service processing, and into the basic radio / GUI presentation layer.

## Scope

This reference is organized to match a practical software-defined radio and digital broadcast receiver pipeline:

1. RF/IQ acquisition and OFDM demodulation
2. Fast Information Channel (FIC) and FIG decoding
3. Main Service Channel (MSC) routing and service decoding
4. Audio/data handling and basic presentation layer

## Main implementation locations

### 1. IQ and OFDM processing

- `src/ofdm/ofdm_demodulator.h`
- `src/ofdm/ofdm_demodulator.cpp`
- `src/ofdm/ofdm_frame_buffer.h`
- `src/ofdm/ofdm_helpers.h`
- `src/ofdm/dab_prs_ref.h`
- `src/ofdm/dab_prs_ref.cpp`
- `src/ofdm/dab_mapper_ref.h`
- `src/ofdm/dab_mapper_ref.cpp`

These files implement the receive-side digital signal processing path: IQ buffering, OFDM symbol recovery, PRS correction, and conversion to demodulated complex subcarrier data.

### 2. FIC / FIG control decoding

- `src/dab/fic/fic_decoder.h`
- `src/dab/fic/fic_decoder.cpp`
- `src/dab/fic/fig_processor.h`
- `src/dab/fic/fig_processor.cpp`
- `src/dab/fic/fig_handler_interface.h`
- `src/dab/radio_fig_handler.h`
- `src/dab/radio_fig_handler.cpp`

These files handle the DAB control plane. FIC blocks and FIG groups are parsed into service and configuration information. This is how the receiver learns which services and subchannels are active.

### 3. MSC service processing

- `src/basic_radio/basic_fic_runner.h`
- `src/basic_radio/basic_fic_runner.cpp`
- `src/basic_radio/basic_msc_runner.h`
- `src/basic_radio/basic_dab_channel.h`
- `src/basic_radio/basic_dab_channel.cpp`
- `src/basic_radio/basic_dab_plus_channel.h`
- `src/basic_radio/basic_dab_plus_channel.cpp`
- `src/basic_radio/basic_data_packet_channel.h`
- `src/basic_radio/basic_data_packet_channel.cpp`

These files represent the mapping between configured service elements and actual payload channels. FIC tells the receiver what exists; the MSC carries the actual data services.

### 4. Basic GUI / presentation layer

- `src/basic_radio/basic_radio.h`
- `src/basic_radio/basic_radio.cpp`
- `src/basic_radio/basic_audio_channel.h`
- `src/basic_radio/basic_audio_channel.cpp`
- `src/basic_radio/basic_audio_controls.h`
- `src/basic_radio/basic_audio_controls.cpp`
- `src/basic_radio/basic_slideshow.h`
- `src/basic_radio/basic_slideshow.cpp`

These are the app-level integration points for audio playback and end-user presentation.

## End-to-end flow

The implementation can be read as the following pipeline:

IQ samples
  -> OFDM demodulation
  -> FIC/FIG decoding
  -> service configuration and subchannel discovery
  -> MSC payload decoding
  -> audio or packet service handling
  -> basic radio presentation layer

## Recommended reading order

1. `src/ofdm/ofdm_demodulator.h`
2. `src/ofdm/ofdm_demodulator.cpp`
3. `src/ofdm/ofdm_frame_buffer.h`
4. `src/dab/fic/fic_decoder.cpp`
5. `src/dab/fic/fig_processor.cpp`
6. `src/dab/radio_fig_handler.cpp`
7. `src/basic_radio/basic_radio.cpp`
8. `src/basic_radio/basic_audio_channel.cpp`
9. `src/basic_radio/basic_slideshow.cpp`

## Practical learning value

This code is a practical reference for understanding:

- how DAB receivers turn RF IQ samples into decoded service data
- how OFDM is used in the receiver chain
- how control metadata is extracted from the FIC
- how service configuration and payload channels are mapped
- how a minimal SDR-style application surfaces the result to users

## Reference note

This is not a standalone library; it is an implementation reference extracted from the existing project source. The original code is the authoritative implementation, and these notes are intended to provide a readable guide to the pipeline structure.
