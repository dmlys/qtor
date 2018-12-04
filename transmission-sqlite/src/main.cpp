#include <csignal>
#include <iostream>

#include <sqlite3yaw.hpp>
#include <sqlite3yaw_ext.hpp>

#include <ext/range/adaptors.hpp>
#include <ext/range/combine.hpp>
#include <ext/range/input_range_facade.hpp>

#include <boost/program_options.hpp>

#include <qtor/sqlite.hpp>
#include <qtor/sqlite-datasource.hpp>
#include <qtor/transmission/data_source.hpp>

#include <qtor/model_meta.hpp>
#include <qtor/custom_meta.hpp>

#include <fmt/format.h>

std::string g_sqlite_path;
std::string g_url;
ext::library_logger::stream_logger g_logger {std::clog};
qtor::transmission::data_source g_source;
sqlite3yaw::session g_session;

void print_help(const boost::program_options::options_description & opts)
{
	auto descr = "Simple program for dumping data from tranmission rpc to sqlite database, used mostly for testing\n";
	auto examples = "Examples:\n"
	    "  transmission-sqlite http://localhost:9091/transmission/rpc sqlite.db";

	std::cout
	    << descr
	    << opts
	    << examples
	    << std::endl;
}

int main(int argc, char ** argv)
{
	qtor::torrent_file_meta meta;
	qtor::custom_meta<qtor::torrent_file> bmeta(meta);

	bmeta.push_bound_value("torrent_id", meta.String, "123");

	qtor::torrent_file tf;
	tf.filename = "test.txt";
	tf.have_size = tf.total_size = 100;
	tf.index = 10;
	tf.priority = 1;
	tf.wanted = true;

	unsigned n = bmeta.item_count();
	bmeta.set_item(tf, qtor::torrent_file_meta::FilePath, "abc.txt");

	for (unsigned u = 0; u < n; ++u)
	{
		fmt::print("{}: {}\n", bmeta.item_name(u).toStdString(), bmeta.get_item(tf, u).toString().toStdString());
	}

	return 0;

	namespace po = boost::program_options;
	po::options_description opts {"options"};
	opts.add_options()
		("help,h", "print help message")
		("url", po::value(&g_url)->required(), "transmission server url")
		("path", po::value(&g_sqlite_path)->required(), "sqlite database path")
		;

	po::positional_options_description pos;
	pos.add("url", 1);
	pos.add("path", 1);

	po::variables_map vm;

	try
	{
		store(po::command_line_parser(argc, argv).options(opts).positional(pos).run(), vm);

		if (vm.count("help") or argc < 2)
		{
			print_help(opts);
			return EXIT_SUCCESS;
		}

		notify(vm);
	}
	catch (po::error & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	using namespace std;
	ext::init_future_library();
	ext::netlib::socket_stream_init();

	g_session.open(g_sqlite_path);

	g_source.set_address(g_url);
	g_source.set_logger(&g_logger);
	g_source.set_timeout(10s);

	if (not g_source.connect().get())
	{
		return EXIT_FAILURE;
	}

	auto torrents = g_source.get_torrents().get();
	qtor::sqlite::drop_torrents_table(g_session);
	qtor::sqlite::drop_torrent_files_table(g_session);
	qtor::sqlite::create_torrents_table(g_session);
	qtor::sqlite::create_torrent_files_table(g_session);

	qtor::sqlite::save_torrents(g_session, torrents);

	for (auto & tor : torrents)
	{
		auto id = tor.id();
		auto files = g_source.get_torrent_files(id);

	}

	g_source.disconnect().get();
	return EXIT_SUCCESS;
}
