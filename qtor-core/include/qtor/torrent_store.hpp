#pragma once
#include <qtor/torrent.hpp>
#include <qtor/abstract_data_source.hpp>
#include <qtor/view_manager.hpp>
#include <viewed/hash_container_base.hpp>

namespace qtor
{
	/// Torrent store hash store of torrents.
	/// Store is associated with torrents subscription.
	/// It also managed view connected views and automatically pauses subscription 
	/// if there are no connected views.
	class torrent_store :
		public viewed::hash_container_base<
			torrent,
			torrent_id_hasher,
			torrent_id_equal
		>,
		public view_manager
	{
		typedef viewed::hash_container_base<
			torrent,
			torrent_id_hasher,
			torrent_id_equal
		> base_type;

	private:
		std::shared_ptr<abstract_data_source> m_source;

	protected:
		auto subscribe() -> ext::netlib::subscription_handle override;

	public:
		/// добавл€ет данные. ”же имеющиес€ данные обновл€ютс€, остальные добавл€ютс€
		/// определ€етс€ по VariantRecord::id
		/// каждый VariantRecord.fields.size() должен быть равен fields_names().size()
		/// подразумеваетс€ что пол€ соответствуют размещению столбцов
		template <class RecordRange>
		void upsert_records(RecordRange newRecs);

		/// замен€ет текущие данные новыми,
		/// каждый VariantRecord.fields.size() должен быть равен fields_names().size()
		/// подразумеваетс€ что пол€ соответствуют размещению столбцов
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
