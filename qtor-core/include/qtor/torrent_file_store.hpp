#pragma once
#include <qtor/torrent_file.hpp>
#include <qtor/abstract_data_source.hpp>
#include <viewed/hash_container.hpp>
#include <boost/multi_index/member.hpp>

namespace qtor
{
	class torrent_file_store :
		public viewed::hash_container<
			torrent_file,
	        boost::multi_index::member<torrent_file, filepath_type, &torrent_file::filename>
		>
	{
		using base_type = viewed::hash_container<
			torrent_file,
			boost::multi_index::member<torrent_file, filepath_type, &torrent_file::filename>
		>;

	protected:
		torrent_id_type m_torrent_id;
		std::shared_ptr<abstract_data_source> m_source;

	public:
		auto torrent_id() { return m_torrent_id; }

	public:
		void refresh();

	public:
		/// добавляет данные. Уже имеющиеся данные обновляются, остальные добавляются
		/// определяется по VariantRecord::id
		/// каждый VariantRecord.fields.size() должен быть равен fields_names().size()
		/// подразумевается что поля соответствуют размещению столбцов
		template <class RecordRange>
		void upsert_records(RecordRange newRecs);

		/// заменяет текущие данные новыми,
		/// каждый VariantRecord.fields.size() должен быть равен fields_names().size()
		/// подразумевается что поля соответствуют размещению столбцов
		template <class RecordRange>
		void assign_records(RecordRange newRecs);
		
	public:
		torrent_file_store(torrent_id_type torrent_id, std::shared_ptr<abstract_data_source> source);
		torrent_file_store(const torrent torrent, std::shared_ptr<abstract_data_source> source);
		~torrent_file_store() = default;
	};

	inline torrent_file_store::torrent_file_store(const torrent torrent, std::shared_ptr<abstract_data_source> source)
	    : torrent_file_store(torrent.id(), std::move(source)) {}


	inline torrent_file_store::torrent_file_store(torrent_id_type torrent_id, std::shared_ptr<abstract_data_source> source)
		: m_source(std::move(source)) {}

	template <class RecordRange>
	void torrent_file_store::upsert_records(RecordRange newRecs)
	{
		upsert(std::make_move_iterator(newRecs.begin()), std::make_move_iterator(newRecs.end()));
	}

	template <class RecordRange>
	void torrent_file_store::assign_records(RecordRange newRecs)
	{
		assign(std::make_move_iterator(newRecs.begin()), std::make_move_iterator(newRecs.end()));
	}
}
