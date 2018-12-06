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
#include <fmt/core.h>

std::string g_sqlite_path;
std::string g_url;
ext::library_logger::stream_logger g_logger {std::clog};
qtor::transmission::data_source g_source;
sqlite3yaw::session g_session;
constexpr unsigned request_slots = 8;


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

template <class LoadFunc, class SaveFunc>
void load_and_save(const qtor::torrent_list & torrents, LoadFunc loader, SaveFunc saver)
{	
	using future_type = std::invoke_result_t<LoadFunc, qtor::abstract_data_source &, qtor::torrent_id_type>;
	std::array<future_type, request_slots> request_array;

	const auto batch_count = torrents.size() / request_slots;
	const auto left_count  = torrents.size() % request_slots;
	const auto count = batch_count ? request_slots : left_count;

	// schedule enough torrents loads
	for (unsigned i = 0; i < count; ++i)
		request_array[i] = loader(g_source, torrents[i].id());

	// process loaded ones and schedule new get operations
	for (unsigned iteration = 0; iteration < batch_count; ++iteration)
	{
		for (unsigned i = 0; i < request_slots; ++i)
		{
			auto cur_idx = iteration * request_slots + i;
			auto next_idx = iteration * request_slots + request_slots + i;
			auto entity = request_array[i].get();

			if (next_idx < torrents.size())
				request_array[i] = loader(g_source, torrents[next_idx].id());

			saver(g_session, std::move(entity), torrents[cur_idx].id());
		}
	}

	// process leftovers
	for (unsigned i = 0; i < left_count; ++i)
	{
		auto & save_tor = torrents[torrents.size() - left_count + i];
		auto entity = request_array[i].get();
		saver(g_session, std::move(entity), save_tor.id());
	}
}

void load_and_save_torrent_files(const qtor::torrent_list & torrents)
{
	using std::placeholders::_1; using std::placeholders::_2;
	auto loader = std::bind(&qtor::abstract_data_source::get_torrent_files, _1, _2);
	auto saver = qtor::sqlite::save_torrent_files;
	return load_and_save(torrents, loader, saver);
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
	g_source.set_request_slots(request_slots);

	qtor::sqlite::drop_torrents_table(g_session);
	qtor::sqlite::drop_torrent_files_table(g_session);
	qtor::sqlite::create_torrents_table(g_session);
	qtor::sqlite::create_torrent_files_table(g_session);

	if (not g_source.connect().get())
		return EXIT_FAILURE;

	sqlite3yaw::exclusive_transaction tr(g_session);

	auto torrents = g_source.get_torrents().get();
	qtor::sqlite::save_torrents(g_session, torrents);
	load_and_save_torrent_files(torrents);

	tr.commit();

	g_source.disconnect().get();
	return EXIT_SUCCESS;
}
