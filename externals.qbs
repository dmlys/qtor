import qbs

Project
{
	SubProject
	{
		filePath: "externals/extlib/extlib.qbs"
		Properties {
			//name: "externals/extlib"
			with_zlib: true
		}
	}

	SubProject
	{
		filePath: "externals/netlib/netlib.qbs"
		Properties {
			//name: "externals/netlib"
			with_openssl: true
		}
	}

	SubProject
	{
		filePath: "externals/QtTools/QtTools.qbs"
		//Properties { name: "externals/QtTools" }
	}

	SubProject
	{
		filePath: "externals/sqlite3yaw/sqlite3yaw.qbs"
		//Properties { name: "externals/sqlite3yaw" }
	}
}
