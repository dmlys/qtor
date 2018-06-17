#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <qtor/sftree_facade_qtbase.hpp>


namespace viewed
{
	template <class Traits, class Container, class ModelBase = QAbstractItemModel>
	class sftree_view_qtbase : public sftree_facade_qtbase<Traits, ModelBase>
	{
		using self_type = sftree_view_qtbase;
		using base_type = sftree_facade_qtbase<Traits, ModelBase>;

	public:
		using container_type = Container;

	protected:
		// store <-> view exchange
		using view_pointer_type = typename container_type::view_pointer_type;
		using view_reference_type = typename container_type::const_reference;
		static_assert(std::is_pointer_v<view_pointer_type>);

		using signal_range_type = typename container_type::signal_range_type;
		using scoped_connection = typename container_type::scoped_connection;

		struct get_view_pointer_type
		{
			view_pointer_type operator()(view_reference_type val) const noexcept { return container_type::get_view_pointer(val); }
		};

		struct get_view_reference_type
		{
			view_reference_type operator()(view_pointer_type ptr) const noexcept { return container_type::get_view_reference(ptr); }
		};

	protected:
		static constexpr get_view_pointer_type get_view_pointer {};
		static constexpr get_view_reference_type get_view_reference {};

	protected:
		// being some typedefs/members from base class into visibility
		using typename base_type::model_helper;
		using typename base_type::int_vector;

		using typename base_type::page_type;
		using typename base_type::leaf_type;
		using typename base_type::node_type;
		using typename base_type::path_type;
		using typename base_type::pathview_type;

		using typename base_type::value_ptr;
		using typename base_type::value_container;
		using typename base_type::code_view_type;
		using typename base_type::seq_view_type;

		using base_type::PAGE;
		using base_type::LEAF;
		using base_type::by_code;
		using base_type::by_seq;


		using typename base_type::leaf_ptr_vector;
		using typename base_type::value_ptr_vector;
		using typename base_type::leaf_ptr_iterator;
		using typename base_type::value_ptr_iterator;

		using typename base_type::get_segment_type;
		using typename base_type::get_children_type;
		using typename base_type::get_children_count_type;

		using base_type::get_segment;
		using base_type::get_children;
		using base_type::get_children_count;

		using base_type::m_root;
		using base_type::m_sorter;
		using base_type::m_filter;

	protected:
		using int_vector_iterator   = typename base_type::int_vector::iterator;
		using signal_range_iterator = typename signal_range_type::iterator;

		struct upsert_context
		{
			signal_range_iterator
				inserted_first, inserted_last,
				updated_first, updated_last,
				erased_first, erased_last;

			int_vector_iterator
				removed_first, removed_last, // share same array, append by incrementing removed_last
				changed_first, changed_last; //                   append by decrementing changed_first

			std::ptrdiff_t inserted_diff, updated_diff, erased_diff;
			std::size_t inserted_count, updated_count, erased_count;

			pathview_type prefix;
			pathview_type inserted_prefix, updated_prefix, erased_prefix;
			path_type inserted_name, updated_name, erased_name;

			value_ptr_vector * vptr_array;
			int_vector * index_array, *inverse_array;
			QModelIndexList::const_iterator model_index_first, model_index_last;
		};

		struct reinit_context
		{
			leaf_ptr_iterator first, last;
			pathview_type prefix;
			value_ptr_vector * vptr_array;
		};

		using typename base_type::value_ptr_filter_type;
		using typename base_type::value_ptr_sorter_type;

	protected:
		std::shared_ptr<container_type> m_owner = nullptr; // pointer to owning container

		/// raii connections
		scoped_connection m_clear_con;
		scoped_connection m_update_con;
		scoped_connection m_erase_con;

	protected:
		// being some methods from base class into visibility
		using base_type::group_by_dirs;

		using base_type::is_subelement;
		using base_type::analyze;

		using base_type::emit_changed;
		using base_type::change_indexes;
		using base_type::inverse_index_array;

