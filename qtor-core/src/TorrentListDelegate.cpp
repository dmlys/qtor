#pragma once
#include <qtor/TorrentListDelegate.hqt>
#include <qtor/TorrentModel.hpp>

#include <QtCore/QStringBuilder>
#include <QtGui/QPainter>
#include <QtWidgets/QAbstractItemView>

//#include <QtTools/Delegates/StyledParts.hpp>
//#include <QtTools/Delegates/DrawFormattedText.hpp>

namespace qtor
{
	const QMargins TorrentListDelegate::ms_InnerMargins = {1, 1, 1, 1};
	const QMargins TorrentListDelegate::ms_OutterMargins = {4, 4, 4, 4};

	template <class Type>
	static optional<double> operator /(optional<Type> opt1, optional<Type> opt2)
	{
		if (not opt1 or not opt2) return nullopt;

		return static_cast<double>(opt1.get()) / static_cast<double>(opt2.get());
	}


	QString TorrentListDelegate::TittleText(const torrent & tor) const
	{
		return ToQString(tor.name().value_or(tor.ms_emptystr));
	}

	QString TorrentListDelegate::ProgressText(const torrent & tor) const
	{
		const auto metadata_progress = tor.metadata_progress();
		const bool magnet = not metadata_progress or metadata_progress.get() < 1.0;

		const auto requested_size = tor.requested_size();
		const auto current_size = tor.current_size();
		const auto total_size = tor.total_size();

		const bool finished = current_size == requested_size;
		const bool partial = requested_size < total_size;

		const auto ratio = tor.ratio();
		const auto seed_ratio = tor.seed_limit();

		QString str;
		if (magnet) // magnet link with metadata still downloading
		{
			//: torrent progress string first part, argument is amount of metadata loading done
			str = tr("Magnetized transfer - retrieving metadata (%1)")
				.arg(m_fmt->format_percent(metadata_progress));
		}
		else if (not finished) // downloading
		{
			//: torrent progress string first part:
			//: %1 is how much we've got,
			//: %2 is how much we'll have when done,
			//: %3 is a percentage of the two
			str = tr("%1 of %2 (%3%)")
				.arg(m_fmt->format_size(current_size))
				.arg(m_fmt->format_size(requested_size))
				.arg(m_fmt->format_percent(current_size / requested_size));
		}
		else
		{
			// whole string looks like:
			// "$current_size of $total_size ($progress), uploaded $ever_uploaded (Ratio: $ratio Goal: $seed_limit)"

			if (partial)
			{
				//: First part of torrent progress string;
				//: %1 is how much we've got,
				//: %2 is the torrent's total size,
				//: %3 is a percentage of the two,
				//: %4 is how much we've uploaded,
				str = tr("%1 of %2 (%3), uploaded %4")
					.arg(m_fmt->format_size(current_size))
					.arg(m_fmt->format_size(total_size))
					.arg(m_fmt->format_percent(current_size / total_size))
					.arg(m_fmt->format_size(tor.ever_uploaded()));
			}
			else
			{
				//: First part of torrent progress string;
				//: %1 is the torrent's total size,
				//: %2 is how much we've uploaded,
				str = tr("%1, uploaded %2")
					.arg(m_fmt->format_size(total_size))
					.arg(m_fmt->format_size(tor.ever_uploaded()));
			}


			if (seed_ratio)
			{
				str = str % ' ' % tr("(Ratio: %3 Goal: %4)")
					.arg(m_fmt->format_ratio(ratio))
					.arg(m_fmt->format_ratio(seed_ratio));
			}
			else
			{
				str = str % ' ' % tr("(Ratio: %1)")
					.arg(m_fmt->format_ratio(ratio));
			}
		}
		
		auto upload_speed = tor.upload_speed();
		auto download_speed = tor.download_speed();

		const speed_type zero = 0;
		// add time when downloading
		if ((seed_ratio and upload_speed.value_or(zero) > zero) or download_speed.value_or(zero) > zero)
		{
			auto eta = tor.eta();
			if (eta)
				//: Second (optional) part of torrent progress string;
				//: %1 is duration;
				//: notice that leading space (before the dash) is included here
				str += tr(" - %1 left").arg(m_fmt->format_duration(eta));
			else
				//: Second (optional) part of torrent progress string;
				//: notice that leading space (before the dash) is included here
				str += tr(" - Remaining time unknown");
		}

		return str.trimmed();
	}

