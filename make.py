#!/usr/bin/env python3
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
import shutil
import stat
import sys

from csbuild.tools.project_generators import visual_studio
from csbuild.tools.assemblers import AsmCompileChecker
from csbuild.tools.cpp_compilers import CppCompileChecker
from csbuild.tools import MsvcCppCompiler, MsvcLinker, MsvcAssembler

###################################################################################################

_REPO_ROOT_PATH = os.path.dirname(os.path.abspath(__file__))
_POST_BUILD_HOOK = "onBuildFinishedHook"

###################################################################################################

# Add the build support path so we have access to the HLSL compiler tool.
sys.path.insert(0, os.path.normpath(f"{_REPO_ROOT_PATH}/Build"))

###################################################################################################

# Disabling file type folders will put all source files under the the same respective directories in
# the generated project files, so for instance, there will be no separation between .cpp and .hpp files.
# This is useful for development that touches many files, but it can make the filters in each project
# look somewhat less organized.
visual_studio.SetEnableFileTypeFolders(False)

from hlsl_compile_checker import HlslCompileChecker
from hlsl_compiler import HlslCompiler

def _createCheckers(inputMappings):
	checkers = {}

	for checkerObj, extensions in inputMappings.items():
		for ext in extensions:
			checkers[ext] = checkerObj

	return checkers

checkers = _createCheckers({
	HlslCompileChecker(HlslCompiler):   HlslCompiler.inputFiles,
	CppCompileChecker(MsvcCppCompiler): MsvcCppCompiler.inputFiles,
	AsmCompileChecker(MsvcAssembler):   MsvcAssembler.inputFiles,
})

# Override the MSVC toolchain so we can add the HLSL compiler to its list of tools.
csbuild.RegisterToolchain(
	"msvc",
	csbuild.GetSystemArchitecture(),
	MsvcCppCompiler,
	MsvcLinker,
	MsvcAssembler,
	HlslCompiler,
	checkers=checkers
)

###################################################################################################

outputSubPath = "{userData.outputRootName}/{targetName}"

csbuild.SetUserData("outputRootName", "{architectureName}")
csbuild.SetOutputDirectory(f"_out/{outputSubPath}")
csbuild.SetIntermediateDirectory(f"_int/{outputSubPath}")

###################################################################################################

with csbuild.Target("debug"):
	csbuild.AddDefines("_DEBUG", "_DF_CONFIG_DEBUG")
	csbuild.SetDebugLevel(csbuild.DebugLevel.ExternalSymbolsPlus)
	csbuild.SetOptimizationLevel(csbuild.OptimizationLevel.Disabled)
	csbuild.SetDebugRuntime(True)

with csbuild.Target("fastdebug"):
	csbuild.AddDefines("_DEBUG", "_DF_CONFIG_FASTDEBUG")
	csbuild.SetDebugLevel(csbuild.DebugLevel.ExternalSymbolsPlus)
	csbuild.SetOptimizationLevel(csbuild.OptimizationLevel.Max)
	csbuild.SetDebugRuntime(True)

with csbuild.Target("release"):
	csbuild.AddDefines("NDEBUG", "_DF_CONFIG_RELEASE")
	csbuild.SetDebugLevel(csbuild.DebugLevel.Disabled)
	csbuild.SetOptimizationLevel(csbuild.OptimizationLevel.Max)
	csbuild.SetDebugRuntime(False)

###################################################################################################

with csbuild.Toolchain("msvc"):
	csbuild.SetCcLanguageStandard("c99")
	csbuild.SetCxxLanguageStandard("c++17")

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddDefines(
			"__cplusplus=201703L",
			"_HAS_CXX17=1",
		)

	csbuild.SetVisualStudioVersion("17") # Visual Studio 2022
	csbuild.AddDefines(
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_NONSTDC_NO_WARNINGS",
	)
	csbuild.AddCompilerCxxFlags(
		"/bigobj",
		"/permissive-",
		"/Zc:__cplusplus",
		"/EHsc",
		"/W4",
	)

########################################################################################################################

