SUMMARY = "Proceso central persistente de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "aurabot-api pwm libgpio sensors leds"
RDEPENDS:${PN} = "aurabot-api aurabot-audio-server"
SRC_URI = " \
    file://CMakeLists.txt \
    file://src/main.c \
    file://src/aurabot_state.h \
    file://src/aurabot_config.h \
    file://src/fsm.c \
    file://src/fsm.h \
    file://src/autonomous.c \
    file://src/autonomous.h \
    file://src/control_lease.c \
    file://src/control_lease.h \
    file://src/motion.c \
    file://src/motion.h \
    file://src/odometry.c \
    file://src/odometry.h \
    file://src/state_outputs.c \
    file://src/state_outputs.h \
    file://src/audio_controller.c \
    file://src/audio_controller.h \
    file://src/robot_controller.c \
    file://src/robot_controller.h \
    file://src/angle_math.c \
    file://src/angle_math.h \
    file://src/time_utils.c \
    file://src/time_utils.h \
    file://src/map.c \
    file://src/map.h \
    file://src/ipc_server.c \
    file://src/ipc_server.h \
    file://src/hardware.c \
    file://src/hardware.h \
    file://src/hardware_config.h \
    file://tests/fake_hardware.c \
    file://tests/fake_hardware.h \
    file://tests/test_fsm.c \
    file://tests/test_core.c \
    file://tests/test_map.c \
    file://tests/test_partial_hardware.c \
    file://aurabot.init \
    file://aurabot-supervisor \
"
S = "${WORKDIR}"

inherit cmake update-rc.d

INITSCRIPT_NAME = "aurabot"
INITSCRIPT_PARAMS = "defaults 75"

do_install:append() {
    install -d ${D}${sysconfdir}/init.d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/aurabot.init \
        ${D}${sysconfdir}/init.d/aurabot
    install -m 0755 ${WORKDIR}/aurabot-supervisor \
        ${D}${sbindir}/aurabot-supervisor
}

FILES:${PN} += "${sysconfdir}/init.d/aurabot ${sbindir}/aurabot-supervisor"
