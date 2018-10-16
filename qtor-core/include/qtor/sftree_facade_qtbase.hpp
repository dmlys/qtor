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

#include <varalgo/sorting_algo.hpp>
#include <varalgo/on_sorted_algo.hpp>

#include <ext/utility.hpp>
#include <ext/try_reserve.hpp>
#include <ext/iterator/zip_iterator.hpp>
#include <ext/iterator/outdirect_iterator.hpp>
#include <ext/iterator/indirect_iterator.hpp>
#include <ext/range/range_traits.hpp>
#include <ext/range/adaptors/moved.hpp>
#include <ext/range/adaptors/outdirected.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <QtCore/QAbstractItemModel>
#include <QtTools/ToolsBase.hpp>

#if BOOST_COMP_GNUC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

namespace viewed
{
	inline namespace sftree_constants
	{
		constexpr unsigned PAGE = 0;
		constexpr unsigned LEAF = 1;
	}

	/// sftree_facade_qtbase is a facade for building qt tree models.
	/// It expects source data to be in form of a list and provides hierarchical view on it.
	/// It implements complex stuff:
	/// * internal tree structure;
	/// * sorting/filtering;
	/// * QAbstractItemModel stuff: index calculation, persistent index management on updates and sorting/filtering;
	///
	/// ModelBase - QAbstractItemModel derived base interface/class this facade implements.
	/// Traits    - traits class describing type and work with this type.
	///
	/// Traits should provide next:
	///   using leaf_type = ...
	///    leaf, this is original type, sort of value_type. it will be provided to this facade in some sort of list and tree hierarchy will be built.
	///    it can be simple struct or a more complex class.
	///
	///   using node_type = ...
	///    node, similar to leaf, but can have other nodes/leaf children, can be same leaf, or different type
	///    (it does not hold children directly, sftree_facade_qtbase takes care of that internally).
	///    It will be created internally while calculating tree structure.
	///    sftree_facade_qtbase will provide some methods/hooks to populate/calculate values of this type
	///
	///   using path_type = ...
	///     type representing path, copy-constructable, for example std::string, std::filesystem::path, etc
	///     each leaf holds path in some way: direct member, accessed via some getter, or even leaf itself is a path.
	///     path describes hierarchy structure in some, sufficient, way. For example filesystem path: /segment1/segment2/...
	///
	///   using pathview_type = ...  path view, it's same to path, as std::string_view same to std::string. can be same as path_type.
	///
	///   using path_equal_to   = ... predicate for comparing paths
	///   using path_less       = ... predicate for comparing paths
	///   using path_hash       = ... path hasher
	///
	///   methods for working with given types:
	///     void set_segment(node_type & node, path_type && prefix, path_type && segment);
	///       assigns path segment into node, path is given as prefix + segment.
	///       In fact node can hold only given segment part, or construct path from both: prefix and segment.
	///       possible implementation: item.name = segment
	///
	///     path_type get_segment(const leaf_type & leaf);
	///     path_type get_segment(const node_type & node);
	///       returns/extracts segment from leaf/node, usually something like: return extract_segment(leaf/node.name)
	///
	///     path_type get_path(const leaf_type & leaf);
	///       returns path from leaf, usually something like: return leaf.filepath
	///
	///     bool bool is_subelement(const pathview_type & prefix, const path_type & name, const torrent_file & item)
	///     auto analyze(const pathview_type & prefix, const leaf_type & item) -> std::tuple<std::uintptr_t, pathview_type, path_type>;
	///                                                                                   PAGE/LEAF      newprefix   segment
	///
	///
	///   using sort_pred_type = ...
	///   using filter_pred_type = ...
	///
	template <class Traits, class ModelBase = QAbstractItemModel>
	class sftree_facade_qtbase : 
		public ModelBase,
		protected Traits
	{
		using self_type = sftree_facade_qtbase;
		using traits_type = Traits;

		static_assert(std::is_base_of_v<QAbstractItemModel, ModelBase>);

	protected:
		using model_base = ModelBase;
		using model_helper = AbstractItemModel;
		using int_vector = std::vector<int>;
		using int_vector_iterator = int_vector::iterator;

		struct page_type;
		using typename traits_type::leaf_type;
		using typename traits_type::node_type;

		using typename traits_type::path_type;
		using typename traits_type::pathview_type;
		using typename traits_type::path_equal_to;
		using typename traits_type::path_less;
		using typename traits_type::path_hasher;

		using traits_type::analyze;
		using traits_type::is_subelement;

		using typename traits_type::sort_pred_type;
		using typename traits_type::filter_pred_type;

		using value_ptr = viewed::pointer_variant<const page_type *, const leaf_type *>;

		using value_ptr_vector   = std::vector<const value_ptr *>;
		using value_ptr_iterator = typename value_ptr_vector::iterator;

		struct get_segment_type
		{
			using result_type = path_type;
			result_type operator()(const path_type & path) const { return traits_type::get_segment(path); }
			result_type operator()(const leaf_type & leaf) const { return traits_type::get_segment(leaf); }
			result_type operator()(const page_type & page) const { return traits_type::get_segment(static_cast<const node_type &>(page)); }
			result_type operator()(const value_ptr & val)  const { return viewed::visit(*this, val); }

			// important, viewed::visit(*this, val) depends on them, otherwise infinite recursion would occur
			result_type operator()(const leaf_type * leaf) const { return traits_type::get_segment(*leaf); }
			result_type operator()(const page_type * page) const { return traits_type::get_segment(*page); }
		};

		struct segment_group_pred_type : private traits_type::path_less
		{
			// arguments - swapped, intended, sort in descending order
			bool operator()(const leaf_type & l1, const leaf_type & l2) const noexcept { return path_less::operator ()(traits_type::get_path(l2), traits_type::get_path(l1)); }
			bool operator()(const leaf_type * l1, const leaf_type * l2) const noexcept { return path_less::operator ()(traits_type::get_path(*l2), traits_type::get_path(*l1)); }
		};

		using value_container = boost::multi_index_container<
			value_ptr,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<get_segment_type, path_hasher, path_equal_to>,
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
			result_type operator()(const value_ptr & val)  const { return viewed::visit(*this, val); }

			// important, viewed::visit(*this, val) depends on them, otherwise infinite recursion would occur
			template <class Type>
			result_type operator()(const Type * ptr) const { return operator()(*ptr); }
		};

