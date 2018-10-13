import qbs
import qbs.Environment

StaticLibrary
{
	Depends { name: "cpp" }
	Depends { name: "Qt"; submodules: ["core"] }

	Depends { name: "extlib" }
    Depends { name: "netlib" }
    Depends { name: "QtTools" }
	Depends { name: "qtor-core" }
	
	cpp.cxxLanguageVersion : "c++17"
	cpp.defines: project.additionalDefines
	//cpp.includePaths: project.additionalIncludePaths
	cpp.systemIncludePaths: project.additionalSystemIncludePaths
	cpp.cxxFlags: project.additionalCxxFlags
	cpp.driverFlags: project.additionalDriverFlags
	cpp.libraryPaths: project.additionalLibraryPaths

	cpp.includePaths : { 
		var includes = ["include"]

		if (project.additionalIncludePaths)
			includes = includes.uniqueConcat(project.additionalIncludePaths)
		
		return includes
	}
	
	Export
	{
        Depends { name: "cpp" }

		cpp.cxxLanguageVersion : "c++17"
		cpp.includePaths : ["include"]
	}
	
    FileTagger {
        patterns: "*.hqt"
        fileTags: ["hpp"]
    }

	files: [
		"include/**",
		"src/**",
        "resources/**"
	]
}
