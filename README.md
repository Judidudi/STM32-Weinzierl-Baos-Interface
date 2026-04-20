# STM32-Weinzierl-Baos-Interface

## Overview

This repository contains a complete bare-metal firmware project for the STM32F446RE microcontroller. It implements communication with a Weinzierl BAOS module (e.g. BAOS 830 / 832) using the BAOS Binary Protocol over a UART interface (FT1.2 framing).

The project is intended as a working reference for integrating KNX communication into an embedded system via a BAOS object server. The KNX stack itself is executed on the BAOS module; the STM32 acts as a communication master and application controller.

## Key Characteristics

- Target platform: STM32F446RE
- Programming model: bare-metal, register-level access
- No use of STM32 HAL
- CMSIS required for register definitions
- UART-based communication with BAOS module
- Implementation of FT1.2 framing and BAOS protocol handling
- Structured separation of hardware access, protocol handling, and application logic

## System Architecture

The system consists of two main components:

STM32F446RE:
- UART initialization and handling (register-based)
- Frame construction and parsing (FT1.2)
- BAOS command handling
- Application-specific logic

Weinzierl BAOS Module:
- KNX protocol stack (certified)
- Object server abstraction
- Interface to KNX bus

The STM32 does not process KNX telegrams directly. All interaction with the KNX system is performed via BAOS commands.

## Repository Structure

Core/
  Src/        Application entry point and main logic
  Inc/        Header files

Drivers/
  Low-level peripheral configuration (GPIO, RCC, UART)

baos/
  BAOS protocol implementation
  - Frame encoding and decoding (FT1.2)
  - Command construction
  - Response parsing

main.c
  Program entry point

## Requirements

### Software

- ARM GCC toolchain or STM32CubeIDE
- CMSIS (mandatory)

The project relies on CMSIS for direct register access. CMSIS headers must be correctly included in the build environment.

### Hardware

- STM32F446RE (e.g. Nucleo-64)
- Weinzierl BAOS module (BAOS 830 or 832)
- UART connection between MCU and BAOS (TTL level)
- KNX bus connection via BAOS module

## UART Configuration

Typical BAOS configuration:

- Baud rate: 9600
- Data bits: 8
- Parity: even
- Stop bits: 1

The configuration must match the BAOS module settings.

## Communication Model

Communication follows a master-slave scheme:

1. STM32 sends a BAOS request frame
2. BAOS module processes the request
3. BAOS module returns a response frame
4. STM32 parses and evaluates the response

The implementation includes:

- Frame generation according to FT1.2
- Reception and validation of frames
- Parsing of BAOS responses
- Handling of object server communication

## Build Instructions

### Using STM32CubeIDE

1. Import the project into the workspace
2. Ensure CMSIS include paths are correctly configured
3. Build the project
4. Flash the firmware to the target

## Notes

- The project does not implement a KNX stack
- All KNX interaction is performed via the BAOS object server
- Timing and framing does comply with the FT1.2 specification
