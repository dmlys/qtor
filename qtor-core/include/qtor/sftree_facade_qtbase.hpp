#pragma once
#include <memory>
#include <vector>
#include <tuple>
#include <algorithm>

#include <viewed/algorithm.hpp>
#include <viewed/refilter_type.hpp>
#include <viewed/get_functor.hpp>
#include <viewed/indirect_functor.hpp>
#include <viewed/qt_model.hpp>
#include <qtor/pointer_variant.hpp>

#include <boost/range/detail/range_return.hpp>
#include <varalgo/sorting_algo.hpp>
#include <varalgo/on_sorted_algo.hpp>

#include <ext/utility.hpp>
#include <ext/iterator/zip_iterator.hpp>
#include <ext/iterator/outdirect_iterator.hpp>
#include <ext/iterator/indirect_iterator.hpp>
#include <ext/range/adaptors/outdirected.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <QtCore/QAbstractItemModel>
#include <QtTools/ToolsBase.hpp>


namespace viewed
{
	namespace detail
	{
		const auto make_ref = [](auto * ptr) { return std::ref(*ptr); };
	}

	template <class Traits, class ModelBase = QAbstractItemModel>
	class sftree_facade_qtbase : 
		protected Traits,
		public ModelBase
	{
		using self_type = sftree_facade_qtbase;
		using traits_type = Traits;

		static_assert(std::is_base_of_v<QAbstractItemModel, ModelBase>);

	protected:
		using model_base = ModelBase;
		using model_helper = AbstractItemModel;
		using int_vector = std::vector<int>;

		struct page_type;
		using typename traits_type::leaf_type;
		using typename traits_type::node_type;
		using typename traits_type::path_type;
		using typename traits_type::pathview_type;

		using typename traits_type::sort_pred_type;
		using typename traits_type::filter_pred_type;

		static constexpr unsigned PAGE = 0;
		static constexpr unsigned LEAF = 1;

		using value_ptr = viewed::pointer_variant<const page_type *, const leaf_type *>;

		using leaf_ptr_vector    = std::vector<const leaf_type *>;
		using value_ptr_vector   = std::vector<const value_ptr *>;
		using leaf_ptr_iterator  = typename leaf_ptr_vector::iterator;
		using value_ptr_iterator = typename value_ptr_vector::iterator;

		struct get_segment_type
		{
			using result_type = path_type;
			result_type operator()(const path_type & path) const { return traits_type::get_segment(path); }
			result_type operator()(const leaf_type & leaf) const { return traits_type::get_segment(leaf); }
			result_type operator()(const leaf_type * leaf) const { return traits_type::get_segment(*leaf); }
			result_type operator()(const page_type & page) const { return traits_type::get_segment(static_cast<const node_type &>page); }
			result_type operator()(const page_type * page) const { return traits_type::get_segment(*page); }
			result_type operator()(const value_ptr & val)  const { return viewed::pv_visit(*this, val); }
		};


		using value_container = boost::multi_index_container<
			value_ptr,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<get_segment_type>,
				boost::multi_index::random_access<>
			>
		>;

		static constexpr unsigned by_code = 0;
		static constexpr unsigned by_seq = 1;

		using code_view_type = typename value_container::template nth_index<by_code>::type;
		using seq_view_type  = typename value_container::template nth_index<by_seq>::type;

		struct page_type_base
		{
			page_type *     parent = nullptr;
			std::size_t     upassed = 0;
			value_container children;
		};

		struct page_type : page_type_base, traits_type::node_type 
		{

		};

		struct get_children_type
		{
			using result_type = const value_container &;
			result_type operator()(const leaf_type & leaf) const { return ms_empty_container; }
			result_type operator()(const page_type & page) const { return page.children; }
			result_type operator()(const value_ptr & val)  const { return viewed::pv_visit(*this, val); }

			template <class Type>
			result_type operator()(const Type * ptr) const { return operator()(*ptr); }
		};

		struct get_children_count_type
		{
			using result_type = std::size_t;
			result_type operator()(const leaf_type & leaf) const { return 0; }
			result_type operator()(const page_type & page) const { return page.upassed; }
			result_type operator()(const value_ptr & val)  const { return viewed::pv_visit(*this, val); }

			template <class Type>
			result_type operator()(const Type * ptr) const { return operator()(*ptr); }
		};


		struct resort_context
		{
			value_ptr_vector * vptr_array;
			int_vector * index_array, *inverse_array;
			QModelIndexList::const_iterator model_index_first, model_index_last;
		};

		struct refilter_context
		{
			value_ptr_vector * vptr_array;
			int_vector * index_array, *inverse_array;
			QModelIndexList::const_iterator model_index_first, model_index_last;
		};



		template <class Pred>
		struct value_ptr_filter_type
		{
			using indirect_type = typename viewed::make_indirect_pred_type<Pred>::type;
			indirect_type pred;

			value_ptr_filter_type(Pred pred)
				: pred(viewed::make_indirect_fun(std::move(pred))) {}

			auto operator()(const value_ptr & v) const
			{
				return get_children_count(v) > 0 or viewed::pv_visit(pred, v);
			}
		};

		template <class Pred>
		struct value_ptr_sorter_type
		{
			using indirect_type = typename viewed::make_indirect_pred_type<Pred>::type;
			indirect_type pred;

			value_ptr_sorter_type(Pred pred)
				: pred(viewed::make_indirect_fun(std::move(pred))) {}

			auto operator()(const value_ptr & v1, const value_ptr & v2) const
			{
				return viewed::pv_visit(pred, v1, v2);
			}
		};

	protected:
		static constexpr get_segment_type        get_segment {};
		static constexpr get_children_type       get_children {};
		static constexpr get_children_count_type get_children_count {};

	protected:
		static const value_container ms_empty_container;

	protected:
		page_type m_root; //= { /*.parent =*/ nullptr};

		sort_pred_type m_sorter;
		filter_pred_type m_filter;

	protected:
		template <class Functor> 
		static void for_each_child_page(page_type & page, Functor && func);

		template <class RandomAccessIterator>
		static void group_by_dirs(RandomAccessIterator first, RandomAccessIterator last);

	protected:
		// core QAbstractItemModel functionality implementation
		page_type * get_page(const QModelIndex & index) const;
		const value_ptr & get_element_ptr(const QModelIndex & index) const;
		QModelIndex create_index(int row, int column, page_type * ptr) const;

	public:
		int rowCount(const QModelIndex & parent) const override;
		QModelIndex parent(const QModelIndex & index) const override;
		QModelIndex index(int row, int column, const QModelIndex & parent) const override;

	protected:
		// implemented by derived class
		virtual bool is_subelement(const pathview_type & prefix, const path_type & path, const leaf_type & item) = 0;
		virtual auto analyze(const pathview_type & prefix, const leaf_type & item)
			-> std::tuple<std::uintptr_t, path_type, pathview_type> = 0;

	protected:
		/// emits qt signal this->dataChanged about changed rows. Changred rows are defined by [first; last)
		/// default implantation just calls this->dataChanged(index(row, 0), inex(row, this->columnCount)
		virtual void emit_changed(int_vector::const_iterator first, int_vector::const_iterator last);
		/// changes persistent indexes via this->changePersistentIndex.
		/// [first; last) - range where range[oldIdx - offset] => newIdx.
		/// if newIdx < 0 - index should be removed(changed on invalid, qt supports it)
		virtual void change_indexes(page_type & page, QModelIndexList::const_iterator model_index_first, QModelIndexList::const_iterator model_index_last,
		                            int_vector::const_iterator first, int_vector::const_iterator last, int offset);
		/// inverses index array in following way:
		/// inverse[arr[i] - offset] = i for first..last.
		/// This is for when you have array of arr[new_index] => old_index,
		/// but need arr[old_index] => new_idx for qt changePersistentIndex
		void inverse_index_array(int_vector & inverse, int_vector::iterator first, int_vector::iterator last, int offset);

	protected:
		/// merges m_store's [middle, last) into [first, last) according to m_sort_pred. stable.
		/// first, middle, last - is are one range, as in std::inplace_merge
		/// if resort_old is true it also resorts [first, middle), otherwise it's assumed it's sorted
		virtual void merge_newdata(
			value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last,
			bool resort_old = true);
		
		/// merges m_store's [middle, last) into [first, last) according to m_sort_pred. stable.
		/// first, middle, last - is are one range, as in std::inplace_merge
		/// if resort_old is true it also resorts [first, middle), otherwise it's assumed it's sorted
		/// 
		/// range [ifirst, imiddle, ilast) must be permuted the same way as range [first, middle, last)
		virtual void merge_newdata(
			value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last,
			int_vector::iterator ifirst, int_vector::iterator imiddle, int_vector::iterator ilast,
			bool resort_old = true);
		
		///// sorts m_store's [first; last) with m_sort_pred, stable sort
		virtual void stable_sort(value_ptr_iterator first, value_ptr_iterator last);
		///// sorts m_store's [first; last) with m_sort_pred, stable sort
		///// range [ifirst; ilast) must be permuted the same way as range [first; last)
		virtual void stable_sort(value_ptr_iterator first, value_ptr_iterator last,
		                         int_vector::iterator ifirst, int_vector::iterator ilast);

		/// sorts m_store's [first; last) with m_sort_pred, stable sort
		/// emits qt layoutAboutToBeChanged(..., VerticalSortHint), layoutUpdated(..., VerticalSortHint)
		virtual void sort_and_notify();
		virtual void sort_and_notify(page_type & page, resort_context & ctx);

		/// refilters m_store with m_filter_pred according to rtype:
		/// * same        - does nothing and immediately returns(does not emit any qt signals)
		/// * incremental - calls refilter_full_and_notify
		/// * full        - calls refilter_incremental_and_notify
		virtual void refilter_and_notify(viewed::refilter_type rtype);
		/// removes elements not passing m_filter_pred from m_store
		/// emits qt layoutAboutToBeChanged(..., NoLayoutChangeHint), layoutUpdated(..., NoLayoutChangeHint)
		virtual void refilter_incremental_and_notify();
		virtual void refilter_incremental_and_notify(page_type & page, refilter_context & ctx);
		/// fills m_store from owner with values passing m_filter_pred and sorts them according to m_sort_pred
		/// emits qt layoutAboutToBeChanged(..., NoLayoutChangeHint), layoutUpdated(..., NoLayoutChangeHint)
		virtual void refilter_full_and_notify();
		virtual void refilter_full_and_notify(page_type & page, refilter_context & ctx);

	public:
		using model_base::model_base;
		virtual ~sftree_facade_qtbase() = default;
	};

