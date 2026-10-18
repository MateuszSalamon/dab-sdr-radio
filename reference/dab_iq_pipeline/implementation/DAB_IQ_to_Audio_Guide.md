# DAB IQ → Audio: Narrative Guide, Flowchart and Architecture Summary

This guide explains how raw IQ samples are processed end-to-end in the repository to deliver audio (and related application data) to the GUI. It ties the code you already extracted to the functional stages of a DAB receiver pipeline so you can use it as a learning reference.

## Quick overview

High-level pipeline (brief):
1. IQ capture (external, not shown) → input to OFDM demodulator
2. OFDM demodulation (null detection, PRS, coarse/fine sync, FFT)
3. DQPSK demapping → Viterbi decoder → frame bits
4. Frame split: FIC (control) and MSC (main service)
5. FIC → FIC_Decoder → FIBs → FIG processor (FIG_Processor) → radio database updater via Radio_FIG_Handler
6. MSC → per-subchannel MSC runner → audio (DAB) or DAB+ decoder → audio frames
7. MOT/Slideshow processing → images for GUI
8. GUI receives updates via observable callbacks and database updater

Files in this reference set and their role
- ofdm_demodulator.h — OFDM demodulator interface, buffering and threading
- ofdm_demodulator_threads.{h,cpp} — pipeline thread coordination primitives
- fic_decoder.{h,cpp} — FIC decoding (Viterbi, descramble, CRC) and FIB emission
- fig_processor.h — FIG parsing entry points and per-FIG handlers (type-based)
- radio_fig_handler.h — bridges FIG events to the DAB database updater
- basic_radio.cpp — top-level router that accepts frame bits, schedules FIC and MSC processing, manages channels
- basic_audio_channel.cpp — channel wrapper that holds MSC decoder + slideshow manager
- basic_slideshow.cpp — MOT slideshow processing (image extraction and lifecycle)

## Step-by-step narrative (detailed)

