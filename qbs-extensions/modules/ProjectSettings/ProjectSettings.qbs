import qbs
import qbs.Environment

Module
{
	Depends { name: "cpp" }

	//qbs.debugInformation: true

	cpp.driverFlags:
	{
		if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			return ["-pthread", "-march=native"]
	}

	cpp.defines:
	{
		var defs =[]

		var envDefines = Environment.getEnv("QBS_EXTRA_DEFINES")
		console.log(envDefines)
		if (envDefines)
		{
			var splitter = new RegExp(" " + "|" + qbs.pathListSeparator)
			envDefines = envDefines.split(splitter).filter(Boolean)
			defs = defs.uniqueConcat(envDefines)
		}

		if (qbs.toolchain.contains("msvc"))
			defs = defs.uniqueConcat(["_SCL_SECURE_NO_WARNINGS"])

		if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
			defs = defs.uniqueConcat(["BOOST_ALL_DYN_LINK"])

		return defs
	}

	cpp.cxxFlags:
	{
		var flags = []

		var envFlags = Environment.getEnv("QBS_EXTRA_FLAGS")
		if (envFlags)
		{
			envFlags = envFlags.split(" ").filter(Boolean)
			flags = flags.concat(envFlags);
		}

		if (qbs.toolchain.contains("msvc"))
		{

		}
		else if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
		{
			//flags.push("-Wsuggest-override")
			flags.push("-Wno-extra");
			//flags.push("-Wno-unused-parameter")
			flags.push("-Wno-unused-local-typedefs")
			flags.push("-Wno-unused-function")
			flags.push("-Wno-implicit-fallthrough")
			//flags.push("-Wno-sign-compare")
		}

		return flags
	}

	cpp.includePaths:
	{
		var includes = []
		var envIncludes = Environment.getEnv("QBS_EXTRA_INCLUDES")
		if (envIncludes)
		{
			envIncludes = envIncludes.split(qbs.pathListSeparator).filter(Boolean)
			includes = includes.uniqueConcat(envIncludes)
		}

		//if (qbs.buildVariant == "debug") includes.push("~/.local/opt/qt-5.12.0/include")

		return includes;
	}

	cpp.libraryPaths:
	{
		var libPaths = []
		var envLibPaths = Environment.getEnv("QBS_EXTRA_LIBPATH")
		if (envLibPaths)
		{
			envLibPaths = envLibPaths.split(qbs.pathListSeparator).filter(Boolean)
			libPaths = libPaths.uniqueConcat(envLibPaths)
		}

		//if (qbs.buildVariant == "debug") libPaths.push("~/.local/opt/qt-5.12.0/lib")

		return libPaths
	}
}
