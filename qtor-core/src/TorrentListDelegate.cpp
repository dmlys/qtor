#pragma once
#include <qtor/TorrentListDelegate.hqt>
#include <qtor/TorrentsModel.hpp>

#include <QtCore/QStringBuilder>
#include <QtGui/QPainter>
#include <QtGui/QLinearGradient>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QAbstractItemView>

#include <QtTools/Delegates/StyledParts.hpp>
#include <QtTools/Delegates/SearchDelegate.hpp>

namespace qtor
{
	const QMargins TorrentListDelegate::ms_InnerMargins = {0, 1, 0, 1};
	const QMargins TorrentListDelegate::ms_OutterMargins = {4, 4, 4, 4};

	template <class Type>
	static optional<double> operator /(optional<Type> opt1, optional<Type> opt2)
	{
		if (not opt1 or not opt2) return nullopt;

		return static_cast<double>(opt1.value()) / static_cast<double>(opt2.value());
	}


	QString TorrentListDelegate::TittleText(const torrent & tor) const
	{
		return ToQString(tor.name().value_or(tor.ms_emptystr));
	}

	QString TorrentListDelegate::ProgressText(const torrent & tor) const
	{
		const auto metadata_progress = tor.metadata_progress();
		const bool magnet = metadata_progress.value() < 1.0;

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

		// add time when downloading
		if ((seed_ratio and upload_speed > 0) or download_speed > 0)
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

	//QString TorrentListDelegate::ShortTransferText(const torrent & tor) const
	//{
	//	auto metadata = tor.metadata_progress();
	//	bool downloading = metadata and (tor.downloading_peers() > 0 or tor.downloading_webseeds() > 0);
	//	bool uploading = metadata and (tor.uploading_peers() > 0);

	//	if (downloading)
	//	{
	//		return m_fmt->format_speed(tor.download_speed()) % " " % m_fmt->format_speed(tor.upload_speed());
	//	}
	//	else if (uploading)
	//	{
	//		return m_fmt->format_speed(tor.upload_speed());
	//	}
	//	else
	//	{
	//		return {};
	//	}
	//}

	//QString TorrentListDelegate::ShortStatusText(const torrent & tor) const
	//{
	//	auto status = tor.status().value();
	//	switch (status)
	//	{
	//		case torrent_status::checking:
	//			return tr("Verifying local data (%1 tested)").arg(m_fmt->format_percent(tor.recheck_progress()));

	//		case torrent_status::downloading:
	//		case torrent_status::seeding:
	//			return ShortTransferText(tor) % "    " % tr("Ratio: %1").arg(m_fmt->format_percent(tor.ratio()));
	//			
	//		default:
	//			return TorrentsModel::StatusString(status);
	//	}
	//}

	QString TorrentListDelegate::StatusText(const torrent & tor) const
	{
		QString str;
		
		auto status = tor.status().value();
		auto metadata_progress = tor.metadata_progress();
		bool metadata_finished = metadata_progress >= 1.0;

		switch (status)
		{
			case torrent_status::checking:
				str = tr("Verifying local data (%1 tested)").arg(m_fmt->format_percent(tor.recheck_progress()));
				break;

			case torrent_status::seeding:
			{
				auto connected = tor.connected_peers().value_or(0);
				auto uploading = tor.uploading_peers().value_or(0);
				if (connected == 0)
				{
					str = tr("Seeding to %Ln peer(s)", 0, uploading);
				}
				else
				{
					str = tr("Seeding to %1 of %Ln connected peer(s)", 0, connected)
						.arg(m_fmt->format_uint64(uploading));
				}

				str += " - " % m_fmt->format_speed(tor.upload_speed());
				break;
			}

			case torrent_status::downloading:
			{
				if (not metadata_finished)
				{
					auto peers = tor.downloading_peers().value_or(0);
					str = tr("Downloading metadata from %Ln peer(s) (%1 done)", 0, peers)
						.arg(m_fmt->format_percent(metadata_progress));
				}
				else
				{
					auto peers = tor.connected_peers().value_or(0);
					auto webseeds = tor.connected_webseeds().value_or(0);
					auto connected = peers + webseeds;
					/* it would be nicer for translation if this was all one string, but I don't see how to do multiple %n's in tr() */
					str = tr("Downloading from %1 of %Ln connected peer(s)", 0, connected)
							 .arg(m_fmt->format_uint64(tor.downloading_peers()));
					
					if (webseeds)
						//: Second (optional) part of phrase "Downloading from ... of ... connected peer(s) and ... web seed(s)";
						//: notice that leading space (before "and") is included here
						str += tr(" and %Ln web seed(s)", 0, webseeds);

					str += " - " % m_fmt->format_speed(tor.download_speed()) % "  " % m_fmt->format_speed(tor.upload_speed());
				}

				break;
			}

			case torrent_status::stopped:
			case torrent_status::checking_queued:
			case torrent_status::downloading_queued:
			case torrent_status::seeding_queued:
			default:
				str = TorrentsModel::StatusString(status);
				break;
		}

		return str;
	}

	void TorrentListDelegate::LayoutItem(const QStyleOptionViewItem & option, const QModelIndex & index, LaidoutItem & item) const
	{
		item.option = &option;
		if (item.index == index)
		{
			auto newTopLeft = option.rect.topLeft();
			auto diff = option.rect.topLeft() - item.hintTopLeft;
			item.hintTopLeft = option.rect.topLeft();

			item.nameRect.translate(diff);
			item.progressRect.translate(diff);
			item.barRect.translate(diff);
			item.statusRect.translate(diff);

			return;
		}
		
		item.hintTopLeft = option.rect.topLeft();
		item.index = index;

		const auto * model = dynamic_cast<const TorrentsModel *>(item.index.model());
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

		QRect rect = item.option->rect - ms_OutterMargins;
		QSize nameSize = nameFM.size(0, item.name);
		QSize progresSize = progressFM.size(0, item.progress);
		QSize statusSize = statusFM.size(0, item.status);

		//auto barWidth = std::max({nameSize.width(), progresSize.width(), statusSize.width(), rect.size().width()});
		QSize progressBarSize = QSize(0, baseFM.height());

		QPoint topLeft = rect.topLeft();

		item.nameRect = {topLeft, nameSize};
		item.progressRect = {topLeft, progresSize};
		item.statusRect = {topLeft, statusSize};
		item.barRect = {topLeft, progressBarSize};

		// layout rectangles vertically in that order
		auto rects = {&item.nameRect, &item.progressRect, &item.barRect, &item.statusRect};

		auto dy = 0;
		for (auto * rect : rects)
		{
			dy += ms_InnerMargins.top();
			rect->translate(0, dy);
			dy += rect->height();
			dy += ms_InnerMargins.bottom();
		}

		auto & totalRect = item.totalRect = {};
		totalRect = item.nameRect | item.progressRect | item.barRect | item.statusRect;
		totalRect += ms_InnerMargins;
		totalRect += ms_OutterMargins;
	}

	void TorrentListDelegate::DrawBackground(QPainter * painter, const LaidoutItem & item) const
	{
		using QtTools::Delegates::ColorGroup;

		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;

		if (selected)
		{
			auto cg = ColorGroup(option);
			painter->fillRect(item.option->rect, option.palette.brush(cg, QPalette::Highlight));
		}

		using namespace QtTools::Delegates;
		if (HasFocusFrame(option))
		{
			DrawFocusFrame(painter, item.option->rect, option);
		}
	}

	void TorrentListDelegate::DrawText(QPainter * painter, const LaidoutItem & item) const
	{
		const QStyleOptionViewItem & option = *item.option;
		auto * style = item.option->widget->style();

		const auto status = item.tor->status().value();
		const bool paused = status == torrent_status::stopped;
		const bool error = false;

		const bool selected = option.state & QStyle::State_Selected;
		auto cg = paused ? QPalette::Disabled : QtTools::Delegates::ColorGroup(option);
		auto cr = selected ? QPalette::HighlightedText : QPalette::Text;

		static const QColor red {"red"};
		painter->setPen(error and not selected ? red : option.palette.color(cg, cr));


		QTextCharFormat fmt, searchFmt = m_searchFormat;
		fmt.setFont(item.nameFont);
		searchFmt.setFont(item.nameFont);

		QVector<QTextLayout::FormatRange> formats;		
		formats.push_back({0, item.name.length(), fmt});

		QtTools::Delegates::FormatSearchText(item.name, m_searchText, searchFmt, formats);
		QtTools::Delegates::DrawSearchFormatedText(painter, item.name, item.nameRect, option, formats);


		QPaintDevice * device = const_cast<QWidget *>(option.widget);
		QFontMetrics nameFM {item.nameFont, device};
		QFontMetrics progressFM {item.progressFont, device};
		QFontMetrics statusFM {item.statusFont, device};

		auto mode = option.textElideMode;
		auto alignment = QStyle::visualAlignment(option.direction, option.displayAlignment);

		auto progressText = progressFM.elidedText(item.progress, mode, item.progressRect.width());
		auto statusText = statusFM.elidedText(item.status, mode, item.statusRect.width());

		painter->setFont(item.progressFont);
		painter->drawText(item.progressRect, alignment, progressText);
		painter->setFont(item.statusFont);
		painter->drawText(item.statusRect, alignment, statusText);
	}

	void TorrentListDelegate::Draw(QPainter * painter, const LaidoutItem & item) const
	{
		DrawBackground(painter, item);
		DrawText(painter, item);
		DrawProgressBar(painter, item);
	}

	QPalette TorrentListDelegate::ProgressBarPalete(const LaidoutItem & item) const
	{
		auto & tor = *item.tor;

		const auto status = tor.status().value();
		const bool seeding = status == torrent_status::seeding;
		const bool downloading = status == torrent_status::downloading;

		auto palette = item.option->palette;
		if (downloading)
		{
			static const QColor front = QColor("steelblue");
			static const QColor back = QColor("lightgrey");

			palette.setBrush(QPalette::Highlight, front);
			palette.setColor(QPalette::Base, back);
			palette.setColor(QPalette::Window, back);
		}
		else if (seeding)
		{
			static const QColor front = QColor("forestgreen");
			static const QColor back = QColor("darkseagreen");

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

		return palette;
	}	

	void TorrentListDelegate::DrawProgressBar(QPainter * painter, const LaidoutItem & item) const
	{
#ifdef Q_OS_WIN
		static auto * style = QStyleFactory::create("Fusion");
#else
		auto * style = item.option->widget->style();
#endif

		auto & tor = *item.tor;
		const auto status = tor.status().value();
		const bool seeding = status == torrent_status::seeding;
		const bool downloading = status == torrent_status::downloading;
		const bool paused = status == torrent_status::stopped;

		const auto metadata_progress = tor.metadata_progress();
		const auto requested_progress = tor.requested_progress();
		const bool magnet = not metadata_progress or metadata_progress.value() < 1.0;

		const auto ratio = tor.ratio();
		const auto seed_limit = tor.seed_limit();

		QStyleOptionProgressBar option;
		option.QStyleOption::operator =(*item.option);

		const auto * listView = qobject_cast<const QAbstractItemView *>(item.option->widget);
		option.styleObject = listView->viewport();

		option.progress = 0;
		option.minimum = 0;
		option.maximum = 1'000;
		
		if (paused) option.state = QStyle::State_None;
		option.state |= QStyle::State_Small;
		
		auto barWidth = item.option->rect.width();
		barWidth -= ms_InnerMargins.right() + ms_InnerMargins.left() + ms_OutterMargins.right() + ms_OutterMargins.left();
		
		option.rect = item.barRect;
		option.rect.setWidth(barWidth);
		option.palette = ProgressBarPalete(item);

		if (seeding and seed_limit)
		{
			const auto scale = ratio / seed_limit;
			option.progress = static_cast<int>(option.minimum + scale.value_or(0) * (option.maximum - option.minimum));
		}
		else
		{
			const auto progress = magnet ? metadata_progress : requested_progress;
			option.progress = static_cast<int>(option.minimum + progress.value_or(0) * (option.maximum - option.minimum));
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

		painter->save();
		Draw(painter, m_cachedItem);
		painter->restore();
	}
}
