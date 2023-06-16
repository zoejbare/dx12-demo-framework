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

###################################################################################################

class ExtLibDirectTex(object):
	projectName = "Ext_DirectXTex"
	outputName = "libdirectxtex"
	path = "External/DirectXTex/DirectXTex"
	projectType = csbuild.ProjectType.StaticLibrary

with csbuild.Project(ExtLibDirectTex.projectName, ExtLibDirectTex.path, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(ExtLibDirectTex.outputName, csbuild.ProjectType.StaticLibrary)

	if csbuild.GetRunMode() == csbuild.RunMode.GenerateSolution:
		csbuild.AddSourceFiles(
			f"{ExtLibDirectTex.path}/*.cpp",
			f"{ExtLibDirectTex.path}/*.h"
		)

	else:
		csbuild.AddSourceFiles(
			f"{ExtLibDirectTex.path}/BC.cpp",
			f"{ExtLibDirectTex.path}/BC4BC5.cpp",
			f"{ExtLibDirectTex.path}/BC6HBC7.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexCompress.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexConvert.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexD3D12.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexDDS.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexFlipRotate.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexHDR.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexImage.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexMipmaps.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexMisc.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexNormalMaps.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexPMAlpha.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexResize.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexTGA.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexUtil.cpp",
			f"{ExtLibDirectTex.path}/DirectXTexWIC.cpp",
		)

	with csbuild.Scope(csbuild.ScopeDef.All):
		csbuild.AddIncludeDirectories(ExtLibDirectTex.path)

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
		ExtLibDirectTex.projectName,
		ExtLibImgui.projectName,
		ExtLibImplot.projectName,
		ExtLibStb.projectName,
		ExtLibTinyObjLoader.projectName,
	]

with csbuild.Project(LibDemoFramework.projectName, LibDemoFramework.sourcePath, LibDemoFramework.dependencies):
	csbuild.SetOutput(LibDemoFramework.outputName, csbuild.ProjectType.SharedLibrary)
	csbuild.SetSupportedPlatforms("Windows")

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

class SampleBasic(object):
	projectName = "Sample-Basic"
	outputName = "basic-sample"
	path = f"{Samples.rootPath}/Basic"
	dependencies = [
		ExtLibImgui.projectName,
		ExtLibImplot.projectName,

		LibDemoFramework.projectName,
	]

with csbuild.Project(SampleBasic.projectName, Samples.rootPath, SampleBasic.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(SampleBasic.outputName, csbuild.ProjectType.Application)
	csbuild.SetHlslContext("basic")

	csbuild.AddSourceDirectories(
		Samples.commonPath,
		SampleBasic.path,
	)

###################################################################################################

class SampleDeferred(object):
	projectName = "Sample-DeferredRendering"
	outputName = "deferred-sample"
	path = f"{Samples.rootPath}/Deferred"
	dependencies = [
		ExtLibImgui.projectName,
		ExtLibImplot.projectName,

		LibDemoFramework.projectName,
	]

with csbuild.Project(SampleDeferred.projectName, Samples.rootPath, SampleDeferred.dependencies, autoDiscoverSourceFiles=False):
	csbuild.SetOutput(SampleDeferred.outputName, csbuild.ProjectType.Application)
	csbuild.SetHlslContext("deferred")

	csbuild.AddSourceDirectories(
		Samples.commonPath,
		SampleDeferred.path,
	)

###################################################################################################
