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
#include <qtor/utils.hpp>
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

	qtor::torrent_list torrents;
	try
	{
		torrents = source.torrent_get({}).get();
		source.disconnect().get();
	}
	catch (std::exception & ex)
	{
		std::cerr << ex.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	return torrents;
}

void save_torrents(qtor::torrent_list torrents)
{
	using namespace qtor;

	sqlite3yaw::session ses;
	ses.open(g_sqlite_path);

	torrent_meta meta;
	torrent::index_type first = torrent::FirstField;
	torrent::index_type last = torrent::LastField;
	sqlite::column_info info;

	info.resize(last - first);

	for (; first != last; ++first)
	{
		auto & val = info[first];
		auto name = meta.item_name(first);
		auto type = meta.item_type(first);
		val = std::make_tuple(FromQString(name), type);
	}
		
	sqlite::create_table(ses, "torrents", info);	
	sqlite3yaw::table_meta tmeta;

	auto names = info | ext::firsts;
	auto batch_range = make_batch_range(meta, names, torrents);

	sqlite3yaw::batch_insert(batch_range, ses, tmeta);
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
	//save_torrents(std::move(torrents));
}