	/************************************************************************/
	/*                  Methods implementations                             */
	/************************************************************************/
	template <class Traits, class ModelBase>
	const typename sftree_facade_qtbase<Traits, ModelBase>::value_container sftree_facade_qtbase<Traits, ModelBase>::ms_empty_container;

	template <class Traits, class ModelBase>
	template <class Functor>
	void sftree_facade_qtbase<Traits, ModelBase>::for_each_child_page(page_type & page, Functor && func)
	{
		for (auto & child : page.children)
		{
			if (child.index() == PAGE)
			{
				auto * child_page = static_cast<page_type *>(child.pointer());
				std::forward<Functor>(func)(*child_page);
			}
		}
	}

	template <class Traits, class ModelBase>
	template <class RandomAccessIterator>
	void sftree_facade_qtbase<Traits, ModelBase>::group_by_dirs(RandomAccessIterator first, RandomAccessIterator last)
	{
		auto sort_pred = [](const auto & leaf1, const auto & leaf2) { return get_segment(leaf1) > get_segment(leaf2); };
		std::sort(first, last, viewed::make_indirect_fun(sort_pred));
	}

	/************************************************************************/
	/*       QAbstractItemModel tree implementation                         */
	/************************************************************************/
	template <class Traits, class ModelBase>
	QModelIndex sftree_facade_qtbase<Traits, ModelBase>::create_index(int row, int column, page_type * ptr) const
	{
		return this->createIndex(row, column, ptr);
	}

