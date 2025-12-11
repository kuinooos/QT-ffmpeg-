# Project Introduction - Kunpeng Innovation Competition

## Project Overview

This project is an edge-cloud collaborative intelligent voice interaction system for the Kunpeng ecosystem, deployed on the OrangePi Kunpeng Pro development board. It achieves tight collaboration between edge-side audio/video processing and cloud-based intelligent analysis. The system implements high-performance edge processing modules in C++ and builds cloud intelligent services in Python, fully leveraging the multi-core performance advantages of the Kunpeng architecture through NUMA-aware memory allocation and CPU affinity scheduling optimization.

The project explores NPU (Neural Processing Unit) hardware acceleration integration. By binding data-feeding threads to specific NUMA nodes (Node 0, Kunpeng CPU 16GB memory domain) and optimizing memory allocation with numactl, it ensures data access locality and reduces cross-NUMA access latency. In the NPU acceleration path, CPU-side affinity scheduling maintains stable data supply to continuously saturate the NPU input buffer, maximizing hardware utilization. In pure CPU processing mode, the NUMA memory allocation strategy also ensures efficient operation of the audio encoding/decoding pipeline, achieving low-latency, high-concurrency real-time speech recognition and interaction response capabilities, providing a complete technical practice solution for edge intelligence scenarios on the Kunpeng platform.

## Core Technical Features

### 1. Edge-Cloud Collaborative Architecture
- **Edge-side (C++)**: High-performance audio/video capture, preprocessing, encoding, and decoding modules on OrangePi Kunpeng Pro
- **Cloud-side (Python)**: Intelligent speech recognition (ASR) service, natural language processing, and business logic processing
- **Communication Mechanism**: Real-time data transmission and synchronization based on efficient network protocols

### 2. NUMA and CPU Affinity Optimization
- **Memory Allocation Optimization**: Use numactl to allocate critical data structures to Node 0 (Kunpeng CPU main memory domain, 16GB), reducing cross-node memory access latency and improving cache hit rate
- **CPU Binding Strategy**: Bind audio processing threads and encoding/decoding threads to cpu0-3 (all located in Node 0), ensuring data processing locality and reducing NUMA remote access overhead
- **Data-Feeding Thread Affinity**: In NPU acceleration scenarios, fix data-feeding threads to specific cores through CPU affinity scheduling, ensuring stable and continuous filling of NPU input buffers to maximize NPU throughput

### 3. NPU Hardware Acceleration Integration (Exploratory Solution)
- **Acceleration Target**: Compute-intensive tasks such as audio feature extraction and neural network inference
- **Integration Strategy**: Through CPU-side stable feeding mechanism combined with affinity scheduling, continuously saturate NPU input buffers to fully utilize NPU computing power
- **Flexibility Design**: The system supports pure CPU processing path simultaneously; when NPU is unavailable or not debugged, it can still ensure service stability and performance through optimized CPU multi-threaded processing

### 4. Audio/Video Processing Optimization
- **FFmpeg Integration**: Efficient audio/video demuxing, decoding, and encoding based on FFmpeg 4.2.1
- **SDL Multimedia Output**: Cross-platform audio playback and video rendering using SDL2
- **Audio/Video Synchronization**: Precise synchronization strategy based on audio clock to ensure playback quality
- **Multi-threaded Architecture**: Independent demuxing, decoding, and rendering threads with queue-based buffering mechanism to improve concurrent processing capability

## Technical Highlights

1. **Kunpeng Ecosystem Adaptation**: Fully utilizes the multi-core architecture of Kunpeng CPU, achieving high-performance edge computing on OrangePi Kunpeng Pro development board through NUMA awareness and CPU affinity optimization

2. **Memory Access Optimization**: numactl configures Node 0 memory allocation strategy, combined with CPU0-3 binding, significantly reducing NUMA remote access latency and improving audio processing pipeline efficiency

3. **Hardware Acceleration Exploration**: Designed NPU acceleration integration solution, ensuring CPU feeding thread stability through affinity scheduling, continuously providing data to NPU, exploring best practices for edge AI acceleration

4. **Edge-Cloud Collaborative Design**: Both edge-side and cloud-side run on the same OrangePi Kunpeng Pro hardware, achieving collaborative optimization of edge intelligence and cloud services through process isolation and resource scheduling

5. **Service Stability**: Adopts multi-path processing mechanism, NPU and CPU processing paths can be dynamically switched, ensuring service availability under various hardware conditions

## Application Scenarios

- **Intelligent Voice Assistant**: Low-latency real-time speech recognition and conversational interaction
- **Edge Audio/Video Processing**: Localized audio/video encoding/decoding and intelligent analysis
- **IoT Terminals**: Multimedia processing and cloud collaboration for edge devices
- **Intelligent Conference Systems**: Real-time speech transcription and intelligent meeting recording

## Performance Optimization Summary

Through the combined optimization of NUMA memory allocation strategy (numactl) and CPU affinity scheduling, this project achieves on the OrangePi Kunpeng Pro platform:
- Reduced audio encoding/decoding latency
- Improved memory access efficiency
- Optimized CPU multi-core utilization
- Enhanced NPU (if successfully integrated) input buffer saturation

These optimization measures fully leverage the hardware characteristics of the Kunpeng architecture, providing high-performance, low-latency processing capabilities for edge intelligence applications.

---

**Author Email**: 1832578485@qq.com  
**Development Platform**: OrangePi Kunpeng Pro  
**Competition Project**: Kunpeng Innovation Competition - Development Board Track
