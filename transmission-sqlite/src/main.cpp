#include <csignal>
#include <iostream>

#include <sqlite3yaw.hpp>
#include <sqlite3yaw_ext.hpp>

#include <ext/range/adaptors.hpp>
#include <ext/range/combine.hpp>
#include <ext/range/input_range_facade.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <qtor/sqlite.hpp>
#include <qtor/transmission/data_source.hpp>


std::string g_sqlite_path;
std::string g_url;


qtor::torrent_list load_torrents()
{
	using namespace std::chrono_literals;

	ext::library_logger::stream_logger logger {std::clog};

	qtor::transmission::data_source source;
	source.set_address(g_url);
	source.set_logger(&logger);
	source.set_timeout(10s);

	if (not source.connect().get())
	{
		std::exit(EXIT_FAILURE);
	}
	
	try
	{
		auto torrents = source.torrent_get({}).get();
		source.disconnect().get();
		return torrents;
	}
	catch (std::exception & ex)
	{
		std::cerr << ex.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void save_torrents(qtor::torrent_list torrents)
{
	try
	{
		sqlite3yaw::session ses;
		ses.open(g_sqlite_path);

		qtor::sqlite::create_torrents_table(ses);
		qtor::sqlite::save_torrents(ses, torrents);
	}
	catch (std::exception & ex)
	{
		std::cerr << "sqlite failure: " << ex.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}


int main(int argc, char ** argv)
{
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
		notify(vm);
	}
	catch (po::error & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	ext::init_future_library();
	ext::socket_stream_init();
	auto torrents = load_torrents();
	save_torrents(std::move(torrents));
}
