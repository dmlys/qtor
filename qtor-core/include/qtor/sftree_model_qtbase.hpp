﻿#pragma once
#include <qtor/sftree_facade_qtbase.hpp>

namespace viewed
{
	template <class Traits, class ModelBase = QAbstractItemModel>
	class sftree_model_qtbase : public sftree_facade_qtbase<Traits, ModelBase>
	{
		using self_type = sftree_model_qtbase;
		using base_type = sftree_facade_qtbase<Traits, ModelBase>;

	public:
		using typename base_type::leaf_type;
		using typename base_type::node_type;
		using typename base_type::value_ptr;

	protected:
		using typename base_type::page_type;

	protected:
		using base_type::get_path;
		using base_type::path_group_pred;

	protected:
		void fill_children_leafs(const page_type & page, std::vector<const leaf_type *> & elements);

	public:
		template <class Iterator>
		auto assign(Iterator first, Iterator last) -> std::enable_if_t<ext::is_iterator_v<Iterator>>;

		template <class Iterator>
		auto upsert(Iterator first, Iterator last) -> std::enable_if_t<ext::is_iterator_v<Iterator>>;

		template <class Range>
		auto assign(Range && range) -> std::enable_if_t<ext::is_range_v<ext::remove_cvref_t<Range>>>;

		template <class Range>
		auto upsert(Range && range) -> std::enable_if_t<ext::is_range_v<ext::remove_cvref_t<Range>>>;
		
		void clear();

	public:
		using base_type::base_type;
	};

	template <class Traits, class ModelBase>
	void sftree_model_qtbase<Traits, ModelBase>::fill_children_leafs(const page_type & page, std::vector<const leaf_type *> & elements)
	{
		for (auto & child : page.children)
		{
			if (child.index() == PAGE)
				fill_children_leafs(*static_cast<const page_type *>(child.pointer()), elements);
			else
				elements.push_back(static_cast<const leaf_type *>(child.pointer()));
		}
	}

	template <class Traits, class ModelBase>
	void sftree_model_qtbase<Traits, ModelBase>::clear()
	{
		this->beginResetModel();
		this->m_root.children.clear();
		this->m_root.nvisible = 0;
		this->endResetModel();
	}

	template <class Traits, class ModelBase>
	template <class Range>
	inline std::enable_if_t<ext::is_range_v<ext::remove_cvref_t<Range>>>
	sftree_model_qtbase<Traits, ModelBase>::assign(Range && range)
	{
		return assign(boost::begin(range), boost::end(range));
	}

	template <class Traits, class ModelBase>
	template <class Range>
	inline std::enable_if_t<ext::is_range_v<ext::remove_cvref_t<Range>>>
	sftree_model_qtbase<Traits, ModelBase>::upsert(Range && range)
	{
		return upsert(boost::begin(range), boost::end(range));
	}


	template <class Traits, class ModelBase>
	template <class Iterator>
	std::enable_if_t<ext::is_iterator_v<Iterator>>
	sftree_model_qtbase<Traits, ModelBase>::assign(Iterator first, Iterator last)
	{
		std::vector<const leaf_type *> existing;
		std::vector<std::unique_ptr<leaf_type>> elements;
		ext::try_reserve(elements, first, last);

		auto make_unique = [](auto && element) { return std::make_unique<leaf_type>(std::forward<decltype(element)>(element)); };
		elements.assign(
			boost::make_transform_iterator(first, make_unique),
			boost::make_transform_iterator(last, make_unique));

		fill_children_leafs(this->m_root, existing);

		auto el_first = elements.begin();
		auto el_last = elements.end();

		auto erased_first = existing.begin();
		auto erased_last = existing.end();		

		this->group_by_paths(erased_first, erased_last);
		this->group_by_paths(el_first, el_last);

		auto leaf_equal = [](auto && l1, auto && l2) { return base_type::path_equal_to(get_path(*l1), get_path(*l2)); };
		el_last = std::unique(el_first, el_last, leaf_equal);


		auto pred = [erased_first, erased_last](auto & ptr)
		{
			// e1 - from [erased_first, erased-last), e2 - from ptr
			auto pred = [](auto * e1, auto * e2) { return path_group_pred(viewed::unmark_pointer(e1), e2); };

			// not (*it < ptr.get())
			// not (ptr.get() < *it) => *it === ptr
			auto it = std::lower_bound(erased_first, erased_last, ptr.get(), pred);
			if (it == erased_last or pred(ptr.get(), viewed::unmark_pointer(*it))) return false;

			*it = viewed::mark_pointer(*it);
			return true;
		};
		
		auto pp = std::stable_partition(el_first, el_last, pred);
		erased_last = std::remove_if(erased_first, erased_last, viewed::marked_pointer);

		// [el_first, pp) - updated, [pp, el_last) - inserted		
		auto updated  = boost::make_iterator_range(el_first, pp) | ext::moved;
		auto inserted = boost::make_iterator_range(pp, el_last)  | ext::moved;

		return this->update_data_and_notify(
			erased_first, erased_last,
			updated.begin(), updated.end(),
			inserted.begin(), inserted.end());
	}

	template <class Traits, class ModelBase>
	template <class Iterator>
	std::enable_if_t<ext::is_iterator_v<Iterator>>
	sftree_model_qtbase<Traits, ModelBase>::upsert(Iterator first, Iterator last)
	{
		std::vector<const leaf_type *> existing;
		std::vector<std::unique_ptr<leaf_type>> elements;
		ext::try_reserve(elements, first, last);

		auto make_unique = [](auto && element) { return std::make_unique<leaf_type>(std::forward<decltype(element)>(element)); };
		elements.assign(
			boost::make_transform_iterator(first, make_unique),
			boost::make_transform_iterator(last, make_unique));

		fill_children_leafs(this->m_root, existing);

		auto el_first = elements.begin();
		auto el_last = elements.end();

		auto ex_first = existing.begin();
		auto ex_last = existing.end();

		this->group_by_paths(ex_first, ex_last);
		this->group_by_paths(el_first, el_last);

		auto leaf_equal = [](auto && l1, auto && l2) { return base_type::path_equal_to(get_path(*l1), get_path(*l2)); };
		el_last = std::unique(el_first, el_last, leaf_equal);


		auto pred = [ex_first, ex_last](auto & ptr)
		{
			// e1 - from [erased_first, erased-last), e2 - from ptr
			auto pred = [](auto * e1, auto * e2) { return path_group_pred(e1, e2); };

			// not (*it < ptr.get())
			// not (ptr.get() < *it) => *it === ptr
			auto it = std::lower_bound(ex_first, ex_last, ptr.get(), pred);
			return it != ex_last and not pred(ptr.get(), *it);
		};

		auto pp = std::stable_partition(el_first, el_last, pred);

		// [el_first, pp) - updated, [pp, el_last) - inserted
		auto updated  = boost::make_iterator_range(el_first, pp) | ext::moved;
		auto inserted = boost::make_iterator_range(pp, el_last)  | ext::moved;

		return this->update_data_and_notify(
			ex_first, ex_first, // no erases
			updated.begin(), updated.end(),
			inserted.begin(), inserted.end());
	}
}
