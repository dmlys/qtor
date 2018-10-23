#include <qtor/SimpleLabel.hqt>
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtTools/Delegates/DrawFormattedText.hpp>

namespace QtTools
{
	Qt::LayoutDirection SimpleLabel::TextDirection() const
	{
		return m_text.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
	}

	QTextOption SimpleLabel::PrepareTextOption() const
	{
		QTextOption textOption;

		const auto direction = TextDirection();
		const Qt::Alignment align = QStyle::visualAlignment(direction, m_alignment);
		textOption.setWrapMode(m_wordWrap ? QTextOption::WordWrap : QTextOption::ManualWrap);
		textOption.setTextDirection(direction);
		textOption.setAlignment(align);

		return textOption;
	}

	auto SimpleLabel::LayoutText(const QRect & rect) const -> std::unique_ptr<QTextLayout>
	{
		using namespace QtTools::Delegates;
		using namespace QtTools::Delegates::TextLayout;

		QPaintDevice * device = const_cast<SimpleLabel *>(this);
		const auto font = this->font();
		const QFontMetrics fm(font);

		const auto  indent = m_indent;
		const auto  left_indent = m_alignment & Qt::AlignLeft ? indent : 0;
		const auto  top_indent  = m_alignment & Qt::AlignTop  ? indent : 0;
		const QSize pixsz  = m_pixmap.size();

		const auto  width  = rect.width();
		const qreal height = std::min(rect.height(), m_maximum_lines ? m_maximum_lines * fm.height() : maximumHeight());

		auto layout = std::make_unique<QTextLayout>(m_text, font, device);
		const auto textopt = PrepareTextOption();
		layout->setCacheEnabled(true);
		layout->setTextOption(textopt);


		qreal cury = top_indent;
		int elide_index = 0;
		layout->beginLayout();
		for (;;)
		{
			auto line = layout->createLine();
			if (not line.isValid()) break;

			qreal posx = cury < pixsz.height() ? pixsz.width() + left_indent : 0;
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

			qreal cury = top_indent;
			layout->beginLayout();
			for (;;)
			{
				auto line = layout->createLine();
				if (not line.isValid()) break;

				qreal posx = cury < pixsz.height() ? pixsz.width() + left_indent : 0;
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

	QRect SimpleLabel::BoundingRect(const QRect & rect) const
	{
		using QtTools::Delegates::TextLayout::BoundingRect;
		auto layout = LayoutText(rect);
		return BoundingRect(*layout, layout->lineCount()).toAlignedRect();
	}

	QSize SimpleLabel::sizeForWidth(int width) const
	{
		return {};
	}

	void SimpleLabel::PreparePainter(QPainter * painter) const
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

	void SimpleLabel::paintEvent(QPaintEvent * event)
	{
		QPainter painter(this);
		drawFrame(&painter);

		auto contents_rect = contentsRect();
		contents_rect.adjust(m_margin, m_margin, -m_margin, -m_margin);

		auto layout = LayoutText(contents_rect);

		PreparePainter(&painter);
		QtTools::Delegates::TextLayout::DrawLayout(&painter, contents_rect.topLeft(), *layout);
	}

	void SimpleLabel::updateLabel()
	{
		updateGeometry();
		update();
	}

	void SimpleLabel::setText(QString text)
	{
		m_text = std::move(text);
		updateLabel();
	}

	void SimpleLabel::setNum(int num)
	{

	}

	void SimpleLabel::setNum(double num)
	{

	}

	void SimpleLabel::setPixmap(QPixmap pixmap)
	{

	}

	void SimpleLabel::clear()
	{
		m_text.clear();
		m_pixmap = {};
		updateLabel();
	}

	void SimpleLabel::setBuddy(QWidget * buddy)
	{

	}

	void SimpleLabel::setAlignment(Qt::Alignment alignment)
	{
		m_alignment = alignment;
		updateLabel();
	}

	void SimpleLabel::setWordWrap(bool wordWrap)
	{
		m_wordWrap = wordWrap;
		updateLabel();
	}

	void SimpleLabel::setMargin(int margin)
	{
		m_margin = margin;
		updateLabel();
	}

	void SimpleLabel::setIndent(int indent)
	{
		m_indent = indent;
		updateLabel();
	}

	SimpleLabel::SimpleLabel(QWidget * parent /*= nullptr*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
	    : QFrame(parent, flags)
	{

	}

	SimpleLabel::SimpleLabel(const QString & text, QWidget * parent /*= nullptr*/, Qt::WindowFlags flags /*= Qt::WindowFlags()*/)
	    : QFrame(parent, flags), m_text(text)
	{

	}
}
