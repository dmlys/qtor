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
		/// ������� ���-�� ��������
		unsigned view_count();
		/// ������������ ��������, ���������� ����� ���-�� �������� �����
		unsigned view_addref();
		/// ��������� ������� ��������. ���������� ����� ���-�� �������� ����� 
		unsigned view_release();

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

	};
}
