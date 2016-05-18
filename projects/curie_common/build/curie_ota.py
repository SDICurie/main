#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    Module to generate OTA packages for Curie.

        includes:
            - arc application
            - quark application
            - ble image

        excludes:
            - quark bootloader

        packages:
            - full package with arc, quark and ble
            - incremental package with arc, quark, and ble

        parameters:
            - chunk size: 4K, highest possible value based on available RAM in
            bootloader
"""

from ota import OtaChipPackage
import argparse


class OtaCuriePackage(OtaChipPackage):
    """
        Curie module main class for full package
    """

    def __init__(self, *args, **kw):
        super(OtaCuriePackage, self).__init__(*args, **kw)
        self.ota_binaries.append(
            {"magic": "ARC", "file": "arc.bin", "type": 0})
        self.ota_binaries.append(
            {"magic": "QRK", "file": "quark.signed.bin", "type": 1})
        self.ota_binaries.append(
            {"magic": "BLE", "file": "ble_core/image.bin", "type": 2})


class OtaCuriePackageIncremental(OtaChipPackage):
    """
        Curie module main class for incremental package
    """

    def __init__(self, *args, **kw):
        super(OtaCuriePackageIncremental, self).__init__(*args, **kw)
        self.chunk_size = 4096
        self.ota_binaries.append(
            {"magic": "ARC", "file": "arc.patch", "type": 0})
        self.ota_binaries.append(
            {"magic": "QRK", "file": "quark.signed.patch", "type": 1})
        self.ota_binaries.append(
            {"magic": "BLE", "file": "ble_core_image.patch", "type": 2})


if __name__ == "__main__":
    """
        Main script called from makefile
    """

    parser = argparse.ArgumentParser(description="Curie OTA package tool")
    parser = argparse.ArgumentParser(description="Curie OTA package tool")
    parser.add_argument(
        "-o",
        "--output_directory",
        help="Output directory",
        required=True)
    parser.add_argument(
        "-i",
        "--input_directory",
        help="Input directory",
        required=True)
    parser.add_argument(
        "-p",
        "--package_type",
        help="Input directory",
        required=True)
    args = parser.parse_args()

    if args.package_type == "full":
        # generate full package
        curie = OtaCuriePackage(
            chip="curie",
            platform=0,
            version=1,
            min_version=0,
            app_min_version=0,
            output_directory=args.output_directory,
            input_directory=args.input_directory)
        curie.package(
            out_file="package.ota.bin",
            compression=True,
            description_file="package.json")

    if args.package_type == "incremental":
        # generate incremental package
        curie = OtaCuriePackageIncremental(
            chip="curie",
            platform=0,
            version=1,
            min_version=0,
            app_min_version=0,
            output_directory=args.output_directory,
            input_directory=args.input_directory)
        curie.diff_packages(
            "from_package.ota.bin",
            "to_package.ota.bin")
        curie.package(
            out_file="package_incremental.bin",
            description_file="package_incremental.json")
