#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
    Module to generate LZG compressed BSDIFF patch from 2 binaries

    Each binary is split in chunk of specified size and a binary patch is
    computed for each chunk.

    Each patch contains a chunk header.
"""
import mmap
import ctypes
import os
import sys
import argparse
import binascii
import multiprocessing
import traceback
import json
import uuid
from itertools import islice
from hashlib import sha256
from multiprocessing.dummy import Pool as ThreadPool
from lzg import LZG
from minibsdiff import MBSDIFF

CHUNK_HEADER_MAGIC = "C!K$"
CHUNK_HEADER_VERSION = 0x01
CHUNK_HEADER_SIZE = 32

CHUNK_TYPE_KEEP = 1
CHUNK_TYPE_COMPRESSED = 2
CHUNK_TYPE_COMPRESSED_PATCH = 3


class ChunkHeader(ctypes.Structure):
    """
        Chunk header with C!K$ magic
    """
    _fields_ = [
        ("magic", ctypes.c_char * 4),
        ("version", ctypes.c_byte),
        ("type", ctypes.c_byte),
        ("id", ctypes.c_int16),
        ("size", ctypes.c_int32),
        ("crc", ctypes.c_int32),
        ("from_len", ctypes.c_int32),
        ("from_crc", ctypes.c_int32),
        ("to_len", ctypes.c_int32),
        ("to_crc", ctypes.c_int32),
    ]


class BsdiffChunk:
    """
        Main class to generate binary patch from chunks
    """

    def __init__(
            self,
            chunk_size=4096,
            temp_directory="./tmp/",
            threads=0,
            remove_identical=True,
            verbose=False,
            package_info=True):
        self.chunk_size = chunk_size
        self.temp_directory = temp_directory
        if threads == 0:
            self.threads = multiprocessing.cpu_count()
        else:
            self.threads = threads
        self.verbose = verbose
        self.remove_identical = remove_identical
        self.package_info = package_info

    def _chunk(self, it):
        """
            Returns an iterator on chunks from input data
        """
        it = iter(it)
        return iter(lambda: tuple(islice(it, self.chunk_size)), ())

    def _process_chunk(
            self,
            from_buffer,
            to_buffer,
            chunk_buffer,
            output_file,
            id,
            chunk_type):
        """
            Method called on each chunk
            Returns a dict with meta-data on chunk
        """

        if chunk_type > CHUNK_TYPE_KEEP:
            chunk_len = len(chunk_buffer)
        else:
            chunk_len = 0

        from_crc = binascii.crc32(from_buffer)
        to_crc = binascii.crc32(to_buffer)

        chunkheader = ChunkHeader(CHUNK_HEADER_MAGIC, CHUNK_HEADER_VERSION,
                                  chunk_type, id, chunk_len, 0, len(from_buffer),
                                  from_crc, len(to_buffer), to_crc)

        chunkheader.crc = binascii.crc32(bytearray(chunkheader))

        arr = bytearray(chunkheader)
        assert len(arr) == CHUNK_HEADER_SIZE
        chunkfile = open(output_file, "wb")
        chunkfile.write(arr)
        if chunk_type > CHUNK_TYPE_KEEP:
            chunkfile.write(chunk_buffer)
        chunkfile.close()

        chunk_len += CHUNK_HEADER_SIZE

        return {
            "_id": id,
            "_type": chunk_type,
            "_name": os.path.basename(output_file),
            "crc_from": "0x%x" %
            (from_crc & 0xffffffff),
            "crc_to": "0x%x" %
            (to_crc & 0xffffffff),
            "crc_header": "0x%x" %
            (chunkheader.crc & 0xffffffff),
            "chunk_header": "0x%x" %
            (binascii.crc32(
                open(
                    output_file,
                    "rb").read()) & 0xffffffff),
            "size_output": chunk_len,
            "size_input": len(to_buffer),
            "ratio": "%0.5f" %
            (1.0 *
             chunk_len /
             len(to_buffer)),
            "delta": chunk_len -
            len(to_buffer)}

    def _diff_with_exception(self, args):
        """
            Wrapper around diff function to get backtrace when it fails
            (workaround multiprocessing behaviour)
        """

        try:
            return self._diff_chunk(args)
        except:
            raise Exception(
                "".join(
                    traceback.format_exception(
                        *sys.exc_info())))

    def _diff_chunk(self, args):
        """
            Generates binary patch on 2 chunks of data.
            Returns compressed chunk when binary patch size is bigger than
            compressed chunk.
            Returns an empty chunk when the 2 chunks are similar.
        """

        o, t, x = args

        assert len(o) <= self.chunk_size
        assert len(t) <= self.chunk_size

        c_name = "%s/%s.%0000d" % (self.temp_directory, uuid.uuid4(), x)

        if o == t:
            open(c_name, "wb").close()
            return self._process_chunk(bytearray(o), bytearray(
                t), bytearray(t), c_name, x, CHUNK_TYPE_KEEP)
        else:
            old_buffer = (
                ctypes.c_ubyte *
                len(o)).from_buffer_copy(
                bytearray(o))
            to_buffer = (
                ctypes.c_ubyte *
                len(t)).from_buffer_copy(
                bytearray(t))

            patch = MBSDIFF().bsdiff(old_buffer, to_buffer)
            assert len(patch) > 0
            to_buffer_compressed = LZG().compress(to_buffer)
            patch_compressed = LZG().compress(patch)

            if len(to_buffer_compressed) <= len(patch_compressed):
                return self._process_chunk(bytearray(o), bytearray(t),
                                           to_buffer_compressed, c_name, x,
                                           CHUNK_TYPE_COMPRESSED)
            else:
                return self._process_chunk(bytearray(o), bytearray(t),
                                           patch_compressed, c_name, x,
                                           CHUNK_TYPE_COMPRESSED_PATCH)

    def diff(self, from_file, to_file, out_file, log_file):
        """
            Split binaries in chunks, diff each chunk and generate a file with
            all chunks (binary patch, compressed or empty)
        """

        out_dir = os.path.dirname(out_file)

        try:
            os.stat(out_dir)
        except:
            os.mkdir(out_dir)

        with open(to_file, 'rb') as t, open(from_file, 'rb') as f:

            from_mmap = mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ)
            to_mmap = mmap.mmap(t.fileno(), 0, prot=mmap.PROT_READ)

            from_len = from_mmap.size()
            assert from_len != 0

            to_len = to_mmap.size()
            assert to_len != 0

            from_hash = sha256(from_mmap).digest()
            to_hash = sha256(to_mmap).digest()

            assert from_hash != to_hash

            from_list = list(self._chunk(from_mmap))
            to_list = list(self._chunk(to_mmap))

            if self.verbose:
                print "Using %d threads" % (self.threads)

            pool = ThreadPool(multiprocessing.cpu_count())

            oss = list()
            tss = list()
            xxx = list()

            for x in range(0, len(to_list)):

                try:
                    o = from_list[x]
                except IndexError:
                    o = list()

                try:
                    t = to_list[x]
                except IndexError:
                    t = list()

                oss.append(o)
                tss.append(t)
                xxx.append(x)

            results = pool.map(self._diff_with_exception, zip(oss, tss, xxx))
            pool.close()
            pool.join()

            out_fd = open(out_file, "wb")
            total_bytes = 0

            patch_information = {}
            patch_information["chunks"] = {}

            for result in results:
                patch_information["chunks"]["%d" % result.get("_id")] = result
                if result["_type"]> CHUNK_TYPE_KEEP:
                    in_fd = open("%s/%s" %
                                 (self.temp_directory, result['_name']), "rb")
                    in_buffer = mmap.mmap(
                        in_fd.fileno(), 0, prot=mmap.PROT_READ)
                    total_bytes += result['size_output']
                    out_fd.write(in_buffer)
                    in_buffer.close()
                    in_fd.close()

            to_file_data = open(to_file, "rb").read()
            to_file_len = len(to_file_data)
            to_file_compressed_len = len(LZG().compress(to_file_data))

            to_file_crc = binascii.crc32(to_file_data)

            patch_information.update(
                {
                    "chunk_size": self.chunk_size,
                    "_name": os.path.basename(out_file),
                    "size": total_bytes,
                    "size_patch": total_bytes,
                    "size_compressed": to_file_compressed_len,
                    "size_original": to_file_len,
                    "crc": "0x%x" %
                    (to_file_crc & 0xffffffff),
                    "gain_vs_original": to_file_len -
                    total_bytes,
                    "gain_vs_compressed": to_file_compressed_len -
                    total_bytes,
                    "ratio_vs_original": "%0.5f" %
                    (1.0 *
                     total_bytes /
                     to_file_len),
                    "ratio_vs_compressed": "%0.5f" %
                    (1.0 *
                     total_bytes /
                     to_file_compressed_len),
                })


            if to_file_compressed_len <= total_bytes:
                patch_information["_name"] = os.path.basename(to_file)
                patch_information["size"] = to_file_compressed_len
                patch_information["gain_vs_original"] = to_file_len - to_file_compressed_len
                patch_information["gain_vs_compressed"] = 0
                patch_information["ratio_vs_original"] = "%0.5f" % \
                    (1.0 *
                     to_file_compressed_len /
                     to_file_len)
                patch_information["ratio_vs_compressed"] = 1
                use_patch = False
            else:
                use_patch = True

            with open(log_file, 'w') as f:
                f.write(
                    json.dumps(
                        patch_information,
                        indent=4,
                        sort_keys=True))

            if self.verbose:
                print json.dumps(patch_information, indent=4, sort_keys=True)

            return use_patch, patch_information

def main(argv):

    parser = argparse.ArgumentParser(description='tool to generate chunk')
    parser.add_argument('-f', '--from_file', help='from', required=True)
    parser.add_argument('-t', '--to_file', help='to', required=True)
    parser.add_argument('-o', '--out_file', help='out file', required=True)
    parser.add_argument(
        '-s',
        '--chunk_size',
        help='chunk size',
        required=False,
        default=4096,
        type=int)
    parser.add_argument(
        '-v',
        '--verbose',
        help='verbose',
        required=False,
        default=False,
        type=bool)
    parser.add_argument('-p', '--temp_dir', help='to', required=True)
    parser.add_argument(
        '-j',
        '--jobs',
        help='jobs',
        required=False,
        default=0,
        type=int)
    parser.add_argument('-l', '--log_file', help='log file', required=True)
    args = parser.parse_args()

    chunk = BsdiffChunk(
        chunk_size=args.chunk_size,
        temp_directory=args.temp_dir,
        verbose=args.verbose,
        threads=args.jobs)
    chunk.diff(args.from_file, args.to_file, args.out_file, args.log_file)

if __name__ == "__main__":
    main(sys.argv[1:])
