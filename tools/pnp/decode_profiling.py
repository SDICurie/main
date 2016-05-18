#!/usr/bin/env python

# Copyright (c) 2015, Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import sys
import subprocess

parser = argparse.ArgumentParser(description="Decode test command profiling log")
parser.add_argument('firmware_path', help='directory containing ELF binaries')
parser.add_argument('profiling_file', help='log from debug profiling test command')
args = parser.parse_args()

# file contains:
# debug profiling 1 e 0x40002b7e 22
# debug profiling 1 x 0x40002b7e 54
# debug profiling 1 e 0x40002b7e 54
# debug profiling 1 x 0x40002b7e 86
#
tab = open(args.profiling_file,"r").readlines()

def space(depth):
	if depth<0:
		depth = 0
	return "  "*depth

depth = 0
out_data = []
try:
	for elem in tab:
		elem = elem.split()
		if elem[3] == 'e':
			depth = depth + 1
			output = subprocess.check_output(['addr2line', '-f', '-e', args.firmware_path + '/ssbl_quark.elf', elem[4]])
			if "??:0" in output:
				output = subprocess.check_output(['addr2line', '-f', '-e', args.firmware_path + '/quark.elf', elem[4]])
			out_data.append({'depth':depth, 'addr':elem[4], 'func':output.split()[0],'ts':int(elem[5])})
			#print space(depth),output.split()[0]
		elif elem[3] == 'x':
			#revert loop to find enter
			for i, x_elem in enumerate(reversed(out_data)):
				#print "revert ",x_elem , i
				if x_elem['depth'] == depth and x_elem['addr'] == elem[4]:
					out_data[-1-i]['duration'] = int(elem[5]) - x_elem['ts']
					break
			depth = depth - 1
except:
	depth = 0

for elem in out_data:
	try:
		if not 'duration' in elem:
			elem['duration']= "oo"
		print space(elem['depth']), elem['func'], elem['duration']
	except:
		depth = 0