@csbuild.OnBuildFinished
def onGlobalPostBuild(projects):
	if csbuild.GetRunMode() == csbuild.RunMode.Normal:
		for project in projects:
			if _POST_BUILD_HOOK in project.userData:
				project.userData.onBuildFinishedHook(project)

def copyAssetFile(inputFilePath, outputRootPath):
	inputFilePath = os.path.normpath(inputFilePath)
	outputRootPath = os.path.normpath(outputRootPath)

	if not os.access(outputRootPath, os.F_OK):
		os.makedirs(outputRootPath)

	outputFilePath = os.path.join(outputRootPath, os.path.basename(inputFilePath))
	if os.access(outputFilePath, os.F_OK):
		inputFileStat = os.stat(inputFilePath)
		outputFileStat = os.stat(outputFilePath)

		if inputFileStat.st_mtime <= outputFileStat.st_mtime:
			return

	csbuild.log.Build(f"Copying file to output: {inputFilePath}")
	shutil.copy(inputFilePath, outputFilePath)

###################################################################################################

class ExtLibDirectXMath(object):
	projectName = "Ext_DirectXMath"
	outputName = "libdirectxmath"
	path = "External/DirectXMath"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibDirectXMath.projectName, ExtLibDirectXMath.path, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibDirectXMath.outputName, csbuild.ProjectType.StaticLibrary)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceFiles(
			f"{ExtLibDirectXMath.path}/**/*.cpp",
			f"{ExtLibDirectXMath.path}/**/*.h",
			f"{ExtLibDirectXMath.path}/**/*.inl",
		)

	else:
		csbuild.AddSourceDirectories(
			f"{ExtLibDirectXMath.path}/Extensions",
			f"{ExtLibDirectXMath.path}/Inc",
			f"{ExtLibDirectXMath.path}/MatrixStack",
			f"{ExtLibDirectXMath.path}/SHMath",
			f"{ExtLibDirectXMath.path}/Stereo3D",
			f"{ExtLibDirectXMath.path}/XDSP",
		)

	with csbuild.Scope(csbuild.ScopeDef.All):
		csbuild.AddIncludeDirectories(
			f"{ExtLibDirectXMath.path}/Extensions",
			f"{ExtLibDirectXMath.path}/Inc",
			f"{ExtLibDirectXMath.path}/MatrixStack",
			f"{ExtLibDirectXMath.path}/SHMath",
			f"{ExtLibDirectXMath.path}/Stereo3D",
			f"{ExtLibDirectXMath.path}/XDSP",
		)

###################################################################################################

class ExtLibDirectXTex(object):
	projectName = "Ext_DirectXTex"
	outputName = "libdirectxtex"
	path = "External/DirectXTex/DirectXTex"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibDirectXTex.projectName, ExtLibDirectXTex.path, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibDirectXTex.outputName, csbuild.ProjectType.StaticLibrary)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceFiles(
			f"{ExtLibDirectXTex.path}/*.cpp",
			f"{ExtLibDirectXTex.path}/*.h"
		)

	else:
		csbuild.AddSourceFiles(
			f"{ExtLibDirectXTex.path}/BC.cpp",
			f"{ExtLibDirectXTex.path}/BC4BC5.cpp",
			f"{ExtLibDirectXTex.path}/BC6HBC7.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexCompress.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexConvert.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexD3D12.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexDDS.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexFlipRotate.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexHDR.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexImage.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexMipmaps.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexMisc.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexNormalMaps.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexPMAlpha.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexResize.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexTGA.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexUtil.cpp",
			f"{ExtLibDirectXTex.path}/DirectXTexWIC.cpp",
		)

	with csbuild.Scope(csbuild.ScopeDef.All):
		csbuild.AddIncludeDirectories(ExtLibDirectXTex.path)

###################################################################################################

