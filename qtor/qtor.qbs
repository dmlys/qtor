import qbs

CppApplication
{
	Depends { name: "cpp" }
	Depends { name: "Qt"; submodules: ["core", "gui", "widgets"] }

	Depends { name: "netlib" }
	Depends { name: "extlib" }
	Depends { name: "QtTools" }

	Depends { name: "qtor-core" }
	Depends { name: "qtor-sqlite" }
	Depends { name: "transmission-remote" }

	cpp.cxxLanguageVersion : "c++17"
	cpp.defines: project.additionalDefines
	cpp.includePaths: project.additionalIncludePaths
	cpp.systemIncludePaths: project.additionalSystemIncludePaths
	cpp.cxxFlags: project.additionalCxxFlags
	cpp.driverFlags: project.additionalDriverFlags
	cpp.libraryPaths: project.additionalLibraryPaths

	cpp.dynamicLibraries: ["yaml-cpp", "z", "stdc++fs", "ssl", "crypto", "boost_regex", "boost_system", "fmt"]

	files: [
		"src/*"
	]
}
