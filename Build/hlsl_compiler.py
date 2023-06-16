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

import csbuild
import os

from csbuild.tools.common.tool_traits import HasDebugLevel, HasOptimizationLevel
from csbuild.tools.common.msvc_tool_base import MsvcToolBase
from csbuild import commands, log
from csbuild._utils import ordered_set, PlatformString
from csbuild._utils.decorators import TypeChecked

from enum import IntEnum, auto

DebugLevel = HasDebugLevel.DebugLevel
OptimizationLevel = HasOptimizationLevel.OptimizationLevel

_REPO_ROOT_PATH = os.path.abspath(f"{os.path.dirname(__file__)}/..")

def _ignore(_):
	pass

class ShaderType(IntEnum):
	Vertex = auto()
	Pixel = auto()
	Geometry = auto()
	Hull = auto()
	Domain = auto()
	Compute = auto()

class HlslCompiler(
	MsvcToolBase,
	HasDebugLevel,
	HasOptimizationLevel
):
	supportedPlatforms = { "Windows" }
	supportedArchitectures = { "x86", "x64", "arm64" }
	inputFiles = { ".hlsl" }
	outputFiles = { ".sbin", ".pdb" }

	def __init__(self, projectSettings):
		MsvcToolBase.__init__(self, projectSettings)
		HasDebugLevel.__init__(self, projectSettings)
		HasOptimizationLevel.__init__(self, projectSettings)

		self._outputPath = None

		self._hlslContext = projectSettings.get("hlslContext", None)
		self._defines = projectSettings.get("hlslDefines", ordered_set.OrderedSet())
		self._includeDirectories = projectSettings.get("hlslIncludeDirectories", ordered_set.OrderedSet())

		self._vsProfile = projectSettings.get("vsProfile", "vs_6_0")
		self._psProfile = projectSettings.get("psProfile", "ps_6_0")
		self._gsProfile = projectSettings.get("gsProfile", "gs_6_0")
		self._hsProfile = projectSettings.get("hsProfile", "hs_6_0")
		self._dsProfile = projectSettings.get("dsProfile", "ds_6_0")
		self._csProfile = projectSettings.get("csProfile", "cs_6_0")

		self._vsEntry = projectSettings.get("vsEntry", "VertexMain")
		self._psEntry = projectSettings.get("psEntry", "PixelMain")
		self._gsEntry = projectSettings.get("gsEntry", "GeometryMain")
		self._hsEntry = projectSettings.get("hsEntry", "HullMain")
		self._dsEntry = projectSettings.get("dsEntry", "DomainMain")
		self._csEntry = projectSettings.get("csEntry", "ComputeMain")

		self._customFlags = projectSettings.get("customFlags", [])

		self._exePath = None
		
	@staticmethod
	@TypeChecked(context=str)
	def SetHlslContext(context):
		csbuild.currentPlan.SetValue("hlslContext", context)

	@staticmethod
	def AddHlslDefines(*defines):
		csbuild.currentPlan.UnionSet("hlslDefines", defines)

	@staticmethod
	def AddHlslIncludeDirectories(*dirs):
		csbuild.currentPlan.UnionSet("hlslIncludeDirectories", [os.path.abspath(d) for d in dirs if d])

	@staticmethod
	@TypeChecked(profile=str)
	def SetVertexShaderProfile(profile):
		csbuild.currentPlan.SetValue("vsProfile", profile)

	@staticmethod
	@TypeChecked(profile=str)
	def SetPixelShaderProfile(profile):
		csbuild.currentPlan.SetValue("psProfile", profile)

	@staticmethod
	@TypeChecked(profile=str)
	def SetGeometryShaderProfile(profile):
		csbuild.currentPlan.SetValue("gsProfile", profile)

	@staticmethod
	@TypeChecked(profile=str)
	def SetHullShaderProfile(profile):
		csbuild.currentPlan.SetValue("hsProfile", profile)

	@staticmethod
	@TypeChecked(profile=str)
	def SetDomainShaderProfile(profile):
		csbuild.currentPlan.SetValue("dsProfile", profile)

	@staticmethod
	@TypeChecked(profile=str)
	def SetComputeShaderProfile(profile):
		csbuild.currentPlan.SetValue("csProfile", profile)

	@staticmethod
	@TypeChecked(entryPoint=str)
	def SetVertexShaderEntryPoint(entryPoint):
		csbuild.currentPlan.SetValue("vsEntry", entryPoint)

	@staticmethod
	@TypeChecked(entryPoint=str)
	def SetPixelShaderEntryPoint(entryPoint):
		csbuild.currentPlan.SetValue("psEntry", entryPoint)

	@staticmethod
	@TypeChecked(entryPoint=str)
	def SetGeometryShaderEntryPoint(entryPoint):
		csbuild.currentPlan.SetValue("gsEntry", entryPoint)

	@staticmethod
	@TypeChecked(entryPoint=str)
	def SetHullShaderEntryPoint(entryPoint):
		csbuild.currentPlan.SetValue("hsEntry", entryPoint)

	@staticmethod
	@TypeChecked(entryPoint=str)
	def SetDomainShaderEntryPoint(entryPoint):
		csbuild.currentPlan.SetValue("dsEntry", entryPoint)

	@staticmethod
	@TypeChecked(entryPoint=str)
	def SetComputeShaderEntryPoint(entryPoint):
		csbuild.currentPlan.SetValue("csEntry", entryPoint)

	def GetHlslIncludeDirectories(self):
		return self._includeDirectories

	def SetupForProject(self, project):
		MsvcToolBase.SetupForProject(self, project)
		HasDebugLevel.SetupForProject(self, project)
		HasOptimizationLevel.SetupForProject(self, project)

		# Construct the output path.
		self._outputPath = os.path.normpath(f"{project.outputDir}/shaders")

		if self._hlslContext:
			self._outputPath = os.path.normpath(f"{self._outputPath}/{self._hlslContext}")

		# Create the output path if it doesn't already exist.
		if not os.access(self._outputPath, os.F_OK):
			os.makedirs(self._outputPath)

		# Reset the DXC path.
		self._exePath = None

		# We can't predict what case the environment variable keys will be in, so we need to loop over each one.
		for key, value in self._vcvarsall.env.items():
			if key.lower() == "path":

				# Find dxc.exe anywhere it's available on the environment path.
				for envPath in [path for path in value.split(";") if path]:
					if os.access(os.path.join(envPath, "dxc.exe"), os.F_OK):
						self._exePath = os.path.normpath(f"{PlatformString(envPath)}/dxc.exe")
						break

				assert self._exePath is not None, \
					"Failed to find path to dxc.exe"

				break

	def Run(self, inputProject, inputFile):
		log.Build(
			"Compiling {} ({}-{}-{})...",
			inputFile,
			inputProject.toolchainName,
			inputProject.architectureName,
			inputProject.targetName
		)

		_, extension = os.path.splitext(inputFile.filename)
		returncode, _, _ = commands.Run(self._getCommand(inputProject, inputFile))
		if returncode != 0:
			raise csbuild.BuildFailureException(inputProject, inputFile)

		return self._getOutputFiles(inputProject, inputFile)

	def _getOutputFiles(self, project, inputFile):
		_ignore(project)

		inputBaseName = os.path.splitext(os.path.basename(inputFile.filename))[0]
		outputPath = os.path.normpath(f"{self._outputPath}/{inputBaseName}")
		outputFiles = [f"{outputPath}.sbin"]

		if self._debugLevel in [DebugLevel.ExternalSymbols, DebugLevel.ExternalSymbolsPlus]:
			outputFiles.append(f"{outputPath}.pdb")

		return tuple(outputFiles)

	def _getCommand(self, project, inputFile):
		shaderType = None

		# Determine the type of the input shader from it's file extension.
		if inputFile.filename.endswith(".vs.hlsl"):     shaderType = ShaderType.Vertex
		elif inputFile.filename.endswith(".ps.hlsl"):    shaderType = ShaderType.Pixel
		elif inputFile.filename.endswith(".gs.hlsl"): shaderType = ShaderType.Geometry
		elif inputFile.filename.endswith(".hs.hlsl"):     shaderType = ShaderType.Hull
		elif inputFile.filename.endswith(".ds.hlsl"):   shaderType = ShaderType.Domain
		elif inputFile.filename.endswith(".cs.hlsl"):  shaderType = ShaderType.Compute

		assert shaderType is not None, \
			f"Unknown shader type for file: {inputFile.filename}"

		cmd = [self._exePath] \
			+ self._getDefaultArgs() \
			+ self._getOptimizationArgs() \
			+ self._getDebugArgs() \
			+ self._getEntryPointArgs(shaderType) \
			+ self._getProfileArgs(shaderType) \
			+ self._getCustomArgs(project) \
			+ self._getPreprocessorArgs() \
			+ self._getIncludeDirectoryArgs() \
			+ self._getOutputFileArgs(project, inputFile) \
			+ [inputFile.filename]

		return cmd

	def _getDefaultArgs(self):
		args = ["-nologo"]
		return args

	def _getOptimizationArgs(self):
		# DXC doesn't really have separate optimization options to select between size and speed,
		# so we hack it here with the normal optimization levels.
		arg = {
			OptimizationLevel.Size:  "-O1",
			OptimizationLevel.Speed: "-O2",
			OptimizationLevel.Max:   "-O3",
		}.get(self._optLevel, "-Od")
		return [arg]

	def _getDebugArgs(self):
		args = {
			DebugLevel.Disabled: ["-Qstrip_debug", "-Qstrip_reflect"],
			DebugLevel.EmbeddedSymbols: ["-Zi", "-Qembed_debug"],
		}.get(self._debugLevel, ["-Zi"])
		return args

	def _getEntryPointArgs(self, shaderType):
		entryPoint = {
			ShaderType.Vertex:   self._vsEntry,
			ShaderType.Pixel:    self._psEntry,
			ShaderType.Geometry: self._gsEntry,
			ShaderType.Hull:     self._hsEntry,
			ShaderType.Domain:   self._dsEntry,
			ShaderType.Compute:  self._csEntry,
		}.get(shaderType)
		return ["-E", entryPoint]

	def _getProfileArgs(self, shaderType):
		profile = {
			ShaderType.Vertex:   self._vsProfile,
			ShaderType.Pixel:    self._psProfile,
			ShaderType.Geometry: self._gsProfile,
			ShaderType.Hull:     self._hsProfile,
			ShaderType.Domain:   self._dsProfile,
			ShaderType.Compute:  self._csProfile,
		}.get(shaderType)
		return ["-T", profile]

	def _getPreprocessorArgs(self):
		args = []
		for define in self._defines:
			args.extend(["-D", define])
		return args

	def _getIncludeDirectoryArgs(self):
		args = []
		for path in self._includeDirectories:
			args.extend(["-I", path])
		return args

	def _getCustomArgs(self, project):
		_ignore(project)
		return self._customFlags

	def _getOutputFileArgs(self, project, inputFile):
		outputFiles = self._getOutputFiles(project, inputFile)

		outputFilePaths = [x for x in outputFiles if x.endswith(".sbin")]
		debugInfoFilePaths = [x for x in outputFiles if x.endswith(".pdb")]

		args = ["-Fo", outputFilePaths[0]]

		if self._debugLevel in [DebugLevel.ExternalSymbols, DebugLevel.ExternalSymbolsPlus]:
			args.extend(["-Fd", debugInfoFilePaths[0]])

		return args