1) OFDM demodulation (src/ofdm/*)
- Input: complex<float> IQ samples representing one CIF duration (or streaming blocks).
- Goals: find null symbol (L1 null), locate PRS, correct coarse and fine frequency/time offsets, perform FFT on carriers and produce demapped carrier data.
- Key stages:
  - FindNullPowerDip: searches circular buffer for L1 null power dip that indicates frame boundary.
  - ReadNullPRS: reads PRS reference symbols used to align frequency response and timing.
  - RunCoarseFreqSync: estimate and apply coarse frequency correction (keeps receiver locked to center frequency offset).
  - RunFineTimeSync: correlation-based impulse detection to place symbol boundaries accurately.
  - ReadSymbols: perform per-symbol processing and hand over to pipeline threads.
- Concurrency: the demodulator uses multiple pipeline threads coordinated with OFDM_Demod_Pipeline and OFDM_Demod_Coordinator primitives. These ensure FFT, phase correction and viterbi-bit extraction are parallelized safely.

Why this matters: OFDM demodulation recovers the transmitted carriers and reduces channel impairments so the digital bitstream can be recovered reliably.

2) DQPSK demapping and Viterbi / output bits
- After FFT, the code computes relative phase and demaps DQPSK sequence pairs into soft symbols (complex samples are turned into soft metrics).
- CalculateViterbiBits transforms these soft symbols into convolutional bits (viterbi_bit_t array) that form a CIF worth of encoded frame bits.
- Observable: OFDM_Demod exposes an Observable that notifies when a full frame (viterbi bits) is ready.

3) Top-level framing and BasicRadio (src/basic_radio/*)
- BasicRadio::Process receives a contiguous array of viterbi_bit_t for one frame.
- The frame is split into FIC (fast information channel) bits and MSC bits based on DAB parameters (nb_fic_bits, nb_msc_bits).
- BasicRadio schedules FIC processing and all configured MSC runner tasks on a thread pool (BasicThreadPool). After tasks return it calls UpdateAfterProcessing().
- UpdateAfterProcessing examines the database updater (from FIC) and, when new subchannels or service component info appears, it dynamically creates MSC runners and channel objects (Basic_DAB_Channel, Basic_DAB_Plus_Channel, Basic_Data_Packet_Channel) for decoding.

4) FIC decoding and FIG handling (src/dab/fic/*)
- FIC_Decoder accepts the encoded bits for the FIC portion of a CIF and performs:
  - Viterbi decoding (convolutional chainback)
  - Additive descrambling
  - CRC16 verification for each FIB (Fast Information Block)
- Each valid FIB triggers the FIC_Decoder's observable (obs_on_fib) which emits raw FIB payload bytes.
- FIG_Processor consumes each FIB payload and parses FIGs (Fast Information Groups) according to their Type/Extension and length fields. It delegates semantics to a FIG_Handler_Interface (Radio_FIG_Handler is one implementation that updates the system database).
- Radio_FIG_Handler translates FIG events into DAB database updates: ensemble metadata, subchannel definitions (UEP/EEP), service components, labels, frequencies, MOT metadata, etc.

5) MSC decoding and audio
- MSC runners process the MSC bitstream for each subchannel.
- For audio stream subchannels:
  - DAB (AAC or MPEG-style) or DAB+ decoders are instantiated by BasicRadio when database indicates service component types.
  - MSC decoders output PCM or decoded audio frames to the higher-level audio channel object.
- For data subchannels in packet mode, packet-handling runners extract packet payload and outer-FEC is applied according to subchannel FEC scheme.

6) MOT and slideshow (src/basic_radio/basic_slideshow.cpp and dab/mot/*)
- MOT (Multimedia Object Transfer) entities are carried in MSC packet streams.
- MOT entities are parsed into MOT_Entity structures; slideshow entities (JPEG/PNG) are converted into Basic_Slideshow objects with metadata (trigger time, expiry, alt URLs).
- Basic_Slideshow_Manager retains last N slides and notifies observers via m_obs_on_new_slideshow to update GUI elements.

7) GUI / Database / Observers
- The code uses Observable<T> patterns to notify consumers when:
  - a new OFDM frame was processed (frame FFT / data)
  - new FIB/FIB groups are available
  - new subchannels or services were discovered
  - new slides are available
- A central DAB database (DAB_Database) contains the parsed configuration and current service/component states; GUI components read this or subscribe to updates through a DatabaseUpdater.

---

## Rendered diagram

- Mermaid source: `reference/dab_iq_pipeline/implementation/diagram.mmd`
- Rendered SVG: `reference/dab_iq_pipeline/implementation/diagram.svg`

You can view the Mermaid source or open the SVG directly in a browser. The Mermaid source is also embeddable in supporting viewers.


## Flowchart (linear ASCII)

IQ samples
  |
  v
OFDM demodulator (null/PRS detection)
  |
  v
FFT per symbol -> DQPSK demap -> soft metrics
  |
  v
Viterbi soft-decoder -> viterbi_bit_t frame
  |
  +---> FIC bits -> FIC_Decoder -> FIBs -> FIG_Processor -> Radio_FIG_Handler -> DatabaseUpdater
  |
  +---> MSC bits -> per-subchannel MSC runners -> (DAB / DAB+) decoders -> audio frames
                                    |
                                    v
                             MOT / slideshow -> Basic_Slideshow_Manager -> GUI

Concurrency: OFDM demod threads run in parallel to pipeline decoding; BasicRadio runs FIC and MSC in a thread pool to decode both channels concurrently.

## Design architecture summary

Components and responsibilities
- OFDM layer (src/ofdm)
  - Purpose: Convert raw IQ into carrier-level complex samples and recover symbol timing/frequency.
  - Main interfaces: OFDM_Demod::Process() and Observable for frame bits.
  - Threading: pipeline threads + coordinator for parallel FFT and demapping.

- FIC layer (src/dab/fic)
  - Purpose: Process Fast Information Channel, producing FIB payloads and passing FIGs for configuration.
  - Main classes: FIC_Decoder, FIG_Processor, FIG_Handler_Interface, Radio_FIG_Handler.
  - Important processing steps: Viterbi decode, descrambler, CRC check, FIG parsing.

- MSC layer and runners (src/basic_radio + src/dab/msc)
  - Purpose: Decode multiplexed services (audio, data) per subchannel.
  - Runners are created dynamically based on database info discovered by FIC/FIG.
  - Audio runners instantiate appropriate decoders for DAB or DAB+.

- MOT / Application data handlers (src/dab/mot + basic_slideshow)
  - Purpose: Parse MOT entities and surface images/metadata to UI.

- Database & Updater
  - Purpose: Maintain current ensemble/subchannel/service metadata and notify listeners when things change.
  - Data consumers (GUI) read database for labeling, frequency lists, service catalogs, etc.

Data flow and interfaces
- Observables (Observable<T>) are used for in-process event streaming (frames, FIBs, slides). This reduces tight coupling.
- BasicRadio is the central runtime orchestrator connecting FIC and MSC runners and housing thread pool.
- Radio_FIG_Handler is the glue that maps FIG parser callbacks to database updates.

Threading model
- OFDM: internal pipelines and coordinator to maximize FFT and demap throughput.
- BasicRadio: thread pool to decode FIC + all MSC runners concurrently per CIF.
- Synchronous sections: UpdateAfterProcessing merges database updates in a mutex to keep shared state consistent.

Key types & terms
- CIF: Common Interleaved Frame — a DAB framing unit
- FIB: Fast Information Block — small control block inside FIC
- FIG: Fast Information Group — higher-level control objects (labels, subchannel maps, MOT registration)
- MSC: Main Service Channel — contains audio/data payloads multiplexed across subchannels
- DQPSK: Differential QPSK — carrier modulation used in DAB carrier symbols
- viterbi_bit_t: soft/hard-bit type used by the Viterbi decoder

## Where to explore in code (recommended path)
1. src/ofdm/ofdm_demodulator.h & ofdm_demodulator_threads.{h,cpp} — understanding how symbols are located and how threads are structured
2. src/ofdm/ofdm_demodulator.cpp — implementation of the signal processing flow (FFT, phase error, demapping)
3. src/dab/fic/fic_decoder.{h,cpp} — how FIBs are recovered (viterbi + descrambler + CRC)
4. src/dab/fic/fig_processor.{h,cpp} & src/dab/radio_fig_handler.{h,cpp} — how FIGs are parsed and applied
5. src/basic_radio/basic_radio.cpp — how frames are routed, thread pool usage, and dynamic runner creation
6. src/basic_radio/basic_audio_channel.cpp and src/dab/msc/* — look into MSC -> audio decoding
7. src/basic_radio/basic_slideshow.cpp & src/dab/mot/* — MOT handling and GUI integration

## Tips for experimenting
- Build with logging enabled and run with a short recorded CIF (or IQ stream) to step through the pipeline.
- Add observers (or temporary prints) to OFDM_Demod::On_OFDM_Frame and FIC_Decoder::OnFIB to see frames and control messages.
- Use small unit tests that feed known IQ samples (or preprocessed FFT outputs) and check each layer independently.

---

This file is committed under reference/dab_iq_pipeline/implementation/DAB_IQ_to_Audio_Guide.md