class ExtLibImgui(object):
	projectName = "Ext_Imgui"
	outputName = "libimgui"
	path = "External/imgui"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibImgui.projectName, ExtLibImgui.path, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibImgui.outputName, csbuild.ProjectType.StaticLibrary)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceDirectories(ExtLibImgui.path)

	else:
		csbuild.AddSourceFiles(
			f"{ExtLibImgui.path}/imgui.cpp",
			f"{ExtLibImgui.path}/imgui_demo.cpp",
			f"{ExtLibImgui.path}/imgui_draw.cpp",
			f"{ExtLibImgui.path}/imgui_tables.cpp",
			f"{ExtLibImgui.path}/imgui_widgets.cpp",
			f"{ExtLibImgui.path}/backends/imgui_impl_dx12.cpp",
		)

	with csbuild.Scope(csbuild.ScopeDef.All):
		csbuild.AddIncludeDirectories(ExtLibImgui.path)
		csbuild.AddDefines("ImTextureID=ImU64")

###################################################################################################

class ExtLibImplot(object):
	projectName = "Ext_Implot"
	outputName = "libimplot"
	path = "External/implot"
	projectType = csbuild.ProjectType.StaticLibrary
	dependencies = [
		ExtLibImgui.projectName,
	]

with csbuild.Project(ExtLibImplot.projectName, ExtLibImplot.path, ExtLibImplot.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibImplot.outputName, csbuild.ProjectType.StaticLibrary)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceDirectories(ExtLibImplot.path)

	else:
		csbuild.AddSourceFiles(
			f"{ExtLibImplot.path}/implot.cpp",
			f"{ExtLibImplot.path}/implot_demo.cpp",
			f"{ExtLibImplot.path}/implot_items.cpp",
		)

	with csbuild.Scope(csbuild.ScopeDef.Children):
		csbuild.AddIncludeDirectories(ExtLibImplot.path)

###################################################################################################

class ExtLibMicroIni(object):
	projectName = "Ext_MicroIni"
	outputName = "libmicroini"
	path = "External/MicroIni/src"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibMicroIni.projectName, ExtLibMicroIni.path):
	csbuild.SetOutput(ExtLibMicroIni.outputName, csbuild.ProjectType.StaticLibrary)

	with csbuild.Scope(csbuild.ScopeDef.Children):
		csbuild.AddIncludeDirectories(ExtLibMicroIni.path)

###################################################################################################

class ExtLibStb(object):
	projectName = "Ext_Stb"
	outputName = "libstb"
	path = "External/stb"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibStb.projectName, ExtLibStb.path, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibStb.outputName, csbuild.ProjectType.Stub)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceFiles(f"{ExtLibStb.path}/*.h")

	with csbuild.Scope(csbuild.ScopeDef.Children):
		csbuild.AddIncludeDirectories(ExtLibStb.path)

###################################################################################################

class ExtLibTinyObjLoader(object):
	projectName = "Ext_TinyObjLoader"
	outputName = "libtinyobj"
	path = "External/tinyobjloader"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibTinyObjLoader.projectName, ExtLibTinyObjLoader.path, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibTinyObjLoader.outputName, csbuild.ProjectType.StaticLibrary)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceFiles(f"{ExtLibTinyObjLoader.path}/tiny_obj_loader.*")

	else:
		csbuild.AddSourceFiles(
			f"{ExtLibTinyObjLoader.path}/tiny_obj_loader.cc",
		)

	with csbuild.Scope(csbuild.ScopeDef.Children):
		csbuild.AddIncludeDirectories(ExtLibTinyObjLoader.path)

###################################################################################################

class LibDemoFramework(object):
	projectName = "DemoFramework"
	outputName = "libdf"
	sourcePath = f"{_REPO_ROOT_PATH}/Source"
	dependencies = [
		ExtLibDirectXMath.projectName,
		ExtLibDirectXTex.projectName,
		ExtLibImgui.projectName,
		ExtLibImplot.projectName,
		ExtLibStb.projectName,
		ExtLibTinyObjLoader.projectName,
	]

