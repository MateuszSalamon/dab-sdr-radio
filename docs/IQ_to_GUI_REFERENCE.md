# IQ → OFDM → FIC/MSC → GUI: Implementation Reference

This reference extracts where the implementation for the pipeline from IQ input (OFDM demodulation) through FIC/MSC processing and into the GUI / basic app lives in the repository. Use this as a reading map to follow the code path for Digital Audio Broadcasting (DAB) signal processing implemented in this project.

> Note: links point to the commit/ref the assistant inspected: ref 0a09ba34a40286cf77e0c48aa47cac64b86341ef

---

## High-level pipeline

1. IQ input / OFDM demodulation
2. Frame handling and PRS/mapper / OFDM helpers
3. FIG / FIC decoding and FIG processing
4. FIC → MSC routing and basic runners
5. GUI / app integration (slideshow / audio controls / channel management)

Follow the sections below in order to trace the code flow.

---

## 1) IQ input and OFDM demodulation

Primary implementation:

- ofdm demodulator (core of IQ → symbol demapping / FFT / carrier handling)
  - src/ofdm/ofdm_demodulator.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/ofdm_demodulator.cpp
  - src/ofdm/ofdm_demodulator.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/ofdm_demodulator.h
  - Key responsibilities: receive IQ samples, perform OFDM FFT, apply PRS (phase reference symbols) correction, equalization, and produce per-subcarrier complex symbols.

- OFDM helpers and frame buffering
  - src/ofdm/ofdm_helpers.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/ofdm_helpers.h
  - src/ofdm/ofdm_frame_buffer.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/ofdm_frame_buffer.h
  - These contain utilities and buffers used by the demodulator to reassemble OFDM frames and manage timing.

- PRS and mapping references
  - src/ofdm/dab_prs_ref.cpp / .h (PRS handling)
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/dab_prs_ref.cpp
  - src/ofdm/dab_mapper_ref.cpp / .h (symbol mapping / constellation & DAB carrier map)
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/dab_mapper_ref.cpp

- OFDM threading & orchestration
  - src/ofdm/ofdm_demodulator_threads.cpp / .h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/ofdm/ofdm_demodulator_threads.cpp

Reading order suggestion: ofdm_demodulator.h → ofdm_demodulator.cpp → ofdm_frame_buffer.h → dab_prs_ref.cpp → dab_mapper_ref.cpp

---

## 2) Frame parsing → FIG & FIC decoding

After OFDM demodulation the system reconstructs frames and extracts Fast Information Channel (FIC) MAP, and FIG (Fast Information Group) fields.

Primary files:

- FIG processing core
  - src/dab/fic/fig_processor.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/fic/fig_processor.h
  - src/dab/fic/fig_processor.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/fic/fig_processor.cpp
  - Responsibilities: parse FIG groups, decode TLV-style metadata that the FIC provides (e.g., service lists, subchannel configurations, MOT links), and call FIG handlers.

- FIC decoder
  - src/dab/fic/fic_decoder.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/fic/fic_decoder.h
  - src/dab/fic/fic_decoder.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/fic/fic_decoder.cpp
  - Responsibilities: lower-level decoding of FIC channel blocks into FIG groups; packet reassembly and error handling.

- FIG handler interface
  - src/dab/fic/fig_handler_interface.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/fic/fig_handler_interface.h
  - This defines the callbacks and handler registration used by FIG processing to route FIGs to higher-level consumers.

- radio_fig_handler (higher-level FIG handling and dispatch)
  - src/dab/radio_fig_handler.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/radio_fig_handler.h
  - src/dab/radio_fig_handler.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/radio_fig_handler.cpp
  - Responsibilities: integrate FIG processing into the radio stack, manage service lists, update internal state and notify other modules (MSC, MOT, GUI).

Reading order suggestion: fic_decoder → fig_processor → fig_handler_interface → radio_fig_handler

---