		struct get_children_count_type
		{
			using result_type = std::size_t;
			result_type operator()(const leaf_type & leaf) const { return 0; }
			result_type operator()(const page_type & page) const { return page.upassed; }
			result_type operator()(const value_ptr & val)  const { return viewed::visit(*this, val); }

			// important, viewed::visit(*this, val) depends on them, otherwise infinite recursion would occur
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

		template <class ErasedRandomAccessIterator, class UpdatedRandomAccessIterator, class InsertedRandomAccessIterator>
		struct update_context_template
		{
			// all should be groped by group_by_segments
			ErasedRandomAccessIterator erased_first, erased_last;
			UpdatedRandomAccessIterator updated_first, updated_last;
			InsertedRandomAccessIterator inserted_first, inserted_last;

			int_vector_iterator
				removed_first, removed_last, // share same array, append by incrementing removed_last
				changed_first, changed_last; //                   append by decrementing changed_first

			std::ptrdiff_t inserted_diff, updated_diff, erased_diff;
			std::size_t inserted_count, updated_count, erased_count;

			pathview_type prefix;
			pathview_type inserted_prefix, updated_prefix, erased_prefix;
			pathview_type inserted_name, updated_name, erased_name;

			value_ptr_vector * vptr_array;
			int_vector * index_array, *inverse_array;
			QModelIndexList::const_iterator model_index_first, model_index_last;
		};

		template <class RandomAccessIterator>
		struct reset_context_template
		{
			RandomAccessIterator first, last;
			pathview_type prefix;
			value_ptr_vector * vptr_array;
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
				return get_children_count(v) > 0 or viewed::visit(pred, v);
			}

			explicit operator bool() const noexcept
			{
				return viewed::active(pred);
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
				return viewed::visit(pred, v1, v2);
			}

			explicit operator bool() const noexcept
			{
				return viewed::active(pred);
			}
		};

	protected:
		static constexpr get_segment_type        get_segment {};
		static constexpr get_children_type       get_children {};
		static constexpr get_children_count_type get_children_count {};
		static constexpr segment_group_pred_type segment_group_pred {};

	protected:
		static const value_container ms_empty_container;

	protected:
		page_type m_root; //= { /*.parent =*/ nullptr};

		sort_pred_type m_sort_pred;
		filter_pred_type m_filter_pred;

