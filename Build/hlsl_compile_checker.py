#
# Copyright (c) 2023, Zoe J. Bare
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#

from __future__ import unicode_literals, division, print_function

import os
import re

from csbuild import log, perf_timer
from csbuild._utils import shared_globals
from csbuild.toolchain import CompileChecker

_includeRegex = re.compile(R'^\s*#\s*include\s+["<](\S+)[">]', re.M)

class HlslCompileChecker(CompileChecker):
	"""
	CompileChecker for C++ files that knows how to get C++ file dependency lists.
	"""
	def __init__(self, compiler):
		CompileChecker.__init__(self)
		self._compiler = compiler

	def GetDependencies(self, buildProject, inputFile):
		with perf_timer.PerfTimer("HLSL header dependency resolution"):
			log.Info("Checking header dependencies for {}", inputFile)

			cache = shared_globals.settings.Get("hlslHeaderCache", {})
			mtime = os.path.getmtime(inputFile)
			if inputFile in cache:
				if mtime <= cache[inputFile]["mtime"]:
					return cache[inputFile]["result"]

			with open(inputFile, "rb") as f:
				contents = f.read()
				contents = contents.decode("utf-8", "replace")

			ret = set()

			includeDirs = [os.path.dirname(inputFile)] + list(buildProject.toolchain.Tool(self._compiler).GetHlslIncludeDirectories())
			for header in _includeRegex.findall(contents):
				for includeDir in includeDirs:
					maybeHeaderLoc = os.path.join(includeDir, header)
					if os.access(maybeHeaderLoc, os.F_OK) and not os.path.isdir(maybeHeaderLoc):
						ret.add(os.path.normpath(maybeHeaderLoc))

			cache[inputFile] = {"mtime": mtime, "result": ret}
			return ret