	template <class Traits, class ModelBase>
	inline auto sftree_facade_qtbase<Traits, ModelBase>::get_page(const QModelIndex & index) const -> page_type *
	{
		return static_cast<page_type *>(index.internalPointer());
	}

	template <class Traits, class ModelBase>
	auto sftree_facade_qtbase<Traits, ModelBase>::get_element_ptr(const QModelIndex & index) const -> const value_ptr &
	{
		assert(index.isValid());
		auto * page = get_page(index);
		assert(page);

		auto & seq_view = page->children.template get<by_seq>();
		assert(index.row() < get_children_count(page));
		return seq_view[index.row()];
	}

	template <class Traits, class ModelBase>
	int sftree_facade_qtbase<Traits, ModelBase>::rowCount(const QModelIndex & parent) const
	{
		if (not parent.isValid())
			return qint(get_children_count(&m_root));

		const auto & val = get_element_ptr(parent);
		return qint(get_children_count(val));
	}

	template <class Traits, class ModelBase>
	QModelIndex sftree_facade_qtbase<Traits, ModelBase>::parent(const QModelIndex & index) const
	{
		if (not index.isValid()) return {};

		page_type * page = get_page(index);
		assert(page);

		page_type * parent_page = page->parent;
		if (not parent_page) return {}; // already top level index

		auto & children = parent_page->children;
		auto & seq_view = children.template get<by_seq>();

		auto code_it = children.find(get_segment(page));
		auto seq_it = children.template project<by_seq>(code_it);

		int row = qint(seq_it - seq_view.begin());
		return create_index(row, 0, parent_page);
	}

