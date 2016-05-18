#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    Module to generate OTA packages.

    3 types of packages are supported:
        - raw full, which contains all binaries as raw
        - compressed full, which contains all binaries as compressed
        - incremental, which contains compressed binary patch


    JSON file contains package description and shall be used by phone or cloud
    applications

    HTML file compares compression ratio between the different types of package

"""

import ctypes
import json
import os
from lzg import LZG
from bsdiff_chunk import BsdiffChunk
from string import Template




class OtaHeader(ctypes.Structure):
    """
        OTA header description

            This definition shall match bootloader's one.

            This shall not be used by phone or cloud applications (JSON file
            shall be used instead)
    """
    _pack_ = 1

    _fields_ = [
        ("magic", ctypes.c_char * 3),
        ("header_version", ctypes.c_byte),
        ("header_length", ctypes.c_int16),
        ("platform", ctypes.c_int16),
        ("crc", ctypes.c_int32),
        ("payload_length", ctypes.c_int32),
        # version of the current package
        ("version", ctypes.c_int32),
        # mininimum version on which it can be applied
        ("min_version", ctypes.c_int32),
        # minimum version of the phone application
        ("app_min_version", ctypes.c_int32)
    ]


class OtaBinary(ctypes.Structure):
    """
        OTA binary header

            This definition shall match bootloader's one.

            This shall not be used by phone or cloud applications (JSON file
            shall be used instead)

    """

    _pack_ = 1

    _fields_ = [
        ("magic", ctypes.c_char * 3),
        ("type", ctypes.c_byte),
        ("version", ctypes.c_int32),
        ("offset", ctypes.c_int32),
        ("length", ctypes.c_int32)
    ]

def _pack(ctype_instance):
    """
        Returns ctype buffer from ctype_instance input parameter
    """

    buf = ctypes.string_at(
        ctypes.byref(ctype_instance),
        ctypes.sizeof(ctype_instance))
    return buf


class OtaPackage(object):
    """
        Base class to create an OTA package
    """

    def __init__(self, platform=0x0000, header_version=1, version=0,
                 min_version=0, app_min_version=0, output_directory="",
                 input_directory=""):
        self.ota_header = OtaHeader("OTA", header_version, ctypes.sizeof(
            OtaHeader), platform, 0, 0, version, min_version, app_min_version)
        self.ota_binaries = list()
        self.description = dict()
        self.board = None
        self.chip = None
        self.project = None
        self.ota_incremental = False
        self.chunk_size = 0
        self.output_directory = output_directory + "/"
        self.input_directory = input_directory + "/"

    def _sum_patch(self, key):
        """
            Returns sum value on key from a patch.
            patch json format is coming from BsdiffChunk class.
        """

        value = 0
        for binary in self.ota_binaries:
            value += binary["patch"][key]
        return value

    def _html_charts(self, html_data, description_file):
        """
            Writes description.html to output_directory with charts to compare
            compression ratio on raw, compressed and incremental binaries.
        """

        html_output = Template("""
            <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
            <div id="chart_div"></div>
            <script>
            google.charts.load('current', {packages: ['corechart', 'bar']});
            google.charts.setOnLoadCallback(drawBasic);
            function drawBasic() {
            var data = google.visualization.arrayToDataTable([
                    ${data}
                ]);
                var options = {
                    title: 'Compression rate',
                    chartArea: {width: '50%'},
                    hAxis: {
                    title: 'Size (bytes)',
                    minValue: 0
                    },
                    vAxis: {
                    title: 'Files'
                    }
                };
                var chart = new google.visualization.BarChart(document.getElementById('chart_div'));
                chart.draw(data, options);
                }
            </script>
            <a href='${json_file}'>description</a>
        """)

        open(self.output_directory + "description.html",
             "w").write(html_output.substitute({"data": html_data,
                                                "json_file": description_file}))

    def _package_check(self, out_file):
        out_file_size = os.stat(self.output_directory + out_file).st_size
        file_size = self.ota_header.header_length + \
            self.description["metrics"]["size"]

        print "check_package: %d == %d" % (out_file_size, file_size)


    def package(self, out_file, compression=True, description_file=None):
        """
            Generate a package
        """

        offset = 0
        payload_length = 0
        payload_original_length = 0
        headers = list()
        self.ota_header.header_length += len(self.ota_binaries) * \
            ctypes.sizeof(OtaBinary)

        if self.board is None:
            self.board = "%s-all-boards" % (self.chip)

        if self.project is None:
            self.project = "%s-all-projects" % (self.board)

        self.description.update({"project": self.project})
        self.description.update({"board": self.board})
        assert self.chip is not None
        self.description.update({"chip": self.chip})
        self.description.update({"incremental": self.ota_incremental})

        print "writing to " + self.output_directory + out_file
        out_fp = open(self.output_directory + out_file, "wb")
        out_fp.seek(self.ota_header.header_length, 0)

        if compression is True and self.ota_incremental is True:
            html_chart = "['File', 'Raw', 'Compressed', 'Patch'],"
        else:
            if compression is True:
                html_chart = "['File', 'Raw', 'Compressed'],"
            else:
                html_chart = "['File'],"

        for binary in self.ota_binaries:

            binary.update({
                "version": 0,
                "offset": 0,
                "length": 0,
                "length_compressed": 0,
                "ratio": 0
            })

            in_file = open(self.input_directory + binary["file"]).read()
            binary["length"] = len(in_file)
            binary["offset"] = offset
            binary["version"] = 0

            if compression is True and self.ota_incremental is False:
                in_file_compressed = LZG().compress(in_file)
                binary["length_compressed"] = len(in_file_compressed)
                binary["ratio"] = "%0.5f" % (
                    1.0 * binary["length_compressed"] / binary["length"])

                header = OtaBinary(
                    binary["magic"], int(
                        binary["type"]), int(
                        binary["version"]), int(
                        binary["offset"]), int(
                        binary["length_compressed"]))
            else:
                header = OtaBinary(
                    binary["magic"], int(
                        binary["type"]), int(
                        binary["version"]), int(
                        binary["offset"]), int(
                        binary["length"]))

                if self.ota_incremental is True:
                    binary["length"] = binary["patch"]["size_original"]
                    binary["length_compressed"] = binary["patch"]["size"]

            payload_original_length += binary["length"]

            headers.append(header)

            if compression is True and self.ota_incremental is False:
                offset += binary["length_compressed"]
                payload_length += binary["length_compressed"]
                out_fp.write(in_file_compressed)
                html_chart += "['%s', %d, %d]," % (binary["magic"],
                                                   binary["length"],
                                                   binary["length_compressed"])
            else:
                if self.ota_incremental is True:
                    binary["length"] = binary["patch"]["size_original"]
                    binary["length_compressed"] = binary["patch"]["size"]
                    offset += binary["length_compressed"]
                    payload_length += binary["length_compressed"]
                    out_fp.write(in_file)
                    html_chart += "['%s', %d, %d, %d]," % (binary["magic"],
                                             binary["patch"]["size_original"],
                                             binary["patch"]["size_compressed"],
                                             binary["patch"]["size"])
                else:
                    offset += binary["length"]
                    payload_length += binary["length"]
                    out_fp.write(in_file)
                    html_chart += "['%s', %d]," % (binary["magic"],
                                                   binary["length"])

        self.ota_header.payload_length = payload_length

        out_fp.seek(0, 0)
        out_fp.write(_pack(self.ota_header))
        for header in headers:
            out_fp.write(_pack(header))
        out_fp.close()

        if description_file is not None:
            self.description.update(
                {"header":
                 {"header_version": self.ota_header.header_version,
                  "header_length": self.ota_header.header_length,
                  "platform": self.ota_header.platform,
                  "crc": self.ota_header.crc,
                  "payload_length": self.ota_header.payload_length,
                  "payload_original_length": payload_original_length,
                  "ratio": "%0.5f" % (1.0 * self.ota_header.payload_length /
                                      payload_original_length),
                  "version": self.ota_header.version,
                  "min_version": self.ota_header.min_version,
                  "app_min_version": self.ota_header.app_min_version,
                  },
                 "binaries": self.ota_binaries
                 }
            )

            if compression is True and self.ota_incremental is False:
                html_chart += "['%s', %d, %d]," % ("package",
                                                   payload_original_length,
                                                   payload_length)
                self.description["metrics"] = {
                        "gain_vs_compressed": 0,
                        "gain_vs_original":
                        self.description["header"]["payload_original_length"] -
                    self.description["header"]["payload_length"],
                        "size": self.description["header"]["payload_length"],
                        "size_compressed":
                    self.description["header"]["payload_length"],
                        "size_original":
                    self.description["header"]["payload_original_length"]}

            else:
                if self.ota_incremental is True:
                    self.description["metrics"] = {
                        "gain_vs_compressed":
                        self._sum_patch("gain_vs_compressed"),
                        "gain_vs_original":
                        self._sum_patch("gain_vs_original"),
                        "size": self._sum_patch("size"),
                        "size_compressed": self._sum_patch("size_compressed"),
                        "size_original": self._sum_patch("size_original")}
                    html_chart += "['%s', %d, %d, %d]," %("package",
                                            self._sum_patch("size_original"),
                                            self._sum_patch("size_compressed"),
                                            self._sum_patch("size"))
                else:
                    self.description["metrics"] = {
                        "gain_vs_compressed": 0,
                        "gain_vs_original": 0,
                        "size": self.description["header"]["payload_length"],
                        "size_compressed":
                        self.description["header"]["payload_length"],
                        "size_original":
                        self.description["header"]["payload_original_length"]}

                    html_chart += "['%s', %d]," % ("package", payload_length)

            json_description = {"package": self.description}
            open(
                self.output_directory +
                description_file,
                "w").write(
                json.dumps(
                    json_description,
                    indent=4,
                    sort_keys=True))

            self._html_charts(html_chart, description_file)
            self._package_check(out_file)

    def get_binaries_from_package(self, in_file):
        """
            Parse OTA package and returns content.

            Used to generate binary patch between two existing full package
        """

        in_data = open(self.input_directory + in_file).read()
        ota_header = (OtaHeader).from_buffer_copy(in_data[0:ctypes.sizeof(OtaHeader)])
        binaries_header_size = ota_header.header_length - \
            ctypes.sizeof(OtaHeader)
        binaries_count = binaries_header_size / ctypes.sizeof(OtaBinary)
        binaries_list = dict()

        offset = ctypes.sizeof(OtaHeader)
        for i in range(0, binaries_count):
            binary_header = (OtaBinary).from_buffer_copy(
                in_data[offset:offset + ctypes.sizeof(OtaBinary)])

            binary_start_offset = ota_header.header_length + \
                                                        binary_header.offset
            binary_end_offset = binary_start_offset + binary_header.length

            print "Found %s, size = %d, offset = %d (copy from %d to %d) (%d)" \
                % (binary_header.magic, binary_header.length,
                   binary_header.offset, binary_start_offset, binary_end_offset,
                   i)

            binaries_list[binary_header.magic] = {
                "data": in_data[binary_start_offset:binary_end_offset]}

            offset += ctypes.sizeof(OtaBinary)
        return binaries_list

    def diff_packages(self, from_file, to_file):
        """
            Generate binary patch between 2 full OTA packages.
        """

        self.ota_incremental = True
        assert self.chunk_size != 0

        from_binaries = self.get_binaries_from_package(in_file=from_file)
        to_binaries = self.get_binaries_from_package(in_file=to_file)

        for binary in self.ota_binaries:

            from_name = "%s/%s.from" % (self.input_directory, binary["file"])
            to_name = "%s/%s.to" % (self.input_directory, binary["file"])
            json_name = "%s.json" % binary["file"]

            from_data = from_binaries.get(binary["magic"])["data"]
            to_data = to_binaries.get(binary["magic"])["data"]

            from_deflate_data = LZG().decompress(from_data)
            to_deflate_data = LZG().decompress(to_data)

            open(from_name, "wb").write(from_deflate_data)
            open(to_name, "wb").write(to_deflate_data)

            try:
                os.mkdir(self.input_directory + "tmp/")
            except OSError:
                pass

            chunk = BsdiffChunk(
                chunk_size=self.chunk_size,
                temp_directory=self.input_directory + "tmp/",
                verbose=False)
            use_patch, patch_information = chunk.diff(from_name, to_name,
                self.input_directory + binary["file"], json_name)

            binary.update({"patch": patch_information})


class OtaChipPackage(OtaPackage):
    """
        Chip (as SoC) specific package class.

        It shall provide reference/base implementation for a chip:
            - binaries definition
            - chip name
    """

    def __init__(self, chip="", *args, **kw):
        super(OtaChipPackage, self).__init__(*args, **kw)
        self.chip = chip


class OtaBoardPackage(OtaChipPackage):
    """
        Board specific package class.

        It shall provide implementation for a board:
            - binaries definition (i.e storage outside the chip: snor, mmc)
            - board name

        A board shall always inherit from a Chip to avoid duplication on
        binaries definition.
    """

    def __init__(self, board="", *args, **kw):
        super(OtaBoardPackage, self).__init__(*args, **kw)
        self.board = board


class OtaProjectPackage(OtaBoardPackage):
    """
        Project specific package class.

        It shall provide implementation for project:
            - binaries definition that are project specific based on the same
            chip or board
    """

    def __init__(self, project="", *args, **kw):
        super(OtaProjectPackage, self).__init__(*args, **kw)
        self.project = project
