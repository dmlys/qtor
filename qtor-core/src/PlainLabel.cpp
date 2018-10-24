#include <qtor/PlainLabel.hqt>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtTools/Delegates/DrawFormattedText.hpp>

namespace QtTools
{
	int PlainLabel::GetIdent(const QFontMetrics & fm) const
	{
		if (m_indent >= 0) return m_indent;
		if (frameWidth() <= 0) return m_indent;

		return fm.width('x') / 2;
	}

	Qt::LayoutDirection PlainLabel::TextDirection() const
	{
		return m_text.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
	}

	QTextOption PlainLabel::PrepareTextOption() const
	{
		QTextOption textOption;

		const auto direction = TextDirection();
		const Qt::Alignment align = QStyle::visualAlignment(direction, m_alignment);
		textOption.setWrapMode(m_word_wrap ? QTextOption::WordWrap : QTextOption::ManualWrap);
		textOption.setTextDirection(direction);
		textOption.setAlignment(align);

		return textOption;
	}

	auto PlainLabel::LayoutText(const QRect & rect) const -> std::unique_ptr<QTextLayout>
	{
		using namespace QtTools::Delegates;
		using namespace QtTools::Delegates::TextLayout;

		QPaintDevice * device = const_cast<PlainLabel *>(this);
		const auto font = this->font();
		const QFontMetrics fm(font);

		const auto  width  = rect.width();
		const qreal height = std::min(rect.height(), m_line_limit ? m_line_limit * fm.height() : maximumHeight());

		auto layout = std::make_unique<QTextLayout>(m_text, font, device);
		const auto textopt = PrepareTextOption();
		layout->setCacheEnabled(true);
		layout->setTextOption(textopt);


		qreal cury = 0;
		int elide_index = 0;
		layout->beginLayout();
		for (;;)
		{
			auto line = layout->createLine();
			if (not line.isValid()) break;

			qreal posx = 0;
			line.setPosition({posx, cury});
			line.setLineWidth(width - posx);
			cury += line.height();

			// last line didn't fit given total height
			if (cury > height)
			{
				elide_index = std::max(0, elide_index - 1);
				break;
			}

			// very long word, does not fit given width
			if (line.naturalTextWidth() > width)
				break;

			++elide_index;
		};

		layout->endLayout();
		const bool needs_elide = elide_index != layout->lineCount();

		if (needs_elide)
		{
			auto text = m_text;
			auto line = layout->lineAt(elide_index);
			int elidePoint = line.textStart();
			text = text.mid(0, elidePoint) + ElideText(fm, text.mid(elidePoint), Qt::ElideRight, line.width());

			layout = nullptr;
			layout = std::make_unique<QTextLayout>(text, font, device);
			layout->setTextOption(textopt);
			layout->setCacheEnabled(true);

			qreal cury = 0;
			layout->beginLayout();
			for (;;)
			{
				auto line = layout->createLine();
				if (not line.isValid()) break;

				qreal posx = 0;
				line.setPosition({posx, cury});
				line.setLineWidth(width - posx);
				cury += line.height();

				// last line didn't fit given total height
				if (cury > height) break;
				// very long word, does not fit given width
				if (line.naturalTextWidth() > width) break;
			};

			layout->endLayout();
		}

		return layout;
	}

	QRect PlainLabel::BoundingRect(const QRect & rect) const
	{
		auto layout = LayoutText(rect);
		return QtTools::Delegates::TextLayout::NaturalBoundingRect(*layout, layout->lineCount()).toAlignedRect();
	}

	QSize PlainLabel::sizeForWidth(int width) const
	{
		const QFontMetrics fm(font());
		const auto indent        = GetIdent(fm);
		const auto left_indent   = m_alignment & Qt::AlignLeft   ? indent : 0;
		const auto right_indent  = m_alignment & Qt::AlignRight  ? indent : 0;
		const auto top_indent    = m_alignment & Qt::AlignTop    ? indent : 0;
		const auto bottom_indent = m_alignment & Qt::AlignBottom ? indent : 0;

		const auto line_limit  = m_line_limit > 0 ? m_line_limit : INT_MAX;
		const auto lines_count = [&fm](const auto & rect) { return rect.height() / fm.lineSpacing(); };

		const auto default_height = QApplication::desktop()->availableGeometry().width();
		const auto maximum_size = this->maximumSize();
		const bool try_width = width < 0 and m_word_wrap;
		width = try_width ? std::min(fm.averageCharWidth() * 80, maximum_size.width())
		                  : width < 0 ? 2000 : width;

		auto rect = BoundingRect({0, 0, width, default_height});
		auto lc = lines_count(rect);
		if (try_width and lc < 4 and lc < line_limit and rect.width() > width / 2)
		{
			rect = BoundingRect({0, 0, width / 2, default_height});
			lc = lines_count(rect);
		}

		if (try_width and lc < 2 and lc < line_limit and rect.width() > width / 4)
			rect = BoundingRect({0, 0, width / 4, default_height});

		rect.adjust(-left_indent, -top_indent, right_indent, bottom_indent);
		rect.adjust(-m_margin, -m_margin, m_margin, m_margin);
		return rect.size();
	}

