FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI_append = " file://platform-top.h file://bsp.cfg"

do_configure_append () {
	install ${WORKDIR}/platform-top.h ${S}/include/configs/
}

do_configure_append_microblaze () {
	if [ "${U_BOOT_AUTO_CONFIG}" = "1" ]; then
		install ${WORKDIR}/platform-auto.h ${S}/include/configs/
		install -d ${B}/source/board/xilinx/microblaze-generic/
		install ${WORKDIR}/config.mk ${B}/source/board/xilinx/microblaze-generic/
	fi
}
SRC_URI += "file://user_2025-07-18-10-33-00.cfg \
            file://user_2025-07-18-11-02-00.cfg \
            file://user_2025-07-18-11-31-00.cfg \
            file://user_2025-07-18-11-59-00.cfg \
            file://user_2025-07-18-12-08-00.cfg \
            file://user_2025-07-18-12-24-00.cfg \
            file://user_2025-07-18-12-34-00.cfg \
            file://user_2025-07-18-12-49-00.cfg \
            file://user_2025-07-18-12-56-00.cfg \
            file://user_2025-07-18-14-26-00.cfg \
            file://user_2025-07-18-14-30-00.cfg \
            file://user_2025-07-18-14-35-00.cfg \
            "

