SUMMARY = "Touch Screen Support Recipe"
DESCRIPTION = "Recipe for enabling touch screen support on a Yocto-based system"
LICENSE = "CLOSED"

SRC_URI = "file://comms.c \
	   file://debug.c \
	   file://protocol.c\
	   file://services.c \
	   file://services_sc.c \
	   file://sysdata.c \
	   file://usb.c \
	   file://ZyConfigCLI.c \
	   file://firmwareUpdate.c \
	   file://loadZys.c \
	   file://saveZys.c \
	   file://logfile.cpp \
	   file://configfile.cpp \
	   file://keycodes.h \
	   file://debug.h \
	   file://dbg2console.h \
	   file://protocol.h \
	   file://usb.h \
	   file://services.h \
	   file://version.h \
	   file://zxy100.h \
	   file://zxy110.h \
	   file://zxymt.h \
	   file://services_sc.h \
	   file://sysdata.h \
	   file://logfile.h \
	   file://configfile.h \
	   file://zytypes.h \
	   file://Makefile \
	   file://ZXY100_402.31_3059.zyf \
	   file://ZXY100_402.43_5C86.zyf \
	   file://ZXY100_501.36_B97E.zyf \
	   file://ZXY110_001.58_C215.zyf \
	   file://ZXY110_001.83_B199.zyf \
	   file://ZXY150_15.03.63_E9E7.zyf \
	   file://ZXY200_02.02.12_134B.zyf \
	   file://ZXY200_02.03.63_9177.zyf \
	   file://ZXY300_03.02.12_AA23.zyf \
	   file://ZXY300_03.03.63_F086.zyf \
	   file://ZXY500_128w_04.03.28_70FE.zyf \
	   file://ZXY500_128w_04.04.19_653D.zyf \
	   file://ZXY500_256w_04.03.28_688B.zyf \
	   file://ZXY500_256w_04.04.19_5BE8.zyf \
	   file://ZXY500_64w_04.03.28_70FE.zyf \
	   file://ZXY500_64w_04.04.19_653D.zyf \
"

S = "${WORKDIR}"

DEPENDS = "libusb1-native"
DEPENDS += "libusb1"

FILES:${PN} += "${base_libdir}/firmware/*"

do_congiure(){
}

do_compile() {
	${CC} -c comms.c -o comms.o -I${includedir}/libusb-1.0 -Wall -g
	${CC} -c debug.c -o debug.o -I${includedir}/libusb-1.0 -Wall -g
	${CC} -c protocol.c -o protocol.o -I${includedir}/libusb-1.0 -Wall -g
	${CC} -c services.c -o services.o -I${includedir}/libusb-1.0 -Wall -g
	${CC} -c services_sc.c -o services_sc.o -I${includedir}/libusb-1.0 -Wall -g
	${CC} -c sysdata.c -o sysdata.o -I${includedir}/libusb-1.0 -Wall -g
	${CC} -c usb.c -o usb.o -I${includedir}/libusb-1.0 -Wall -g
	${CXX} -c configfile.cpp -o configfile.o -I${includedir}/libusb-1.0 -Wall -g
	${CXX} -c logfile.cpp -o logfile.o -I${includedir}/libusb-1.0 -Wall -g
	${AR} rcs libzylib.a comms.o debug.o protocol.o services.o services_sc.o sysdata.o usb.o configfile.o logfile.o
	${CC} -c ZyConfigCLI.o ZyConfigCLI.c -I*.h -I${includedir}/libusb-1.0 -Wall -g
	${CC} -o ZyConfigCLI ${S}/ZyConfigCLI.o ${S}/libzylib.a -I${includedir}/libusb-1.0 -L{libdir} -lusb-1.0 -Wall -g
	${CC} -c firmwareUpdate.o firmwareUpdate.c -I*.h -I${includedir}/libusb-1.0 -Wall -g
	${CC} -o firmwareUpdate ${S}/firmwareUpdate.o ${S}/libzylib.a -I${includedir}/libusb-1.0 -L{libdir} -lusb-1.0 -Wall -g
	${CC} -c loadZys.o loadZys.c -I*.h -I${includedir}/libusb-1.0 -Wall -g
	${CC} -o loadZys ${S}/loadZys.o ${S}/libzylib.a -I${includedir}/libusb-1.0 -L{libdir} -lusb-1.0 -Wall -g
	${CC} -c saveZys.o saveZys.c -I*.h -I${includedir}/libusb-1.0 -Wall -g
	${CC} -o saveZys ${S}/saveZys.o ${S}/libzylib.a -I${includedir}/libusb-1.0 -L{libdir} -lusb-1.0 -Wall -g
}

do_install() {
        install -d ${D}${bindir}
        install -d ${D}${base_libdir}/firmware
        install -m 0755 ${S}/ZyConfigCLI ${D}${bindir}
        install -m 0755 ${S}/firmwareUpdate ${D}${bindir}
        install -m 0755 ${S}/loadZys ${D}${bindir}
        install -m 0755 ${S}/saveZys ${D}${bindir}
	install -m 0644 ${S}/*.zyf ${D}${base_libdir}/firmware
}

do_package_qa() {
}