		using base_type::merge_newdata;
		using base_type::stable_sort;

	protected:
		static upsert_context copy_context(const upsert_context & ctx, pathview_type newprefix);

		virtual auto process_erased(page_type & page, upsert_context & ctx)->std::tuple<path_type &, pathview_type &>;
		virtual auto process_updated(page_type & page, upsert_context & ctx)->std::tuple<path_type &, pathview_type &>;
		virtual auto process_inserted(page_type & page, upsert_context & ctx)->std::tuple<path_type &, pathview_type &>;
		virtual void rearrange_children(page_type & page, upsert_context & ctx);
		virtual void update_page(page_type & page, upsert_context & ctx);
		virtual void recalculate_page(page_type & page) = 0;

		virtual void reinit_view(page_type & page, reinit_context & ctx);

	protected:
		/// container event handlers, those are called on container signals,
		/// you could reimplement them to provide proper handling of your view

		/// called when new data is updated in owning container
		/// view have to synchronize itself.
		/// @Param erased range of pointers to erased records
		/// @Param updated range of pointers to updated records
		/// @Param inserted range of pointers to inserted
		/// 
		/// default implementation removes erased, appends inserted records, and does nothing with updated
		virtual void update_data(
			const signal_range_type & erased,
			const signal_range_type & updated,
			const signal_range_type & inserted);

		/// called when some records are erased from container
		/// view have to synchronize itself.
		/// @Param erased range of pointers to updated records
		/// 
		/// default implementation, erases those records from main store
		virtual void erase_records(const signal_range_type & erased);

		/// called when container is cleared, clears m_store.
		virtual void clear_view();

		/// connects container signals to appropriate handlers
		virtual void connect_signals();

	public:
		/// returns pointer to owning container
		const auto & get_owner() const noexcept { return m_owner; }

		/// reinitializes view
		/// default implementation just copies from owner
		virtual void reinit_view();

		/// normally should not be called outside of view class.
		/// Provided, when view class used directly without inheritance, to complete initialization.
		/// Calls connects signals and calls reinit_view.
		/// 
		/// Derived views probably will automatically call it constructor
		/// or directly connect_signals/reinit_view
		virtual void init();

	public:
		sftree_view_qtbase(std::shared_ptr<container_type> owner, QObject * parent = nullptr) 
			: base_type(parent), m_owner(std::move(owner)) {}

		virtual ~sftree_view_qtbase() = default;
	};

	/************************************************************************/
	/*              upsert method implementation                            */
	/************************************************************************/
	template <class Traits, class Container, class ModelBase>
	auto sftree_view_qtbase<Traits, Container, ModelBase>::copy_context(const upsert_context & ctx, pathview_type newprefix) -> upsert_context
	{
		upsert_context newctx;
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

	template <class Traits, class Container, class ModelBase>
	auto sftree_view_qtbase<Traits, Container, ModelBase>::process_erased(page_type & page, upsert_context & ctx) -> std::tuple<path_type &, pathview_type &>
	{
		auto & container = page.children;
		auto & seq_view  = container.template get<by_seq>();
		auto & code_view = container.template get<by_code>();

		// consumed nothing from previous step, return same name/prefix
		if (ctx.erased_diff == 0)
			return std::tie(ctx.erased_name, ctx.erased_prefix);

		for (; ctx.erased_first != ctx.erased_last; ++ctx.erased_first)
		{
			const auto * item = *ctx.erased_first;
			std::uintptr_t type;
			std::tie(type, ctx.erased_name, ctx.erased_prefix) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.erased_name, ctx.erased_prefix);

			auto it = container.find(get_segment(item));
			assert(it != container.end());
			
			auto seqit = container.project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*ctx.removed_last++ = pos;

			// erasion will be done later, in rearrange_children
			// container.erase(it);
		}

		ctx.erased_name = path_type();
		ctx.erased_prefix = pathview_type();
		return std::tie(ctx.erased_name, ctx.erased_prefix);
	}

