import qbs
import qbs.Environment

StaticLibrary
{
	Depends { name: "cpp" }
	Depends { name: "extlib" }
    Depends { name: "netlib" }
    Depends { name: "QtTools" }
	
	Depends { name: "Qt"; submodules: ["core", "gui", "widgets"] }
	
	cpp.cxxLanguageVersion : "c++17"
	cpp.driverFlags : ["-pthread"]
	
	cpp.cxxFlags: project.additionalCxxFlags
	cpp.defines: project.additionalDefines
	//cpp.libraryPaths: project.additionalLibraryPaths

	cpp.includePaths : { 
		var includes = ["include"]
		if (project.additionalIncludePaths)
			includes = includes.uniqueConcat(project.additionalIncludePaths)
			
		var envIncludes = Environment.getEnv("QBS_THIRDPARTY_INCLUDES")
		if (envIncludes)
		{
			envIncludes = envIncludes.split(qbs.pathListSeparator)
			includes = includes.uniqueConcat(envIncludes)
		}
		
		return includes
	}
	
	Export
	{
        Depends { name: "cpp" }

		cpp.cxxLanguageVersion : "c++17"
		cpp.driverFlags : ["-pthread"]
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
