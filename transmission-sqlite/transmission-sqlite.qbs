import qbs

CppApplication
{
	Depends { name: "cpp" }
	Depends { name: "Qt"; submodules: ["core"] }

	Depends { name: "netlib" }
	Depends { name: "extlib" }
	Depends { name: "sqlite3yaw" }
	Depends { name: "QtTools" }

	Depends { name: "qtor-core" }
	Depends { name: "qtor-sqlite" }
	Depends { name: "transmission-remote" }

	cpp.cxxLanguageVersion : "c++17"
	cpp.cxxFlags: project.additionalCxxFlags
	cpp.driverFlags: project.additionalDriverFlags
	cpp.defines: project.additionalDefines
	cpp.systemIncludePaths: project.additionalSystemIncludePaths
	cpp.includePaths: project.additionalIncludePaths
	cpp.libraryPaths: project.additionalLibraryPaths

	cpp.dynamicLibraries: ["yaml-cpp", "z", "fmt", "stdc++fs", "ssl", "crypto", "sqlite3", "boost_regex", "boost_system", "boost_program_options"]

	files: [
		"src/*"
	]
}