	template <class Traits, class Container, class ModelBase>
	auto sftree_view_qtbase<Traits, Container, ModelBase>::process_updated(page_type & page, upsert_context & ctx) -> std::tuple<path_type &, pathview_type &>
	{
		auto & container = page.children;
		auto & seq_view  = container.template get<by_seq>();
		auto & code_view = container.template get<by_code>();

		// consumed nothing from previous step, return same name/prefix
		if (ctx.updated_diff == 0)
			return std::tie(ctx.updated_name, ctx.updated_prefix);

		for (; ctx.updated_first != ctx.updated_last; ++ctx.updated_first)
		{
			const auto * item = *ctx.updated_first;
			std::uintptr_t type;
			std::tie(type, ctx.updated_name, ctx.updated_prefix) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.updated_name, ctx.updated_prefix);

			auto it = container.find(get_segment(item));
			assert(it != container.end());

			auto seqit = container.project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*--ctx.changed_first = pos;
		}

		ctx.updated_name = path_type();
		ctx.updated_prefix = pathview_type();
		return std::tie(ctx.updated_name, ctx.updated_prefix);
	}

	template <class Traits, class Container, class ModelBase>
	auto sftree_view_qtbase<Traits, Container, ModelBase>::process_inserted(page_type & page, upsert_context & ctx) -> std::tuple<path_type &, pathview_type &>
	{
		auto & container = page.children;
		//auto & seq_view  = container.template get<by_seq>();
		//auto & code_view = container.template get<by_code>();
		
		// consumed nothing from previous step, return same name/prefix
		if (ctx.inserted_diff == 0)
			return std::tie(ctx.inserted_name, ctx.inserted_prefix);

		for (; ctx.inserted_first != ctx.inserted_last; ++ctx.inserted_first)
		{
			const auto * item = *ctx.inserted_first;
			std::uintptr_t type;
			std::tie(type, ctx.inserted_name, ctx.inserted_prefix) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.inserted_name, ctx.inserted_prefix);

			bool inserted;
			std::tie(std::ignore, inserted) = container.insert(item);
			assert(inserted); (void)inserted;	
		}

		ctx.inserted_name = path_type();
		ctx.inserted_prefix = pathview_type();
		return std::tie(ctx.inserted_name, ctx.inserted_prefix);
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::update_page(page_type & page, upsert_context & ctx)
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

			auto is_subelement = [this, &prefix, &name](const auto * item) { return this->is_subelement(prefix, name, *item); };			
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

				child_page->name = name;
				child_page->parent = &page;
				std::tie(it, inserted) = container.insert(std::move(child));
			}			

			// process child
			if (child_page)
			{
				update_page(*child_page, newctx);

				auto seqit = container.project<by_seq>(it);
				auto pos = seqit - seq_view.begin();

				if (child_page->children.size() == 0)
					// actual erasion will be done later in rearrange_children
					*ctx.removed_last++ = pos;
				else if (not inserted)
					*--ctx.changed_first = pos;
			}
		}

		ctx.inserted_count = container.size() - oldsz;
		ctx.updated_count = ctx.changed_last - ctx.changed_first;
		ctx.erased_count = ctx.removed_last - ctx.removed_first;

		rearrange_children(page, ctx);
		recalculate_page(page);
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::rearrange_children(page_type & page, upsert_context & ctx)
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

		auto filter = value_ptr_filter_type(std::cref(m_filter));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto index_pass_pred = [vfirst, fpred](int index) { return fpred(vfirst[index]); };


		auto vchanged_first = ctx.changed_first;
		auto vchanged_last = std::partition(ctx.changed_first, ctx.changed_last,
			[upassed = page.upassed](int index) { return index < upassed; });

		auto vchanged_pp = viewed::active(m_filter) 
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

		if (not viewed::active(m_filter))
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

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::update_data(
		const signal_range_type & erased, const signal_range_type & updated, const signal_range_type & inserted)
	{
		int_vector affected_indexes, index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;
		//leaf_ptr_vector updated_vec, inserted_vec, erased_vec;
		
		affected_indexes.resize(erased.size() + std::max(updated.size(), inserted.size()));
		//updated_vec.assign(updated.begin(), updated.end());
		//erased_vec.assign(erased.begin(), erased.end());
		//inserted_vec.assign(inserted.begin(), inserted.end());
		
		upsert_context ctx;
		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		//ctx.erased_first = erased_vec.begin();
		//ctx.erased_last  = erased_vec.end();
		//ctx.inserted_first = inserted_vec.begin();
		//ctx.inserted_last = inserted_vec.end();
		//ctx.updated_first = updated_vec.begin();
		//ctx.updated_last = updated_vec.end();

		ctx.erased_first = erased.begin();
		ctx.erased_last  = erased.end();
		ctx.inserted_first = inserted.begin();
		ctx.inserted_last = inserted.end();
		ctx.updated_first = updated.begin();
		ctx.updated_last = updated.end();

		ctx.removed_first = ctx.removed_last = affected_indexes.begin();
		ctx.changed_first = ctx.changed_last = affected_indexes.end();

		group_by_dirs(ctx.erased_first, ctx.erased_last);
		group_by_dirs(ctx.inserted_first, ctx.inserted_last);
		group_by_dirs(ctx.updated_first, ctx.updated_last);
		
		this->layoutAboutToBeChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
		
		auto indexes = this->persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last  = indexes.end();

		update_page(m_root, ctx);

		this->layoutChanged(model_helper::empty_model_list, model_helper::NoLayoutChangeHint);
	}

	/************************************************************************/
	/*                 Others method implementation                         */
	/************************************************************************/
	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::reinit_view()
	{
		this->beginResetModel();

		m_root.upassed = 0;
		m_root.children.clear();

		value_ptr_vector valptr_array;
		reinit_context ctx;

		leaf_ptr_vector elements;
		elements.assign(
			boost::make_transform_iterator(m_owner->begin(), get_view_pointer),
			boost::make_transform_iterator(m_owner->end(), get_view_pointer)
		);

		auto first = elements.begin();
		auto last = elements.end();
		group_by_dirs(first, last);

		ctx.vptr_array = &valptr_array;
		ctx.first = first;
		ctx.last = last;
		reinit_view(m_root, ctx);

		this->endResetModel();
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::reinit_view(page_type & page, reinit_context & ctx)
	{
		auto & container = page.children;
		auto & seq_view = container.template get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;

		while (ctx.first != ctx.last)
		{
			const auto * item_ptr = *ctx.first;
			auto[type, name, newprefix] = analyze(ctx.prefix, *item_ptr);
			if (type == LEAF)
			{
				auto * leaf = item_ptr;
				container.insert(leaf);
				++ctx.first;
			}
			else // PAGE
			{
				auto page_ptr = std::make_unique<page_type>();
				page_ptr->parent = &page;
				page_ptr->name = name;

				auto newctx = ctx;
				newctx.prefix = std::move(newprefix);
				newctx.first = ctx.first;

				auto issub = [this, &prefix = ctx.prefix, &name](const auto * item) { return this->is_subelement(prefix, name, *item); };
				newctx.last = std::find_if_not(ctx.first, ctx.last, issub);
				reinit_view(*page_ptr, newctx);

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

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::erase_records(const signal_range_type & erased)
	{
		signal_range_type inserted, updated;
		update_data(erased, updated, inserted);
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::clear_view()
	{
		this->beginResetModel();
		m_root.children.clear();
		this->endResetModel();
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::init()
	{
		connect_signals();
		reinit_view();
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::connect_signals()
	{
		m_clear_con  = m_owner->on_clear([this] { clear_view(); });
		m_update_con = m_owner->on_update([this](const signal_range_type & e, const signal_range_type & u, const signal_range_type & i) { update_data(e, u, i); });
		m_erase_con  = m_owner->on_erase([this](const signal_range_type & r) { erase_records(r); });
	}
}