## 3) FIC → MSC and basic runners

The FIC contains the service configuration. That configuration is used to map MSC (main service channel) content to audio/data channels.

Key files in basic runner glue code:

- basic_fic_runner
  - src/basic_radio/basic_fic_runner.cpp / .h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_fic_runner.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_fic_runner.h
  - Connects FIC-derived configuration to the rest of the basic_radio app.

- basic_msc_runner (MSC runner header in this repo)
  - src/basic_radio/basic_msc_runner.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_msc_runner.h
  - The MSC runner(s) will take subchannel assignment and start MSC processing for services.

- basic_radio (main orchestration for a simple app)
  - src/basic_radio/basic_radio.cpp / .h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_radio.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_radio.h
  - Initializes demodulator, FIG handlers, runners, and audio subsystems. Good place to see end-to-end wiring for a minimal app.

---

## 4) GUI / Presentation / Audio control

The basic GUI-like components in this repo are fairly lightweight (slideshow, basic audio controls) and integrated with the basic_radio app.

- Basic slideshow and UI elements
  - src/basic_radio/basic_slideshow.cpp / .h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_slideshow.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_slideshow.h
  - These present MOT images / slideshow elements produced by FIG/MOT handlers.

- Audio channel & controls
  - src/basic_radio/basic_audio_channel.cpp / .h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_audio_channel.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_audio_channel.h
  - src/basic_radio/basic_audio_controls.cpp / .h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_audio_controls.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/basic_radio/basic_audio_controls.h

- How GUI is fed: FIG and radio_fig_handler update service & MOT state; basic_radio registers runners/handlers and passes content to slideshow / audio channel.

---

## 5) Other helpers / logging / constants

- Logging utilities for DAB stack
  - src/dab/dab_logging.*
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/dab_logging.cpp
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/dab_logging.h

- Misc info & constants
  - src/dab/dab_misc_info.h
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/dab/dab_misc_info.h

---

## Quick start to read the code (recommended sequence)

1. src/ofdm/ofdm_demodulator.h + .cpp — understand how IQ samples become OFDM symbols.
2. src/ofdm/ofdm_frame_buffer.h — how frames are buffered and passed along.
3. src/dab/fic/fic_decoder.* — how FIC blocks are turned into FIG groups.
4. src/dab/fic/fig_processor.* — how FIGs are decoded and dispatched.
5. src/dab/radio_fig_handler.* — how FIGs update radio state and notify other modules.
6. src/basic_radio/basic_fic_runner.* and basic_msc_runner.h — how FIC and MSC are orchestrated in the simple app.
7. src/basic_radio/basic_slideshow.*, basic_audio_channel.*, basic_audio_controls.* — how GUI and audio are presented.

---

## Build / Run notes

- The project uses CMake. Top-level and subdirectory CMakeLists.txt are present under src and subfolders. See src/CMakeLists.txt and subfolder CMakeLists.txt files.
  - src/CMakeLists.txt
    - https://github.com/MateuszSalamon/dab-sdr-radio/blob/0a09ba34a40286cf77e0c48aa47cac64b86341ef/src/CMakeLists.txt

- Many components are CPU-heavy (OFDM/FFT) — check simd_flags.h and ofdm/profiler.h for performance helpers.

---

## How this file was created

I inspected the repository tree under src at commit 0a09ba3 and collected the files implementing the pipeline from IQ/OFDM demodulation, through FIC/FIG processing, to the basic GUI elements used by the simple application. The links above point to the repo files so you can open them and read implementation details.

If you want, I can also:
- Extract short code snippets (key functions or class declarations) into this document for quick reference.
- Create a sequence diagram or flow diagram showing function/class call relationships.
- Add a mapping of which functions call which (a lightweight call graph) for a specific path (e.g., ofdm_demodulator -> fic_decoder -> fig_processor -> radio_fig_handler -> basic_radio).

---

End of reference.
