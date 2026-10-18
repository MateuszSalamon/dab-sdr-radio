# DAB IQ → Audio: Short Study Notes

## 1. Signal chain

Raw IQ samples -> OFDM demodulator -> DQPSK demap -> Viterbi decode -> FIC + MSC split -> service decode -> audio / slideshow.

## 2. OFDM stage

The repository does OFDM synchronization before decoding data:
- find the null symbol / frame boundary
- detect PRS symbols for timing/frequency reference
- estimate coarse frequency offset
- estimate fine time offset
- apply FFT and extract carrier data

This is implemented mainly in `src/ofdm/` and centered around `OFDM_Demod` and the worker/coordinator threads.

## 3. Bit recovery stage

After FFT, the code:
- computes relative phase
- demaps DQPSK symbols
- converts soft symbols into Viterbi bits

This produces a frame bit sequence representing one DAB CIF (common interleaved frame).

## 4. FIC / FIG control channel

The frame is split into:
- FIC: control information
- MSC: service payload

FIC decoding is handled by `FIC_Decoder`:
- Viterbi decoding
- descrambling
- CRC16 validation
- FIB output

Then `FIG_Processor` parses FIGs (Fast Information Groups), and `Radio_FIG_Handler` updates the DAB database.

This is how the receiver learns:
- ensemble identity
- subchannels
- service components
- labels / metadata
- frequencies / alternate services

## 5. MSC service payload stage

The MSC path carries audio and data for each subchannel.
`BasicRadio` splits the frame and schedules MSC runners for each known subchannel. Each runner decodes data according to the service type.

Typical outcomes:
- DAB audio stream
- DAB+ audio stream
- packet data channel
- MOT object transfer (for slideshows / images)

## 6. Audio and GUI output

The audio path ends in channel objects (`Basic_DAB_Channel`, `Basic_DAB_Plus_Channel`, etc.).
MOT objects are parsed and collected by `Basic_Slideshow_Manager`, then presented to the GUI as slideshow/image updates.

## 7. Architecture pattern

Key pattern in the project:
- raw IQ -> synchronized OFDM -> bits
- bits -> FIC for config / database
- bits -> MSC for audio and data
- database + observables -> GUI updates

## 8. Best files to read first

1. `src/ofdm/ofdm_demodulator.h`
2. `src/ofdm/ofdm_demodulator_threads.h`
3. `src/dab/fic/fic_decoder.h`
4. `src/dab/fic/fig_processor.h`
5. `src/dab/radio_fig_handler.h`
6. `src/basic_radio/basic_radio.cpp`
7. `src/basic_radio/basic_audio_channel.cpp`
8. `src/basic_radio/basic_slideshow.cpp`

## 9. Important idea

This repository is a reference implementation of a complete DAB receiver pipeline:
- radio front-end synchronization
- digital demodulation
- FIC metadata decoding
- MSC service extraction
- audio reconstruction and MOT display

That is the main design story behind the code.
