﻿#pragma once
#include <qtor/torrent.hpp>
#include <qtor/view_manager.hpp>
#include <qtor/abstract_data_source.hpp>
#include <viewed/hash_container.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace qtor
{
	/// Hash store of torrents.
	/// Store is associated with torrents subscription.
	/// It also managed view connected views and automatically pauses subscription 
	/// if there are no connected views.
	class torrent_store :
		public viewed::hash_container<
			torrent, boost::multi_index::const_mem_fun<torrent, torrent_id_type, &torrent::id>
		>,
		public view_manager
	{
		typedef viewed::hash_container<
			torrent, boost::multi_index::const_mem_fun<torrent, torrent_id_type, &torrent::id>
		> base_type;

	protected:
		std::shared_ptr<abstract_data_source> m_source;

	protected:
		auto subscribe() -> ext::net::subscription_handle override;

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
		torrent_store(std::shared_ptr<abstract_data_source> source);
		~torrent_store() = default;
	};


	template <class RecordRange>
	void torrent_store::upsert_records(RecordRange newRecs)
	{
		upsert(std::make_move_iterator(newRecs.begin()), std::make_move_iterator(newRecs.end()));
	}

	template <class RecordRange>
	void torrent_store::assign_records(RecordRange newRecs)
	{
		assign(std::make_move_iterator(newRecs.begin()), std::make_move_iterator(newRecs.end()));
	}
}