	template <class Traits, class ModelBase>
	QModelIndex sftree_facade_qtbase<Traits, ModelBase>::index(int row, int column, const QModelIndex & parent) const
	{
		if (not parent.isValid())
		{
			return create_index(row, column, ext::unconst(&m_root));
		}
		else
		{
			auto & element = get_element_ptr(parent);
			auto & children = get_children(element);
			auto count = get_children_count(element);

			if (count < row)
				return {};

			// only page can have children
			assert(element.index() == PAGE);
			auto * page = static_cast<page_type *>(element.pointer());
			return create_index(row, column, page);
		}
	}

	/************************************************************************/
	/*                     qt emit helpers                                  */
	/************************************************************************/
	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::emit_changed(int_vector::const_iterator first, int_vector::const_iterator last)
	{
		if (first == last) return;

		int ncols = this->columnCount(model_helper::invalid_index);
		for (; first != last; ++first)
		{
			// lower index on top, higher on bottom
			int top, bottom;
			top = bottom = *first;

			// try to find the sequences with step of 1, for example: ..., 4, 5, 6, ...
			for (++first; first != last and *first - bottom == 1; ++first, ++bottom)
				continue;

			--first;

			auto top_left = this->index(top, 0, model_helper::invalid_index);
			auto bottom_right = this->index(bottom, ncols - 1, model_helper::invalid_index);
			this->dataChanged(top_left, bottom_right, model_helper::all_roles);
		}
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::change_indexes(page_type & page, QModelIndexList::const_iterator model_index_first, QModelIndexList::const_iterator model_index_last, int_vector::const_iterator first, int_vector::const_iterator last, int offset)
	{
		auto size = last - first; (void)size;

		for (; model_index_first != model_index_last; ++model_index_first)
		{
			const QModelIndex & idx = *model_index_first;
			if (not idx.isValid()) continue;

			auto * pageptr = get_page(idx);
			if (pageptr != &page) continue;

			auto row = idx.row();
			auto col = idx.column();

			if (row < offset) continue;

			assert(row < size); (void)size;
			auto newIdx = create_index(first[row - offset], col, pageptr);
			this->changePersistentIndex(idx, newIdx);
		}
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::inverse_index_array(int_vector & inverse, int_vector::iterator first, int_vector::iterator last, int offset)
	{
		inverse.resize(last - first);
		int i = offset;

		for (auto it = first; it != last; ++it, ++i)
		{
			int val = *it;
			inverse[viewed::unmark_index(val) - offset] = not viewed::marked_index(val) ? i : -1;
		}

		//std::copy(buffer.begin(), buffer.end(), first);
	}

	/************************************************************************/
	/*                    sort/filter support                               */
	/************************************************************************/
	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::merge_newdata(
		value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last, bool resort_old /* = true */)
	{
		if (not viewed::active(m_sorter)) return;

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_indirect_fun(std::move(sorter));

		if (resort_old) varalgo::stable_sort(first, middle, comp);
		varalgo::sort(middle, last, comp);
		varalgo::inplace_merge(first, middle, last, comp);

	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::merge_newdata(
		value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last,
		int_vector::iterator ifirst, int_vector::iterator imiddle, int_vector::iterator ilast, bool resort_old /* = true */)
	{
		if (not viewed::active(m_sorter)) return;

		assert(last - first == ilast - ifirst);
		assert(middle - first == imiddle - ifirst);

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(std::move(sorter)));

		auto zfirst = ext::make_zip_iterator(first, ifirst);
		auto zmiddle = ext::make_zip_iterator(middle, imiddle);
		auto zlast = ext::make_zip_iterator(last, ilast);

		if (resort_old) varalgo::stable_sort(zfirst, zmiddle, comp);
		varalgo::sort(zmiddle, zlast, comp);
		varalgo::inplace_merge(zfirst, zmiddle, zlast, comp);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::stable_sort(value_ptr_iterator first, value_ptr_iterator last)
	{
		if (not viewed::active(m_sorter)) return;

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_indirect_fun(std::move(sorter));
		varalgo::stable_sort(first, last, comp);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::stable_sort(
		value_ptr_iterator first, value_ptr_iterator last,
		int_vector::iterator ifirst, int_vector::iterator ilast)
	{
		if (not viewed::active(m_sorter)) return;

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(std::move(sorter)));

		auto zfirst = ext::make_zip_iterator(first, ifirst);
		auto zlast = ext::make_zip_iterator(last, ilast);
		varalgo::stable_sort(zfirst, zlast, comp);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::sort_and_notify()
	{
		if (not viewed::active(m_sorter)) return;

		resort_context ctx;
		int_vector index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;

		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		this->layoutAboutToBeChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);

		auto indexes = this->persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last = indexes.end();

		sort_and_notify(m_root, ctx);

		this->layoutChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::sort_and_notify(page_type & page, resort_context & ctx)
	{
		auto & container = page.children;
		auto & seq_view = container.template get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		index_array.resize(seq_ptr_view.size());

		auto first = valptr_vector.begin();
		auto middle = first + page.upassed;
		auto last = valptr_vector.end();

		auto ifirst = index_array.begin();
		auto imiddle = ifirst + page.upassed;
		auto ilast = index_array.end();

		std::iota(ifirst, ilast, offset);
		stable_sort(first, middle, ifirst, imiddle);

		seq_view.rearrange(boost::make_transform_iterator(first, detail::make_ref));
		inverse_index_array(inverse_array, ifirst, ilast, offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
					   inverse_array.begin(), inverse_array.end(), offset);

		for_each_child_page(page, [this, &ctx](auto & page) { sort_and_notify(page, ctx); });
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::refilter_and_notify(viewed::refilter_type rtype)
	{
		switch (rtype)
		{
			default:
			case viewed::refilter_type::same:        return;
			
			case viewed::refilter_type::incremental: return refilter_incremental_and_notify();
			case viewed::refilter_type::full:        return refilter_full_and_notify();
		}
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::refilter_incremental_and_notify()
	{
		refilter_context ctx;
		int_vector index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;

		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		this->layoutAboutToBeChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);

		auto indexes = this->persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last = indexes.end();

		refilter_incremental_and_notify(m_root, ctx);

		this->layoutChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::refilter_incremental_and_notify(page_type & page, refilter_context & ctx)
	{
		for_each_child_page(page, [this, &ctx](auto & page) { refilter_incremental_and_notify(page, ctx); });

		auto & container = page.children;
		auto & seq_view = container.template get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		index_array.resize(seq_ptr_view.size());

		auto filter = value_ptr_filter_type(std::cref(m_filter));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto zfpred = viewed::make_get_functor<0>(fpred);

		auto vfirst = valptr_vector.begin();
		auto vlast = vfirst + page.upassed;

		auto ivfirst = index_array.begin();
		auto ivlast = ivfirst + page.upassed;
		auto isfirst = ivlast;
		auto islast = index_array.end();

		std::iota(ivfirst, islast, offset);

		auto[vpp, ivpp] = std::stable_partition(
			ext::make_zip_iterator(vfirst, ivfirst),
			ext::make_zip_iterator(vlast, ivlast),
			zfpred).get_iterator_tuple();

		auto mark_index = [](int idx) { return viewed::mark_index(idx); };
		std::transform(ivpp, ivlast, ivpp, mark_index);

		int upassed_new = vpp - vfirst;
		seq_view.rearrange(boost::make_transform_iterator(vfirst, detail::make_ref));
		page.upassed = upassed_new;

		inverse_index_array(inverse_array, index_array.begin(), index_array.end(), offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
					   inverse_array.begin(), inverse_array.end(), offset);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::refilter_full_and_notify()
	{
		refilter_context ctx;
		int_vector index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;

		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		this->layoutAboutToBeChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);

		auto indexes = this->persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last = indexes.end();

		refilter_full_and_notify(m_root, ctx);

		this->layoutChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
	}
	
	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::refilter_full_and_notify(page_type & page, refilter_context & ctx)
	{
		for_each_child_page(page, [this, &ctx](auto & page) { refilter_full_and_notify(page, ctx); });

		auto & container = page.children;
		auto & seq_view = container.template get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;
		int upassed_new;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		index_array.resize(seq_ptr_view.size());

		auto filter = value_ptr_filter_type(std::cref(m_filter));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto zfpred = viewed::make_get_functor<0>(fpred);

		auto vfirst = valptr_vector.begin();
		auto vlast = vfirst + page.upassed;
		auto sfirst = vlast;
		auto slast = valptr_vector.end();

		auto ivfirst = index_array.begin();
		auto ivlast = ivfirst + page.upassed;
		auto isfirst = ivlast;
		auto islast = index_array.end();

		std::iota(ivfirst, islast, offset);

		if (not viewed::active(m_filter))
		{
			upassed_new = slast - vfirst;
			merge_newdata(vfirst, vlast, slast, ivfirst, ivlast, islast, false);
		}
		else
		{
			auto[vpp, ivpp] = std::stable_partition(
				ext::make_zip_iterator(vfirst, ivfirst),
				ext::make_zip_iterator(vlast, ivlast),
				zfpred).get_iterator_tuple();

			auto[spp, ispp] = std::partition(
				ext::make_zip_iterator(sfirst, isfirst),
				ext::make_zip_iterator(slast, islast),
				zfpred).get_iterator_tuple();

			auto mark_index = [](int idx) { return viewed::mark_index(idx); };
			std::transform(ivpp, ivlast, ivpp, mark_index);
			std::transform(ispp, islast, ispp, mark_index);

			vlast = std::rotate(vpp, sfirst, spp);
			ivlast = std::rotate(ivpp, isfirst, ispp);
			
			upassed_new = vlast - vfirst;
			merge_newdata(vfirst, vpp, vlast, ivfirst, ivpp, ivlast, false);
		}

		seq_view.rearrange(boost::make_transform_iterator(vfirst, detail::make_ref));
		page.upassed = upassed_new;

		inverse_index_array(inverse_array, index_array.begin(), index_array.end(), offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
					   inverse_array.begin(), inverse_array.end(), offset);
	}
}
