#pragma once
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
		std::shared_ptr<container_type> m_owner = nullptr; // pointer to owning container

		/// raii connections
		scoped_connection m_clear_con;
		scoped_connection m_update_con;
		scoped_connection m_erase_con;

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
	/*                     method implementation                            */
	/************************************************************************/
	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::reinit_view()
	{
		using leaf_pointer_vector = std::vector<view_pointer_type>;
		using reset_context = typename base_type::reset_context_template<typename leaf_pointer_vector::iterator>;

		leaf_pointer_vector elements;
		elements.assign(
			boost::make_transform_iterator(m_owner->begin(), get_view_pointer),
			boost::make_transform_iterator(m_owner->end(), get_view_pointer)
		);

		this->beginResetModel();

		m_root.upassed = 0;
		m_root.children.clear();

		value_ptr_vector valptr_array;
		reset_context ctx;

		auto first = elements.begin();
		auto last = elements.end();
		this->group_by_segments(first, last);

		ctx.vptr_array = &valptr_array;
		ctx.first = first;
		ctx.last = last;
		this->reset_page(m_root, ctx);

		this->endResetModel();

	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::update_data(
		const signal_range_type & erased, const signal_range_type & updated, const signal_range_type & inserted)
	{
		using signal_range_iterator = typename signal_range_type::iterator;

		auto erased_first = erased.begin();
		auto erased_last = erased.end();
		auto updated_first = updated.begin();
		auto updated_last = updated.end();
		auto inserted_first = inserted.begin();
		auto inserted_last = inserted.end();

		base_type::group_by_segments(erased_first, erased_last);
		base_type::group_by_segments(updated_first, updated_last);
		base_type::group_by_segments(inserted_first, inserted_last);

		return base_type::update_data_and_notify(
			erased_first, erased_last,
			updated_first, updated_last,
			inserted_first, inserted_last);
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::erase_records(const signal_range_type & erased)
	{
		signal_range_type updated, inserted;
		return update_data(erased, updated, inserted);
	}

	template <class Traits, class Container, class ModelBase>
	void sftree_view_qtbase<Traits, Container, ModelBase>::clear_view()
	{
		this->beginResetModel();
		this->m_root.children.clear();
		this->m_root.upassed = 0;
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
