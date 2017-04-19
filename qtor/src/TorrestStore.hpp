#pragma once
#include <Torrent.hpp>
#include <viewed/hash_container_base.hpp>

namespace qtor
{
	class TorrentStore :
		public viewed::hash_container_base<
			Torrent,
			TorrentIdHasher,
			TorrentIdEqual
		>
	{
		typedef viewed::hash_container_base<
			Torrent,
			TorrentIdHasher,
			TorrentIdEqual
		> base_type;

	private:
		virtual void subscribe();
		virtual void start_subscription();
		virtual void stop_subscription();

	private:
		/// текущее кол-во проекций
		unsigned view_count();
		/// регистрирует проекцию, Возвращает новое кол-во проекций после
		unsigned view_addref();
		/// уменьшает счетчик проекций. Возвращает новое кол-во проекций после 
		unsigned view_release();

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

	};
}
