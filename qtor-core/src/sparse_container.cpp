#include <algorithm>
#include <ext/utility.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>

#include <QtCore/QChar>
#include <qtor/sparse_container.hpp>

namespace qtor
{
	const sparse_container::any_type sparse_container::ms_empty;

	bool sparse_container_comparator::operator()(const sparse_container & c1, const sparse_container & c2) const
	{
		const auto & v1 = c1.get_item(m_key);
		const auto & v2 = c2.get_item(m_key);

		return m_ascending ? v1 < v2 : v2 < v1;
	}

	static std::uint32_t toupper(std::uint32_t ch)
	{
#if 0
		if ('a' <= ch && ch <= 'z')
			return ch + 'A' - 'a';

		// \U0430 - 'a', \U0410 - 'A', \U044F - 'я'
		//  L'a'   <= ch && ch <= L'я'
		if (0x0430 <= ch && ch <= 0x044F)
			return ch + 0x0410 - 0x0430; // ch + L'А' - L'а'

		return ch;
#endif
		return QChar::toUpper(ch);
	}

	static void trim(QString & str)
	{
		str = str.trimmed();
	}

	static bool istarts_with(const QString & str, const QString & test)
	{
		return str.startsWith(test, Qt::CaseInsensitive);
	}

	static bool iequals(const QString & str, const QString & test)
	{
		return str.compare(test, Qt::CaseInsensitive) == 0;
	}

	static bool icontains(const QString & str, const QString & search)
	{
		return str.contains(search, Qt::CaseInsensitive);
	}

	static bool empty(const QString & str)
	{
		return str.isEmpty();
	}

	static void trim(std::string & str)
	{
		auto * pfirst = ext::unconst(str.data());
		auto * plast = pfirst + str.size();

		typedef boost::u8_to_u32_iterator<const char *, std::uint32_t> utf_iter;
		utf_iter first {pfirst}, last {plast};

		auto notspace = [](auto ch) { return not QChar::isSpace(ch); };
		auto new_first = std::find_if(first, last, notspace);

		auto new_last = std::find_if(
			std::make_reverse_iterator(last),
			std::make_reverse_iterator(new_first),
			notspace).base();

		auto stopped = std::move(new_first.base(), new_last.base(), pfirst);
		str.erase(stopped - pfirst, plast - stopped);
	}

	static bool istarts_with(const std::string & str, const std::string & test)
	{
		if (str.size() < test.size())
			return false;

		auto * pfirst = str.data();
		auto * plast = pfirst + str.size();
		auto * tpfirst = test.data();
		auto * tplast = tpfirst + test.size();

		typedef boost::u8_to_u32_iterator<const char *, std::uint32_t> utf_iter;
		utf_iter test_first {tpfirst}, test_last {tplast};
		utf_iter first {pfirst};

		auto eq = [](auto ch1, auto ch2) { return toupper(ch1) == toupper(ch2); };
		return std::equal(test_first, test_last, first, eq);
	}

	static bool iequals(const std::string & str, const std::string & test)
	{
		auto * pfirst = str.data();
		auto * plast = pfirst + str.size();
		auto * tpfirst = test.data();
		auto * tplast = tpfirst + test.size();

		typedef boost::u8_to_u32_iterator<const char *, std::uint32_t> utf_iter;
		utf_iter first {pfirst}, last {plast};
		utf_iter test_first {tpfirst}, test_last {tplast};

		auto eq = [](auto ch1, auto ch2) { return toupper(ch1) == toupper(ch2); };
		return std::equal(first, last, test_first, test_last, eq);
	}

	static bool icontains(const std::string & str, const std::string & search)
	{
		auto * ifirst = str.data();
		auto * ilast = ifirst + str.size();
		auto * isfirst = search.data();
		auto * islast = isfirst + search.size();

		typedef boost::u8_to_u32_iterator<const char *, std::uint32_t> utf_iter;
		utf_iter first {ifirst}, last {ilast};
		utf_iter sfirst {isfirst}, slast {islast};

		auto eq = [](auto ch1, auto ch2) { return toupper(ch1) == toupper(ch2); };
		auto res = std::search(first, last, sfirst, slast, eq);
		return res != last;
	}

	viewed::refilter_type sparse_container_filter::set_items(index_array items)
	{
		viewed::refilter_type result;

		auto first = items.begin();
		auto last = items.end();
		auto m_first = m_items.begin();
		auto m_last = m_items.end();

		auto it = std::search(first, last, m_first, m_last);
		if (it == last)
			result = viewed::refilter_type::full;
		else if (it == first && last - first == m_last - m_first)
			result = viewed::refilter_type::same;
		else
			result = viewed::refilter_type::incremental;

		m_items = std::move(items);
		return result;
	}

	viewed::refilter_type sparse_container_filter::set_expr(string_type search)
	{
		trim(search);
		viewed::refilter_type result;
		if (iequals(search, m_filter))
			result = viewed::refilter_type::same;
		else if (istarts_with(search, m_filter))
			result = viewed::refilter_type::incremental;
		else
			result = viewed::refilter_type::full;

		m_filter = std::move(search);
		return result;
	}

	viewed::refilter_type sparse_container_filter::set_expr(string_type search, index_array items)
	{
		return std::max(
			set_expr(std::move(search)),
			set_items(std::move(items))
		);
	}

	bool sparse_container_filter::matches(const sparse_container & c) const
	{
		for (auto idx : m_items)
		{
			const auto & val = c.get_item(idx);
			if (matches(val)) return true;
		}

		return false;
	}

	bool sparse_container_filter::matches(const sparse_container::any_type & val) const
	{
		auto * str = any_cast<string_type>(&val);
		return str and icontains(*str, m_filter);
	}

	bool sparse_container_filter::always_matches() const noexcept
	{
		return empty(m_filter) or m_items.empty();
	}
}
