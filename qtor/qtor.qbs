import qbs

CppApplication
{
	Depends { name: "cpp" }
	Depends { name: "Qt"; submodules: ["core", "gui", "widgets", "svg"] }

	Depends { name: "netlib" }
	Depends { name: "extlib" }
	Depends { name: "QtTools" }

	Depends { name: "qtor-core" }
	Depends { name: "qtor-sqlite" }
	Depends { name: "transmission-remote" }

	Depends { name: "ProjectSettings"; required: false }

	cpp.cxxLanguageVersion : "c++17"
	cpp.cxxFlags: project.additionalCxxFlags
	cpp.driverFlags: project.additionalDriverFlags
	cpp.defines: project.additionalDefines
	cpp.systemIncludePaths: project.additionalSystemIncludePaths
	cpp.includePaths: project.additionalIncludePaths
	cpp.libraryPaths: project.additionalLibraryPaths

	cpp.dynamicLibraries: ["z", "stdc++fs", "ssl", "crypto", "sqlite3", "boost_regex", "boost_system", "fmt"]

	files: [
		"src/*"
	]
}