	QString TorrentListDelegate::StatusText(const torrent & tor) const
	{
		return "dummy status";
	}

	void TorrentListDelegate::LayoutItem(const QStyleOptionViewItem & option, const QModelIndex & index, LaidoutItem & item) const
	{
		item.option = &option;
		if (item.index == index)
			return;
		
		item.index = index;

		const auto * model = dynamic_cast<const TorrentModel *>(item.index.model());
		const auto & tor = model->GetItem(item.index.row());

		item.tor = &tor;
		item.name = TittleText(tor);
		item.progress = ProgressText(tor);
		item.status = StatusText(tor);

		item.baseFont = option.font;
		item.nameFont = item.progressFont = item.statusFont = item.baseFont;

		item.nameFont.setBold(true);
		item.progressFont.setPointSize(item.baseFont.pointSize() * 9 / 10);
		item.statusFont.setPointSize(item.baseFont.pointSize() * 9 / 10);

		QPaintDevice * device = const_cast<QWidget *>(option.widget);
		QFontMetrics baseFM {item.baseFont, device};
		QFontMetrics nameFM {item.nameFont, device};
		QFontMetrics progressFM {item.progressFont, device};
		QFontMetrics statusFM {item.statusFont, device};

		QSize nameSize = nameFM.size(0, item.name);
		QSize progresSize = progressFM.size(0, item.progress);
		QSize statusSize = statusFM.size(0, item.status);
		QSize progressBarSize = QSize(0, baseFM.height() * 10 / 10);

		QPoint topLeft = {0, 0};

		auto hintRect = option.rect;
		item.nameRect = {topLeft, nameSize};
		item.progressRect = {topLeft, progresSize};
		item.statusRect = {topLeft, statusSize};
		item.barRect = {topLeft, progressBarSize};

		// layout rectangles vertically in that order
		auto & totalRect = item.totalRect = {};
		auto rects = {&item.nameRect, &item.progressRect, &item.barRect, &item.statusRect};

		auto dy = 0;
		for (auto * rect : rects)
		{
			*rect += ms_InnerMargins;
			rect->translate(0, dy);
			dy += rect->height();

			totalRect |= *rect;
		}
		
		// expand to hint width
		totalRect.setWidth(std::max(totalRect.width(), hintRect.width()));
		totalRect += ms_OutterMargins;
	}

	void TorrentListDelegate::DrawBackground(QPainter * painter, const LaidoutItem & item) const
	{
		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;
		const bool enabled = option.state & QStyle::State_Enabled;
		const bool active = option.state & QStyle::State_Active;

		QPalette::ColorGroup cg =
			not enabled ? QPalette::Disabled :
			not  active ? QPalette::Inactive
			            : QPalette::Normal;

		if (selected)
		{
			auto rect = item.option->rect;
			rect.moveTo(0, 0);
			painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
		}
	}

	void TorrentListDelegate::DrawText(QPainter * painter, const LaidoutItem & item) const
	{
		const QStyleOptionViewItem & option = *item.option;
		auto * style = item.option->widget->style();

		const auto status = item.tor->status().get();
		const bool paused = status == torrent_status::stopped;
		const bool error = false;

		const bool selected = option.state & QStyle::State_Selected;
		const bool enabled = option.state & QStyle::State_Enabled;
		const bool active = option.state & QStyle::State_Active;

		QPalette::ColorGroup cg =
			paused or not enabled ? QPalette::Disabled :
			          not active  ? QPalette::Inactive
			                      : QPalette::Normal;

		QPalette::ColorRole cr = selected ? QPalette::HighlightedText : QPalette::Text;

		qDebug("cg = %d, cr = %d", cg, cr);

		static const QColor red {"red"};
		painter->setPen(error and not selected ? red : option.palette.color(cg, cr));

		auto flags = Qt::AlignLeft | Qt::AlignVCenter;
		//const auto flags = 0;

		painter->setFont(item.nameFont);
		painter->drawText(item.nameRect - ms_InnerMargins, flags, item.name);
		painter->setFont(item.progressFont);
		painter->drawText(item.progressRect - ms_InnerMargins, flags, item.progress);
		painter->setFont(item.statusFont);
		painter->drawText(item.statusRect - ms_InnerMargins, flags, item.status);
	}

