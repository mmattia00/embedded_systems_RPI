# embedded_systems_RPI
repo containing the projects for the course of Embedded Systems with Raspberry PI followed at Universidad Politecnica de Madrid on 2025

## Project 1: Local I²C Applications  
This project focused on developing low-level I²C applications running locally on a Raspberry Pi. We interacted with sensors using user-space programs in C, tested real-time data reading, and validated communication via basic terminal output. The work provided foundational experience with sensor integration and direct hardware communication using the I²C protocol.

## Project 2: Minimal Embedded Linux with Buildroot  
Here we built a custom Linux image for the Raspberry Pi using Buildroot. The repository contains the configuration file (`.config`) used to generate a minimal OS tailored for embedded applications.

## Project 3: Embedded IoT System with UDP & MQTT  
This final project integrates all previous work into a complete IoT system. A Raspberry Pi, running the custom Buildroot image, collects data from two I²C sensors (MPU-6050 for acceleration, TCS34725 for color) and sends buffered readings via UDP every 10 seconds to a server running on a Linux VM. The server calculates statistical metrics and, in parallel, sensor data is published to ThingsBoard using MQTT for real-time visualization.

We enabled I²C manually in the Buildroot system and handled SSH access configuration to control the Pi remotely. Data is visualized live via a dashboard in ThingsBoard. The client-server architecture was tested incrementally, starting from simple UDP "hello" messages, and evolved into the final multi-sensor communication system.

This project highlights the complete pipeline: embedded OS customization, sensor integration, network communication, and IoT data visualization.