	int PlainLabel::heightForWidth(int width) const
	{
		return sizeForWidth(width).height();
	}

	QSize PlainLabel::sizeHint() const
	{
		if (m_cached_size_hint.isValid()) return m_cached_size_hint;
		return m_cached_size_hint = sizeForWidth(-1);
	}

	//QSize SimpleLabel::minimumSizeHint() const
	//{
	//	if (m_cached_minumum_size_hint.isValid()) return m_cached_minumum_size_hint;
	//	return m_cached_minumum_size_hint = sizeForWidth(0);
	//}

	void PlainLabel::PreparePainter(QPainter * painter) const
	{
		//const auto * style = this->style();
		//const auto enabled = isEnabled();
		const auto foregroundRole = this->foregroundRole();
		const auto palette = this->palette();

	    if (foregroundRole != QPalette::NoRole)
		{
	        auto pen = painter->pen();
	        painter->setPen(QPen(palette.brush(foregroundRole), pen.widthF()));
	    }

		painter->setFont(this->font());
	}

	void PlainLabel::paintEvent(QPaintEvent * event)
	{
		QPainter painter(this);
		drawFrame(&painter);

		const QFontMetrics fm(font());
		const auto indent        = GetIdent(fm);
		const auto left_indent   = m_alignment & Qt::AlignLeft   ? indent : 0;
		const auto right_indent  = m_alignment & Qt::AlignRight  ? indent : 0;
		const auto top_indent    = m_alignment & Qt::AlignTop    ? indent : 0;
		const auto bottom_indent = m_alignment & Qt::AlignBottom ? indent : 0;

		auto contents_rect = contentsRect();
		contents_rect.adjust(m_margin, m_margin, -m_margin, -m_margin);
		contents_rect.adjust(left_indent, top_indent, -right_indent, -bottom_indent);

		auto layout = LayoutText(contents_rect);

		PreparePainter(&painter);
		QtTools::Delegates::TextLayout::DrawLayout(&painter, contents_rect.topLeft(), *layout);
	}

	void PlainLabel::UpdateLabel()
	{
		auto pol = sizePolicy();
		pol.setHeightForWidth(m_word_wrap);
		setSizePolicy(pol);

		//m_cached_minumum_size_hint = {};
		m_cached_size_hint = {};
		updateGeometry();
		update();
	}

	void PlainLabel::setText(QString text)
	{
		m_text = std::move(text);
		UpdateLabel();
	}

	void PlainLabel::setNum(int num)
	{
		const auto locale = this->locale();
		auto str = locale.toString(num);
		setText(std::move(str));
	}

	void PlainLabel::setNum(double num)
	{
		const auto locale = this->locale();
		auto str = locale.toString(num);
		setText(std::move(str));
	}

	void PlainLabel::clear()
	{
		m_text.clear();
		UpdateLabel();
	}

	void PlainLabel::setAlignment(Qt::Alignment alignment)
	{
		m_alignment = alignment;
		UpdateLabel();
	}

	void PlainLabel::setWordWrap(bool wordWrap)
	{
		m_word_wrap = wordWrap;
		UpdateLabel();
	}

	void PlainLabel::setMargin(int margin)
	{
		m_margin = margin;
		UpdateLabel();
	}

	void PlainLabel::setIndent(int indent)
	{
		m_indent = indent;
		UpdateLabel();
	}

	void PlainLabel::init()
	{
		QSizePolicy pol(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::Label);
		setSizePolicy(pol);
	}

	PlainLabel::PlainLabel(QWidget * parent /*= nullptr*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
	    : QFrame(parent, flags)
	{
		init();
	}

	PlainLabel::PlainLabel(const QString & text, QWidget * parent /*= nullptr*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
	    : QFrame(parent, flags), m_text(text)
	{
		init();
	}
}