	void TorrentListDelegate::Draw(QPainter * painter, const LaidoutItem & item) const
	{
		//QIcon::State qs = paused ? QIcon::Off : QIcon::On;
		//QIcon::Mode im = paused or not enabled ? QIcon::Disabled :
		//                              selected ? QIcon::Selected
		//                                       : QIcon::Normal;

		//const QIcon::Mode emblemIm = selected ? QIcon::Selected : QIcon::Normal;
		//const QIcon emblemIcon = error ? QIcon::fromTheme(QStringLiteral("emblem-important"), style->standardIcon(QStyle::SP_MessageBoxWarning)) : QIcon();

		// render
		//tor.getMimeTypeIcon().paint(painter, layout.iconRect, Qt::AlignCenter, im, qs);

		//if (not emblemIcon.isNull())
		//	emblemIcon.paint(painter, item.emblemRect, Qt::AlignCenter, emblemIm, qs);

		//auto flags = Qt::AlignLeft | Qt::AlignVCenter;

		DrawBackground(painter, item);
		DrawText(painter, item);
		DrawProgressBar(painter, item);
	}

	void TorrentListDelegate::DrawProgressBar(QPainter * painter, const LaidoutItem & item) const
	{
		auto * style = item.option->widget->style();
		auto & tor = *item.tor;

		const auto status = tor.status().get();
		const bool seeding = status == torrent_status::seed;
		const bool downloading = status == torrent_status::download;
		const bool paused = status == torrent_status::stopped;

		const auto metadata_progress = tor.metadata_progress();
		const auto requested_progress = tor.requested_progress();
		const bool magnet = not metadata_progress or metadata_progress.get() < 1.0;

		const auto ratio = tor.ratio();
		const auto seed_limit = tor.seed_limit();
		constexpr double ZERO = 0.0;

		QStyleOptionProgressBar option;
		option.QStyleOption::operator =(*item.option);

		const auto * listView = qobject_cast<const QAbstractItemView *>(item.option->widget);		
		option.styleObject = listView->viewport();

		option.progress = 0;
		option.minimum = 0;
		option.maximum = 1'000;
		
		if (paused) option.state = QStyle::State_None;
		option.state |= QStyle::State_Small;

		option.rect = item.barRect;
		option.rect.setWidth(item.option->rect.width());
		option.rect -= ms_InnerMargins;

		auto & palette = option.palette;

		if (seeding and seed_limit)
		{
			const auto scale = ratio / seed_limit;
			option.progress = static_cast<int>(option.minimum + scale.value_or(ZERO) * (option.maximum - option.minimum));
		}
		else
		{
			const auto progress = magnet ? metadata_progress : requested_progress;
			option.progress = static_cast<int>(option.minimum + progress.value_or(ZERO) * (option.maximum - option.minimum));
		}
	

		if (downloading)
		{
			static const QColor front = QColor("forestgreen");
			static const QColor back  = QColor("darkseagreen");

			palette.setBrush(QPalette::Highlight, front);
			palette.setColor(QPalette::Base, back);
			palette.setColor(QPalette::Window, back);
		}
		else if (seeding)
		{
			static const QColor front = QColor("steelblue");
			static const QColor back = QColor("steelblue");

			palette.setBrush(QPalette::Highlight, front);
			palette.setColor(QPalette::Base, back);
			palette.setColor(QPalette::Window, back);
		}
		else
		{
			static const QColor front = QColor("silver");
			static const QColor back = QColor("grey");

			palette.setBrush(QPalette::Highlight, front);
			palette.setColor(QPalette::Base, back);
			palette.setColor(QPalette::Window, back);
		}

		style->drawControl(QStyle::CE_ProgressBar, &option, painter);
	}

	QSize TorrentListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		LayoutItem(option, index, m_cachedItem);
		return m_cachedItem.totalRect.size();
	}

	void TorrentListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		LayoutItem(option, index, m_cachedItem);
		auto area = (option.rect - ms_OutterMargins).topLeft();

		painter->save();
		painter->translate(area);
		Draw(painter, m_cachedItem);
		painter->restore();
	}
}
