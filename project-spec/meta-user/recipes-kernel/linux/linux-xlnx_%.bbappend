FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append = " file://bsp.cfg"
KERNEL_FEATURES_append = " bsp.cfg"
SRC_URI += "file://user_2025-07-14-14-56-00.cfg \
            file://user_2025-07-18-08-53-00.cfg \
            file://user_2025-07-18-11-09-00.cfg \
            file://user_2025-07-18-11-41-00.cfg \
            file://user_2025-07-19-16-30-00.cfg \
            file://user_2025-07-22-17-02-00.cfg \
            "

