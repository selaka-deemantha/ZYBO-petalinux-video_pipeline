# Zybo VGA Pipeline on Linux

This branch contains a **VGA video pipeline for the Zybo board** that runs on **Embedded Linux**.  
The design was created using **Vivado 2021** for the hardware and **PetaLinux 2024** for the software platform.

---

## 🚀 Overview
The project implements a VGA display pipeline on the Zybo board using Linux.  
Key components in the hardware design include:
- AXI VDMA
- Video Timing Controller (VTC)
- Stream-to-Video-Out
- RGB2VGA / RGB2DVI bridge
- Dynamic clock generator

The PetaLinux project configures the Linux kernel, device tree, and rootfs to support the video pipeline.

---


## 🟢 How to Build & Run
1. **Clone the repository**
   ```bash
   git clone -b working_vga_pipeline git@github.com:selaka-deemantha/ZYBO-petalinux-video_pipeline.git
   cd <repo>

   iasjfoiasoifja
