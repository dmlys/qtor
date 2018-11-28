#include <qtor/utils.hpp>

namespace qtor
{
	types_variant make_types_variant(const model_meta * meta, const model_meta::any_type & val, model_meta::index_type index)
	{		
		auto type = meta->item_type(index);
		switch (type)
		{
			case model_meta::Speed:
			case model_meta::Size:
			case model_meta::Uint64:
				if (auto * ptr = any_cast<uint64_type>(&val))
					return *ptr;
				else
					return nullopt;

			case model_meta::Bool:
				if (auto * ptr = any_cast<bool>(&val))
					return *ptr;
				else
					return nullopt;

			case model_meta::Ratio:
			case model_meta::Percent:
			case model_meta::Double:
				if (auto * ptr = any_cast<double>(&val))
					return *ptr;
				else
					return nullopt;

			case model_meta::String:
				if (auto * ptr = any_cast<string_type>(&val))
					return *ptr;
				else
					return nullopt;

			case model_meta::DateTime:
				if (auto * ptr = any_cast<datetime_type>(&val))
					return *ptr;
				else
					return nullopt;

			case model_meta::Duration:
				if (auto * ptr = any_cast<duration_type>(&val))
					return *ptr;
				else
					return nullopt;

			default:
				return nullopt;
		}
	}
}
