# This is .bbappend file for linux-toradex-mainline

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-uart-kernel-change.patch"
SRC_URI += "file://0001-second-ethernet-support.patch"
SRC_URI += "file://0001-additional-uart-kernel-change.patch"
SRC_URI += "file://0001-uart-serial-kernel-updates.patch"
SRC_URI += "file://0002-ethernet2_additional_changes.patch"
SRC_URI += "file://0002_eth2_additional_changes_2.patch"
SRC_URI += "file://0001-sd-card-non-removable.patch"
SRC_URI += "file://0001-micro-SD-card-enable.patch"
