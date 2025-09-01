# Gemini Context: DanDike NewBoard CPU1 Firmware

## Project Overview

This project is the firmware for the CPU1 core of the "DanDike NewBoard," a sophisticated signal processing and generation platform, likely for power systems testing and simulation. It runs on a Xilinx embedded system (indicated by Vitis, Xilinx libraries, and hardware parameters).

The application is responsible for:
- **Signal Generation:** Generating precise AC/DC voltage and current signals, including complex waveforms with multiple harmonics (`SetACS`, `SetHarm`).
- **Signal Analysis:** Capturing signals via ADC and performing FFT analysis (`ADDA.c`, `My_KissFft.c`) to measure parameters like frequency, amplitude, phase, power factor, and total harmonic distortion (THD).
- **Hardware Interfacing:** Directly controlling a wide range of peripherals, including AD/DA converters (via DMA), IIC devices (RTC 8025), GPS for time synchronization, and digital I/O.
- **Communication:** Receiving control commands and sending back data via a JSON-based protocol. The communication is managed through a shared memory message queue (`Msg_Que.c`), suggesting a multi-core or multi-processor architecture (e.g., communicating with a Linux core on a Zynq device).
- **Calibration:** The system includes routines for calibrating the analog inputs and outputs (`Rc64.c`, `handle_WriteCalibrateAC`).

### Core Technologies
- **Language:** C
- **Platform:** Xilinx Embedded System (likely Vitis toolchain)
- **Libraries:**
    - `cJSON`: For parsing command and control messages.
    - `kiss_fft`: For performing Fast Fourier Transforms on captured signals.
    - Xilinx Standalone BSP: `xparameters.h`, `xscugic`, `xaxidma`, etc.

## Building and Running

The project appears to be a standard Xilinx Vitis application. There are no Makefiles or command-line build scripts in this directory.

- **Build:** The project should be built using the Xilinx Vitis IDE.
- **Run:** The application is likely run on the target hardware via the Vitis debugger or by booting the system from flash memory containing the compiled application.

```
# TODO: Add specific command-line build steps if they exist (e.g., using Vitis CLI).
```

## Development Conventions

- **Communication Protocol:** The primary interface for controlling the device is through JSON messages. The definitions for commands and data structures are in `Communications_Protocol.h`. New commands are added to the `FunCodeMap` in `Communications_Protocol.c`.
- **Hardware Abstraction:** Each major hardware component has its own driver module (e.g., `ADDA.c`, `gps.c`, `IIC_Master.c`).
- **State Management:** Global variables and `static` variables are used to maintain the state of the device, such as current waveform parameters (`Wave_Amplitude`, `Wave_Frequency`), device status (`devState`), and calibration data.
- **Concurrency:** The system uses mutexes (`mutex_utils.c`) to manage access to shared resources, which is critical in a system that uses interrupts and DMA. The main loop in `main.c` is event-driven, responding to flags set by interrupt handlers or command parsers (`dac_parameters_updated_by_command`, `AdcFinish_Flag`).
- **Memory Layout:** The memory map is defined in `lscript.ld`. Key memory regions like shared memory for communication are explicitly defined in headers (e.g., `Share_addr` in `ADDA.h`).
