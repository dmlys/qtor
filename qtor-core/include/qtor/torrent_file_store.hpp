#pragma once
#include <viewed/hash_container_base.hpp>
#include <qtor/abstract_data_source.hpp>
#include <qtor/view_manager.hpp>
#include <qtor/torrent.hpp>

namespace qtor
{
	class torrent_file_store :
		public viewed::hash_container_base<
			torrent_file,
			torrent_file_id_hasher,
			torrent_file_id_equal
		>
	{
		using base_type = viewed::hash_container_base<
			torrent_file,
			torrent_file_id_hasher,
			torrent_file_id_equal
		>;

	public:
		/// ��������� ������. ��� ��������� ������ �����������, ��������� �����������
		/// ������������ �� VariantRecord::id
		/// ������ VariantRecord.fields.size() ������ ���� ����� fields_names().size()
		/// ��������������� ��� ���� ������������� ���������� ��������
		template <class RecordRange>
		void upsert_records(RecordRange newRecs);

		/// �������� ������� ������ ������,
		/// ������ VariantRecord.fields.size() ������ ���� ����� fields_names().size()
		/// ��������������� ��� ���� ������������� ���������� ��������
		template <class RecordRange>
		void assign_records(RecordRange newRecs);
		
	public:
		using base_type::base_type;
	};


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