	protected:
		template <class Functor>
		static void for_each_child_page(page_type & page, Functor && func);

		template <class RandomAccessIterator>
		static void group_by_segments(RandomAccessIterator first, RandomAccessIterator last);

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
		virtual void recalculate_page(page_type & page) = 0;

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
		
		/// sorts m_store's [first; last) with m_sort_pred, stable sort
		virtual void stable_sort(value_ptr_iterator first, value_ptr_iterator last);
		/// sorts m_store's [first; last) with m_sort_pred, stable sort
		/// range [ifirst; ilast) must be permuted the same way as range [first; last)
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

	protected:
		template <class update_context> static update_context copy_context(const update_context & ctx, pathview_type newprefix);

		template <class update_context> auto process_erased(page_type & page, update_context & ctx)   -> std::tuple<pathview_type &, pathview_type &>;
		template <class update_context> auto process_updated(page_type & page, update_context & ctx)  -> std::tuple<pathview_type &, pathview_type &>;
		template <class update_context> auto process_inserted(page_type & page, update_context & ctx) -> std::tuple<pathview_type &, pathview_type &>;
		template <class update_context> void rearrange_children_and_notify(page_type & page, update_context & ctx);
		template <class update_context> void update_page_and_notify(page_type & page, update_context & ctx);
		
		template <class reset_context> void reset_page(page_type & page, reset_context & ctx);

	protected:
		template <class ErasedRandomAccessIterator, class UpdatedRandomAccessIterator, class InsertedRandomAccessIterator>
		void update_data_and_notify(
			ErasedRandomAccessIterator erased_first, ErasedRandomAccessIterator erased_last,
			UpdatedRandomAccessIterator updated_first, UpdatedRandomAccessIterator updated_last,
			InsertedRandomAccessIterator inserted_first, InsertedRandomAccessIterator inserted_last);

	public:
		const auto & sort_pred()   const { return m_sort_pred; }
		const auto & filter_pred() const { return m_filter_pred; }

		template <class ... Args> auto filter_by(Args && ... args) -> refilter_type;
		template <class ... Args> void sort_by(Args && ... args);

