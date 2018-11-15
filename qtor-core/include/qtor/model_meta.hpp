#pragma once
#include <qtor/types.hpp>

namespace qtor
{
	class model_meta
	{
	public:
		using index_type = unsigned;
		using any_type = any;

	public:
		// currently supported types,
		// it's not enum to allow user custom extension
		static constexpr unsigned Uint64   = 0;
		static constexpr unsigned Int64    = 1;
		static constexpr unsigned Bool     = 2;
		static constexpr unsigned Double   = 3;
		static constexpr unsigned String   = 4;

		static constexpr unsigned Speed    = 5;
		static constexpr unsigned Size     = 6;
		static constexpr unsigned DateTime = 7;
		static constexpr unsigned Duration = 8;
		static constexpr unsigned Percent  = 9;
		static constexpr unsigned Ratio    = 10;

		static constexpr unsigned Unknown = -1;

	public:
		virtual  index_type item_count()                const noexcept = 0;
		virtual    unsigned item_type(index_type index) const noexcept = 0;
		virtual string_type item_name(index_type index) const = 0;

	public:
		virtual ~model_meta() = default;
	};

	template <class Type>
	class model_accessor : public model_meta
	{
	public:
		any_type get_item(const Type & item, int index) const = 0;
	};
}

Q_DECLARE_METATYPE(qtor::model_meta *);
