#!/bin/bash

read -p "Enter [SD card name] [SD card size] [Fat32 size]: " name size fat_size

echo "$name partition into $fat_size size fat32 partition and $((size - fat_size)) ext4 partition"

full_sd_path="/dev/$name"
fat32_lable="BOOT"
ext4_lable="ROOTFS"
mount_point_boot="/mnt/boot"

echo "WARNING: This will erase all data on $name"
read -p "type y to proceed: " confirm
[ "$confirm" != "y" ] && echo "Aborted." && exit 1

echo "Unmounting existing partitions..."
for part in $(lsblk -ln -o NAME "$full_sd_path" | grep -v "$(basename $name)$"); do
    echo "Unmounting $part"
    sudo umount "/dev/$part" 2>/dev/null || true
done

echo "Deleting all partitions by creating a new partition table..."
sudo parted -s "$full_sd_path" mklabel msdos

echo "Creating FAT32 partition..."
sudo parted -s "$full_sd_path" mkpart primary fat32 1MiB ${fat_size}GB
sudo parted -s "$full_sd_path" set 1 boot on

echo "Creating ext4 partition..."
sudo parted -s "$full_sd_path" mkpart primary ext4 ${fat_size}GB 100%

sleep 2
sudo mkfs.vfat -F 32 -n "$fat32_lable" "${full_sd_path}1"

# Create mount point for BOOT partition
echo "Creating mount point $mount_point_boot ..."
sudo mkdir -p "$mount_point_boot"

# Mount BOOT partition
echo "Mounting FAT32 BOOT partition..."
sudo mount "${full_sd_path}1" "$mount_point_boot"

# Copy boot files to BOOT partition
echo "Copying boot.scr, BOOT.BIN, image.ub to $mount_point_boot ..."
sudo cp boot.scr BOOT.BIN image.ub "$mount_point_boot/"

sudo umount ${full_sd_path}2 2>/dev/null || true
sudo dd if=rootfs.ext4 of=${full_sd_path}2 bs=4M status=progress conv=fsync

sync