with csbuild.Project(LibDemoFramework.projectName, LibDemoFramework.sourcePath, LibDemoFramework.dependencies):
	csbuild.SetOutput(LibDemoFramework.outputName, csbuild.ProjectType.SharedLibrary)
	csbuild.SetSupportedPlatforms("Windows")
	csbuild.SetHlslContext("framework")

	csbuild.AddDefines("DF_DLL_EXPORT")

	with csbuild.Scope(csbuild.ScopeDef.Children):
		csbuild.AddIncludeDirectories(LibDemoFramework.sourcePath)

	with csbuild.Toolchain("msvc"):
		csbuild.AddLibraries(
			"d3d12",
			"d3dcompiler",
			"dxgi",
		)

###################################################################################################

class Samples(object):
	rootPath = f"{_REPO_ROOT_PATH}/Samples"
	commonPath = f"{rootPath}/Common"

###################################################################################################

class LibSampleCommon(object):
	projectName = "LibSampleCommon"
	outputName = "libdx12samplecommon"
	path = f"{Samples.rootPath}/Common"
	modelAssetPath = f"{path}/Models"
	textureAssetPath = f"{path}/Textures"
	dependencies = [
		ExtLibImgui.projectName,
		ExtLibImplot.projectName,

		LibDemoFramework.projectName,
	]

	@staticmethod
	def onPostBuild(project):
		subDirName = "common"

		modelOutputRootPath = f"{project.outputDir}/models/{subDirName}"
		textureOutputRootPath = f"{project.outputDir}/textures/{subDirName}"

		# Copy the model files.
		copyAssetFile(f"{LibSampleCommon.modelAssetPath}/head.obj", modelOutputRootPath)

		# Copy the texture files.
		copyAssetFile(f"{LibSampleCommon.textureAssetPath}/pine_attic_2k.hdr", textureOutputRootPath)

with csbuild.Project(LibSampleCommon.projectName, LibSampleCommon.path, LibSampleCommon.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(LibSampleCommon.outputName, csbuild.ProjectType.StaticLibrary)
	csbuild.SetUserData(_POST_BUILD_HOOK, LibSampleCommon.onPostBuild)
	csbuild.SetHlslContext("common")

	csbuild.AddExcludeDirectories(
		LibSampleCommon.modelAssetPath,
		LibSampleCommon.textureAssetPath,
	)
	csbuild.AddSourceDirectories(
		LibSampleCommon.path,
	)

###################################################################################################

class SampleBasic(object):
	projectName = "Sample-Basic"
	outputName = "basic-sample"
	path = f"{Samples.rootPath}/Basic"
	dependencies = [
		LibSampleCommon.projectName,
	]

with csbuild.Project(SampleBasic.projectName, SampleBasic.path, SampleBasic.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(SampleBasic.outputName, csbuild.ProjectType.Application)
	csbuild.SetHlslContext("basic")

	csbuild.AddSourceDirectories(
		SampleBasic.path,
	)

###################################################################################################

class SampleDeferred(object):
	projectName = "Sample-DeferredRendering"
	outputName = "deferred-sample"
	path = f"{Samples.rootPath}/Deferred"
	dependencies = [
		LibSampleCommon.projectName,
	]

with csbuild.Project(SampleDeferred.projectName, SampleDeferred.path, SampleDeferred.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(SampleDeferred.outputName, csbuild.ProjectType.Application)
	csbuild.SetHlslContext("deferred")

	csbuild.AddSourceDirectories(
		SampleDeferred.path,
	)

###################################################################################################

class SampleEnvDiffuse(object):
	projectName = "Sample-EnvDiffuse"
	outputName = "env-diffuse-sample"
	path = f"{Samples.rootPath}/EnvDiffuse"
	dependencies = [
		LibSampleCommon.projectName,
	]

with csbuild.Project(SampleEnvDiffuse.projectName, SampleEnvDiffuse.path, SampleEnvDiffuse.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(SampleEnvDiffuse.outputName, csbuild.ProjectType.Application)
	csbuild.SetHlslContext("env-diffuse")

	csbuild.AddSourceDirectories(
		SampleEnvDiffuse.path,
	)

###################################################################################################