	public:
		using model_base::model_base;
		virtual ~sftree_facade_qtbase() = default;
	};

	/************************************************************************/
	/*                  Methods implementations                             */
	/************************************************************************/
	template <class Traits, class ModelBase>
	const typename sftree_facade_qtbase<Traits, ModelBase>::value_container sftree_facade_qtbase<Traits, ModelBase>::ms_empty_container;

	namespace detail
	{
		const auto make_ref = [](auto * ptr) { return std::ref(*ptr); };
	}

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
	void sftree_facade_qtbase<Traits, ModelBase>::group_by_segments(RandomAccessIterator first, RandomAccessIterator last)
	{
		std::sort(first, last, segment_group_pred);
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
		if (not viewed::active(m_sort_pred)) return;

		auto sorter = value_ptr_sorter_type(m_sort_pred);
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
		if (not viewed::active(m_sort_pred)) return;

		assert(last - first == ilast - ifirst);
		assert(middle - first == imiddle - ifirst);

		auto sorter = value_ptr_sorter_type(m_sort_pred);
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
		if (not viewed::active(m_sort_pred)) return;

		auto sorter = value_ptr_sorter_type(m_sort_pred);
		auto comp = viewed::make_indirect_fun(std::move(sorter));
		varalgo::stable_sort(first, last, comp);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::stable_sort(
		value_ptr_iterator first, value_ptr_iterator last,
		int_vector::iterator ifirst, int_vector::iterator ilast)
	{
		if (not viewed::active(m_sort_pred)) return;

		auto sorter = value_ptr_sorter_type(m_sort_pred);
		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(std::move(sorter)));

		auto zfirst = ext::make_zip_iterator(first, ifirst);
		auto zlast = ext::make_zip_iterator(last, ilast);
		varalgo::stable_sort(zfirst, zlast, comp);
	}

	template <class Traits, class ModelBase>
	void sftree_facade_qtbase<Traits, ModelBase>::sort_and_notify()
	{
		if (not viewed::active(m_sort_pred)) return;

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

		auto filter = value_ptr_filter_type(std::cref(m_filter_pred));
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

		std::transform(ivpp, ivlast, ivpp, viewed::mark_index);

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

		auto filter = value_ptr_filter_type(std::cref(m_filter_pred));
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

		if (not viewed::active(m_filter_pred))
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

	template <class Traits, class ModelBase>
	template <class ... Args>
	auto sftree_facade_qtbase<Traits, ModelBase>::filter_by(Args && ... args) -> viewed::refilter_type
	{
		auto rtype = m_filter_pred.set_expr(std::forward<Args>(args)...);
		refilter_and_notify(rtype);

		return rtype;
	}

	template <class Traits, class ModelBase>
	template <class ... Args>
	void sftree_facade_qtbase<Traits, ModelBase>::sort_by(Args && ... args)
	{
		m_sort_pred = sort_pred_type(std::forward<Args>(args)...);
		sort_and_notify();
	}

	/************************************************************************/
	/*               reset_data methods implementation                      */
	/************************************************************************/	 
	template <class Traits, class ModelBase>
	template <class reset_context>
	void sftree_facade_qtbase<Traits, ModelBase>::reset_page(page_type & page, reset_context & ctx)
	{
		auto & container = page.children;
		auto & seq_view = container.template get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;

		while (ctx.first != ctx.last)
		{
			auto && item_ptr = *ctx.first;
			auto [type, newprefix, name] = analyze(ctx.prefix, *item_ptr);
			if (type == LEAF)
			{
				container.insert(std::forward<decltype(item_ptr)>(item_ptr));
				++ctx.first;
			}
			else // PAGE
			{
				auto newctx = ctx;
				newctx.prefix = std::move(newprefix);
				newctx.first = ctx.first;

				auto issub = [this, &prefix = ctx.prefix, &name](const auto * item) { return this->is_subelement(prefix, name, *item); };
				newctx.last = std::find_if_not(ctx.first, ctx.last, issub);

				auto page_ptr = std::make_unique<page_type>();
				page_ptr->parent = &page;
				traits_type::set_segment(*page_ptr, std::move(ctx.prefix), std::move(name));

				reset_page(*page_ptr, newctx);

				container.insert(std::move(page_ptr));
				ctx.first = newctx.last;
			}
		}

		page.upassed = container.size();
		value_ptr_vector & refs = *ctx.vptr_array;
		refs.assign(seq_ptr_view.begin(), seq_ptr_view.end());

		auto refs_first = refs.begin();
		auto refs_last = refs.end();
		stable_sort(refs_first, refs_last);

		seq_view.rearrange(boost::make_transform_iterator(refs_first, detail::make_ref));
	}

	/************************************************************************/
	/*               update_data methods implementation                     */
	/************************************************************************/
	template <class Traits, class ModelBase>
	template <class update_context>
	auto sftree_facade_qtbase<Traits, ModelBase>::copy_context(const update_context & ctx, pathview_type newprefix) -> update_context
	{
		update_context newctx;
		newctx.inserted_first = ctx.inserted_first;
		newctx.inserted_last = ctx.inserted_last;
		newctx.updated_first = ctx.updated_first;
		newctx.updated_last = ctx.updated_last;
		newctx.erased_first = ctx.erased_first;
		newctx.erased_last = ctx.erased_last;

		newctx.changed_first = newctx.changed_last = ctx.changed_first;
		newctx.removed_last = newctx.removed_first = ctx.removed_last;

		newctx.prefix = std::move(newprefix);

		newctx.vptr_array = ctx.vptr_array;
		newctx.index_array = ctx.index_array;
		newctx.inverse_array = ctx.inverse_array;

		newctx.model_index_first = ctx.model_index_first;
		newctx.model_index_last = ctx.model_index_last;

		return newctx;
	}

	template <class Traits, class ModelBase>
	template <class update_context>
	auto sftree_facade_qtbase<Traits, ModelBase>::process_erased(page_type & page, update_context & ctx) -> std::tuple<pathview_type &, pathview_type &>
	{
		auto & container = page.children;
		auto & seq_view  = container.template get<by_seq>();
		auto & code_view = container.template get<by_code>();

		// consumed nothing from previous step, return same name/prefix
		if (ctx.erased_diff == 0)
			return std::tie(ctx.erased_name, ctx.erased_prefix);

		for (; ctx.erased_first != ctx.erased_last; ++ctx.erased_first)
		{
			auto && item = *ctx.erased_first;
			std::uintptr_t type;
			std::tie(type, ctx.erased_prefix, ctx.erased_name) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.erased_name, ctx.erased_prefix);

			auto it = container.find(get_segment(*item));
			assert(it != container.end());
			
			auto seqit = container.template project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*ctx.removed_last++ = pos;

			// erasion will be done later, in rearrange_children_and_notify
			// container.erase(it);
		}

		ctx.erased_name = pathview_type();
		ctx.erased_prefix = pathview_type();
		return std::tie(ctx.erased_name, ctx.erased_prefix);
	}

	template <class Traits, class ModelBase>
	template <class update_context>
	auto sftree_facade_qtbase<Traits, ModelBase>::process_updated(page_type & page, update_context & ctx) -> std::tuple<pathview_type &, pathview_type &>
	{
		auto & container = page.children;
		auto & seq_view  = container.template get<by_seq>();
		auto & code_view = container.template get<by_code>();

		// consumed nothing from previous step, return same name/prefix
		if (ctx.updated_diff == 0)
			return std::tie(ctx.updated_name, ctx.updated_prefix);

		for (; ctx.updated_first != ctx.updated_last; ++ctx.updated_first)
		{
			auto && item = *ctx.updated_first;
			std::uintptr_t type;
			std::tie(type, ctx.updated_prefix, ctx.updated_name) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.updated_name, ctx.updated_prefix);

			auto it = container.find(get_segment(*item));
			assert(it != container.end());

			auto seqit = container.template project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*--ctx.changed_first = pos;
			
			value_ptr & val = ext::unconst(*it);
			val = std::forward<decltype(item)>(item);
		}

		ctx.updated_name = pathview_type();
		ctx.updated_prefix = pathview_type();
		return std::tie(ctx.updated_name, ctx.updated_prefix);
	}

	template <class Traits, class ModelBase>
	template <class update_context>
	auto sftree_facade_qtbase<Traits, ModelBase>::process_inserted(page_type & page, update_context & ctx) -> std::tuple<pathview_type &, pathview_type &>
	{
		auto & container = page.children;
		//auto & seq_view  = container.template get<by_seq>();
		//auto & code_view = container.template get<by_code>();
		
		// consumed nothing from previous step, return same name/prefix
		if (ctx.inserted_diff == 0)
			return std::tie(ctx.inserted_name, ctx.inserted_prefix);

		for (; ctx.inserted_first != ctx.inserted_last; ++ctx.inserted_first)
		{
			auto && item = *ctx.inserted_first;
			std::uintptr_t type;
			std::tie(type, ctx.inserted_prefix, ctx.inserted_name) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.inserted_name, ctx.inserted_prefix);

			bool inserted;
			std::tie(std::ignore, inserted) = container.insert(std::forward<decltype(item)>(item));
			assert(inserted); (void)inserted;	
		}

		ctx.inserted_name = pathview_type();
		ctx.inserted_prefix = pathview_type();
		return std::tie(ctx.inserted_name, ctx.inserted_prefix);
	}

	template <class Traits, class ModelBase>
	template <class update_context>
	void sftree_facade_qtbase<Traits, ModelBase>::update_page_and_notify(page_type & page, update_context & ctx)
	{
		const auto & prefix = ctx.prefix;
		auto & container = page.children;
		auto & seq_view  = container.template get<by_seq>();
		auto & code_view = container.template get<by_code>();
		auto oldsz = container.size();
		ctx.inserted_diff = ctx.updated_diff = ctx.erased_diff = -1;

		for (;;)
		{
			process_inserted(page, ctx);
			process_updated(page, ctx);
			process_erased(page, ctx);

			// at this point only pages are at front of ranges
			auto newprefix = std::max({ctx.erased_prefix, ctx.updated_prefix, ctx.inserted_prefix});
			auto name = std::max({ctx.erased_name, ctx.updated_name, ctx.inserted_name});
			if (name.isEmpty()) break;

			auto newctx = copy_context(ctx, std::move(newprefix));

			auto is_subelement = [this, &prefix, &name](auto && item) { return this->is_subelement(prefix, name, *item); };
			ctx.inserted_first = std::find_if_not(ctx.inserted_first, ctx.inserted_last, is_subelement);
			ctx.updated_first  = std::find_if_not(ctx.updated_first,  ctx.updated_last,  is_subelement);
			ctx.erased_first   = std::find_if_not(ctx.erased_first,   ctx.erased_last,   is_subelement);

			newctx.inserted_last = ctx.inserted_first;
			newctx.updated_last  = ctx.updated_first;
			newctx.erased_last   = ctx.erased_first;

			ctx.inserted_diff = newctx.inserted_last - newctx.inserted_first;
			ctx.updated_diff  = newctx.updated_last  - newctx.updated_first;
			ctx.erased_diff   = newctx.erased_last   - newctx.erased_first;
			assert(ctx.inserted_diff or ctx.updated_diff or ctx.erased_diff);

			page_type * child_page = nullptr;
			bool inserted = false;
			auto it = container.find(name);
			if (it != container.end())
				child_page = static_cast<page_type *>(it->pointer());
			else 
			{
				assert(ctx.updated_diff or ctx.inserted_diff);
				auto child = std::make_unique<page_type>();
				child_page = child.get();

				traits_type::set_segment(*child_page, std::move(prefix), std::move(name));
				child_page->parent = &page;
				std::tie(it, inserted) = container.insert(std::move(child));
			}			

			// process child
			if (child_page)
			{
				update_page_and_notify(*child_page, newctx);

				auto seqit = container.template project<by_seq>(it);
				auto pos = seqit - seq_view.begin();

				if (child_page->children.size() == 0)
					// actual erasion will be done later in rearrange_children_and_notify
					*ctx.removed_last++ = pos;
				else if (not inserted)
					*--ctx.changed_first = pos;
			}
		}

		ctx.inserted_count = container.size() - oldsz;
		ctx.updated_count = ctx.changed_last - ctx.changed_first;
		ctx.erased_count = ctx.removed_last - ctx.removed_first;

		rearrange_children_and_notify(page, ctx);
		recalculate_page(page);
	}

	template <class Traits, class ModelBase>
	template <class update_context>
	void sftree_facade_qtbase<Traits, ModelBase>::rearrange_children_and_notify(page_type & page, update_context & ctx)
	{
		auto & container = page.children;
		auto & seq_view = container.template get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;
		int upassed_new;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		auto vfirst = valptr_vector.begin();
		auto vlast = vfirst + page.upassed;
		auto sfirst = vlast;
		auto slast = vfirst + (seq_ptr_view.size() - ctx.inserted_count);
		auto nfirst = slast;
		auto nlast = valptr_vector.end();

		index_array.resize(container.size());
		auto ifirst = index_array.begin();
		auto imiddle = ifirst + page.upassed;
		auto ilast  = index_array.end();
		std::iota(ifirst, ilast, offset);

		auto filter = value_ptr_filter_type(std::cref(m_filter_pred));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto index_pass_pred = [vfirst, fpred](int index) { return fpred(vfirst[index]); };


		auto vchanged_first = ctx.changed_first;
		auto vchanged_last = std::partition(ctx.changed_first, ctx.changed_last,
			[upassed = page.upassed](int index) { return index < upassed; });

		auto vchanged_pp = viewed::active(m_filter_pred) 
			? std::partition(vchanged_first, vchanged_last, index_pass_pred)
			: vchanged_last;

		auto schanged_first = vchanged_last;
		auto schanged_last = ctx.changed_last;

		// mark removed ones by nullifying them
		for (auto it = ctx.removed_first; it != ctx.removed_last; ++it)
		{
			int index = *it;
			vfirst[index] = nullptr;
			ifirst[index] = -1;
		}

		// mark removed ones from [vfirst, vlast) by nullifying them
		for (auto it = vchanged_pp; it != vchanged_last; ++it)
		{
			int index = *it;
			vfirst[index] = nullptr;
			ifirst[index] = -1;
		}

		if (not viewed::active(m_filter_pred))
		{
			// remove erased ones, and filtered out ones
			vlast  = std::remove(vfirst, vlast, nullptr);
			sfirst = std::remove(std::make_reverse_iterator(slast), std::make_reverse_iterator(sfirst), nullptr).base();
			nlast  = std::move(sfirst, nlast, vlast);
			upassed_new = nlast - vfirst;
		}
		else
		{
			for (auto it = schanged_first; it != schanged_last; ++it)
				vfirst[*it] = mark_pointer(vfirst[*it]);

			vlast  = std::remove_if(vfirst, vlast, [](auto ptr) { return unmark_pointer(ptr) == nullptr; });
			sfirst = std::remove(std::make_reverse_iterator(slast), std::make_reverse_iterator(sfirst), nullptr).base();

			// [spp, npp) - gathered elements from [sfirst, nlast) satisfying fpred
			auto spp = std::partition(sfirst, slast, [](auto * ptr) { return not marked_pointer(ptr); });
			auto npp = std::partition(nfirst, nlast, fpred);
			upassed_new = (vlast - vfirst) + (npp - spp);

			for (auto it = spp; it != slast; ++it)
				*it = unmark_pointer(*it);

			// rotate them at the beginning of shadow area
			// and in fact merge those with visible area
			std::rotate(sfirst, spp, nlast);
			nlast = std::move(sfirst, nlast, vlast);
		}

		auto rfirst = nlast;
		auto rlast = rfirst;

		{
			auto point = imiddle;
			imiddle = std::remove(ifirst, imiddle, -1);
			ilast = std::remove(point, ilast, -1);			
			ilast = std::move(point, ilast, imiddle);
		}
		

		for (auto it = vchanged_pp; it != vchanged_last; ++it)
		{
			int index = *it;
			*rlast++ = seq_ptr_view[index];
			*ilast++ = viewed::mark_index(index);
		}

		for (auto it = ctx.removed_first; it != ctx.removed_last; ++it)
		{
			int index = *it;
			*rlast++ = seq_ptr_view[index];
			*ilast++ = viewed::mark_index(index);
		}

		bool resort_old = vchanged_first != vchanged_pp;
		merge_newdata(vfirst, vlast, nlast, ifirst, imiddle, ifirst + (nlast - vfirst), resort_old);

		seq_view.rearrange(boost::make_transform_iterator(vfirst, detail::make_ref));
		seq_view.resize(seq_view.size() - ctx.erased_count);
		page.upassed = upassed_new;
		
		inverse_index_array(inverse_array, ifirst, ilast, offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
		               inverse_array.begin(), inverse_array.end(), offset);
	}

	template <class Traits, class ModelBase>
	template <class ErasedRandomAccessIterator, class UpdatedRandomAccessIterator, class InsertedRandomAccessIterator>
	void sftree_facade_qtbase<Traits, ModelBase>::update_data_and_notify(
		ErasedRandomAccessIterator erased_first, ErasedRandomAccessIterator erased_last,
		UpdatedRandomAccessIterator updated_first, UpdatedRandomAccessIterator updated_last,
		InsertedRandomAccessIterator inserted_first, InsertedRandomAccessIterator inserted_last)
	{
		using update_context = update_context_template<ErasedRandomAccessIterator, UpdatedRandomAccessIterator, InsertedRandomAccessIterator>;
		int_vector affected_indexes, index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;
		//leaf_ptr_vector updated_vec, inserted_vec, erased_vec;
		
		auto expected_indexes = erased_last - erased_first + std::max(updated_last - updated_first, inserted_last - inserted_first);
		affected_indexes.resize(expected_indexes);
		//updated_vec.assign(updated.begin(), updated.end());
		//erased_vec.assign(erased.begin(), erased.end());
		//inserted_vec.assign(inserted.begin(), inserted.end());
		
		update_context ctx;
		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		//ctx.erased_first = erased_vec.begin();
		//ctx.erased_last  = erased_vec.end();
		//ctx.inserted_first = inserted_vec.begin();
		//ctx.inserted_last = inserted_vec.end();
		//ctx.updated_first = updated_vec.begin();
		//ctx.updated_last = updated_vec.end();

		ctx.erased_first = erased_first;
		ctx.erased_last  = erased_last;
		ctx.updated_first = updated_first;
		ctx.updated_last = updated_last;
		ctx.inserted_first = inserted_first;
		ctx.inserted_last = inserted_last;

		ctx.removed_first = ctx.removed_last = affected_indexes.begin();
		ctx.changed_first = ctx.changed_last = affected_indexes.end();

		auto pred = viewed::make_indirect_fun(segment_group_pred);
		assert(std::is_sorted(ctx.erased_first, ctx.erased_last, pred));
		assert(std::is_sorted(ctx.updated_first, ctx.updated_last, pred));
		assert(std::is_sorted(ctx.inserted_first, ctx.inserted_last, pred));
		
		this->layoutAboutToBeChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
		
		auto indexes = this->persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last  = indexes.end();

		this->update_page_and_notify(m_root, ctx);

		this->layoutChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
	}
}

#if BOOST_COMP_GNUC
#pragma GCC diagnostic pop
#endif
