#pragma once
#include <qtor/sftree_facade_qtbase.hpp>

namespace viewed
{
	template <class Traits, class ModelBase = QAbstractItemModel>
	class sftree_model_qtbase : public sftree_facade_qtbase<Traits, ModelBase>
	{

	};
}
