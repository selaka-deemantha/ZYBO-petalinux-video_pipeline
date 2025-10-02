# QT cross compilation

This branch contain, qt cross compilation method and petalinux qt enable in rootfs. 

## Enable QT in petalinux

**working_vga_pipeline** contain hardware design and device tree design of the zybo vga pipeline. run that branch before this branch and make sure you have working vga pipeline with framebuffer support **(/dev/fb0)**

To enable QT in petalinux following packages and libs need to be enabled in rootfs. 

first run **petalinux-config -c rootfs** and enable following packages. For the project petalinux 2024 have used.

Go to 
**Petalinux Package Groups > packagegroup-xilinx-qt** and enable **packagegroup-xilinx-qt** and **populate_sdk_qt5**. we need **populate_sdk_qt5** this to cross compile qt applications in a host machine. 

And also go to the **Petalinux Package Groups > packagegroup-xilinx-qt-extended** and enable **packagegroup-xilinx-qt-extended**. 

### packagegroup-xilinx-qt

A predefined package group provided by Xilinx under Package Groups in PetaLinux.
It includes all commonly required Qt components for embedded GUI development on Xilinx boards.

We can also enable original qt libraries also. This allows user to choose only the required libraries. This method is importatnt if user wants to develop small size image.

Go to the **Filesystem Packages  > misc** and enable those packages. 
- Filesystem Packages  > misc  > qtbase: qtbase, qtbase-tools
- Filesystem Packages  > misc  > qtcharts: qtcharts
- Filesystem Packages  > misc  > qtdeclarative: qtdeclarative


## Install petalinux sdk 

Follow these steps to generate and package the PetaLinux SDK for cross-compiling Qt applications:
- **Build the PetaLinux Project**
    ```build
    petalinux-build
    ```

- **Create the SDK**
    ```sdk
    petalinux-build --sdk
    ```
    Creates a self-extracting SDK installer (sdk.sh) inside the images/linux/ directory.

- **run sdk.sh and install petalinux-sdk**
    




