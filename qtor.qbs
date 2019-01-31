import qbs
import qbs.Environment

Project
{
	//property pathList additionalIncludePaths: qbs.buildVariant == "debug" ? ["~/.local/opt/qt-5.12.0/include"] : []
	//property pathList additionalLibraryPaths: qbs.buildVariant == "debug" ? ["~/.local/opt/qt-5.12.0/lib"]     : []
	property stringList additionalDriverFlags: ["-pthread"]
	
	property stringList additionalDefines:
	{
		var defs =[]

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
			flags.push("-Wno-extra");
			//flags.push("-Wno-unused-parameter")
			flags.push("-Wno-unused-local-typedefs")
			flags.push("-Wno-unused-function")
			flags.push("-Wno-implicit-fallthrough")
			//flags.push("-Wno-sign-compare")
		}

		return flags
	}

	SubProject
	{
		filePath: "externals/extlib/extlib.qbs"
		Properties {
			name: "externals/extlib"
			with_zlib: true
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

	SubProject
	{
		filePath: "externals/sqlite3yaw/sqlite3yaw.qbs"
		Properties { name: "externals/sqlite3yaw" }
	}
	
	references: [
		"qtor/qtor.qbs",
		"qtor-core/qtor-core.qbs",
		"qtor-sqlite/qtor-sqlite.qbs",
		"transmission-remote/transmission-remote.qbs",
		"transmission-sqlite/transmission-sqlite.qbs",
		"externals/QtTools/examples/viewed-examples.qbs",
	]
}
