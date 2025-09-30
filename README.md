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
   ```

2. **Setup petalinux environment**
    ```source
    source <petalinux_path>/settings.sh
    ```
3. **Configure the XSA file**

    ```config
    petalinux-config --get-hw-description=/bsp

    ```
4. **Build the project**
    ```build
    petalinux-build
    petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --u-boot images/linux/u-boot.elf --fpga images/linux/system.bit --force
    ```
5. **Partition the SD card**
    
    copy the sd_partition.sh bash script into /images/linux folder. Run the Script. It will ask sd card name, size of the sd card and FAT32 partition size. FAT32 partition is needed for rootfs. 
eg: **<sdc card name> <total size of the sd card> <fat32 size required>**

eg: **sda 32 24**

Now this will partition the sd card and remove it from the machine and insert it into the zybo board. Put the zybo board into sd card boot mode. Connect a display via VGA. 
