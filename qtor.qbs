import qbs
import qbs.Environment

Project
{
	//property pathList additionalIncludePaths: []
	//property pathList additionalLibraryPaths: []
	//property stringList additionalDefines: []
	
	property stringList additionalDefines:
	{
		var defs =[]

		// https://bugreports.qt.io/browse/QTCREATORBUG-20884
		// https://bugreports.qt.io/browse/QTCREATORBUG-19348
		// clang code model at least with creator 4.7.0 has some strange behaviour with __cplusplus define.
		// it is always defined as 201402L unless in project it explicitly defined via cpp.defines otherwise
		//defs.push("__cplusplus=201703L")

		if (qbs.toolchain.contains("msvc"))
			defs = defs.uniqueConcat(["_SCL_SECURE_NO_WARNINGS"])

		return defs
	}

	property stringList additionalCxxFlags:
	{
		var flags = []
		if (qbs.toolchain.contains("msvc"))
		{

		}
		else if (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("clang"))
		{
			//flags.push("-Wsuggest-override")
			flags.push("-Wno-unused-parameter")
			flags.push("-Wno-unused-function")
			flags.push("-Wno-implicit-fallthrough")
            flags.push("-Wno-unused-local-typedefs")
		}

		return flags
	}

	SubProject
	{
		filePath: "externals/extlib/extlib.qbs"
		Properties {
			name: "externals/extlib"
			with_zlib:    true
		}
	}
	
	SubProject
	{
		filePath: "externals/netlib/netlib.qbs"
		Properties {
			name: "externals/netlib"
			with_openssl: true
		}
	}

	SubProject
	{
		filePath: "externals/QtTools/QtTools.qbs"
		Properties { name: "externals/QtTools" }
	}
	
	references: [
//		"qtor/qtor.qbs",
		"qtor-core/qtor-core.qbs",
//		"qtor-sqlite/qtor-sqlite.qbs"
	]
}
