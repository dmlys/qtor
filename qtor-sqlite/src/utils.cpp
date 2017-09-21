#include <qtor/utils.hpp>

namespace qtor
{
	sparse_variant to_sparse_variant::operator()(const any_type & val, index_type index) const
	{
		using meta = sparse_container_meta;
		auto type = m_meta->item_type(index);
		switch (type)
		{
			case meta::Speed:
			case meta::Size:
			case meta::Uint64:
				if (auto * ptr = any_cast<uint64_type>(&val))
					return *ptr;
				else
					return nullopt;

			case meta::Bool:
				if (auto * ptr = any_cast<bool>(&val))
					return *ptr;
				else
					return nullopt;

			case meta::Double:
				if (auto * ptr = any_cast<double>(&val))
					return *ptr;
				else
					return nullopt;

			case meta::String:
				if (auto * ptr = any_cast<string_type>(&val))
					return *ptr;
				else
					return nullopt;

			case meta::DateTime:
				if (auto * ptr = any_cast<datetime_type>(&val))
					return *ptr;
				else
					return nullopt;

			case meta::Duration:
				if (auto * ptr = any_cast<duration_type>(&val))
					return *ptr;
				else
					return nullopt;

			default:
				return nullopt;
		}
	}
}