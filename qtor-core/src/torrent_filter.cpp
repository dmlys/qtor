#include <algorithm>
#include <boost/regex/pending/unicode_iterator.hpp>

#include <QtCore/QChar>
#include <qtor/torrent_filter.hpp>

namespace qtor
{
	static std::uint32_t toupper(std::uint32_t ch)
	{
#if 0
		if ('a' <= ch && ch <= 'z')
			return ch + 'A' - 'a';

		// \U0430 - 'a', \U0410 - 'A', \U044F - 'ÿ'
		//  L'a'   <= ch && ch <= L'ÿ'
		if (0x0430 <= ch && ch <= 0x044F)
			return ch + 0x0410 - 0x0430; // ch + L'À' - L'à'

		return ch;
#endif
		return QChar::toUpper(ch);
	}

	static void trim(string_type & str)
	{
		auto * pfirst = const_cast<char *>(str.data());
		auto * plast = pfirst + str.size();

		typedef boost::u8_to_u32_iterator<const char *, std::uint32_t> utf_iter;
		utf_iter first {pfirst}, last {plast};

		auto space = [](auto ch) { return QChar::isSpace(ch); };
		auto new_first = std::find_if(first, last, space);

		auto new_last = std::find_if(
			std::make_reverse_iterator(last),
			std::make_reverse_iterator(new_first),
			space).base();

		auto stopped = std::move(new_last.base(), new_last.base(), pfirst);
		str.erase(stopped - pfirst, plast - stopped);
	}

	static bool istarts_with(const string_type & str, const string_type & test)
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

	static bool iequals(const string_type & str, const string_type & test)
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

	static bool icontains(const string_type & str, const string_type & search)
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

	viewed::refilter_type torrent_filter::set_expr(std::string search)
	{
		trim(search);
		viewed::refilter_type result;
		if (iequals(m_search, search))
			result = viewed::refilter_type::same;
		else if (istarts_with(m_search, search)) 
			result = viewed::refilter_type::incremental;
				
		m_search = std::move(search);
		return result;
	}
	
	bool torrent_filter::matches(const torrent & t) const noexcept
	{
		return icontains(t.name, m_search);
	}

	bool torrent_filter::always_matches() const noexcept
	{
		return m_search.empty();
	}
}
