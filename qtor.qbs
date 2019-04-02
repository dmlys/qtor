import qbs
import qbs.Environment

Project
{
	qbsSearchPaths: ["qbs-extensions"]
	
	references: [
		"externals.qbs",
		"qtor-tests.qbs",

		"qtor/qtor.qbs",
		"qtor-core/qtor-core.qbs",
		"qtor-sqlite/qtor-sqlite.qbs",
		"transmission-remote/transmission-remote.qbs",
		"transmission-sqlite/transmission-sqlite.qbs",


		"externals/QtTools/examples/viewed-examples.qbs",
	]

	AutotestRunner {}
}
