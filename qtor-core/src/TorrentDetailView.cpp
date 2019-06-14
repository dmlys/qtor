#include <qtor/TorrentDetailView.hqt>
#include <qtor/formatter.hpp>

namespace qtor
{
	void TorrentDetailView::SetTorrent(const torrent_list & torrents)
	{
		uint64_type status;
		bool paused;

		size_type have_total = 0;
		size_type have_verified = 0;
		size_type have_unverified = 0;
		size_type verified_peices = 0;


		for (auto & torr : torrents)
		{
			const auto metadata_progress = torr.metadata_progress().value_or(0.0);
			if (metadata_progress == 1.0)
			{
				const auto status = torr.status();
				const bool paused = status == torrent_status::stopped or status == torrent_status::unknown;
				const auto error_string = torr.error_string().value_or(torrent::ms_emptystr);
				const bool error = status == torrent_status::unknown or not error_string.isEmpty();
			}

			have_total += torr.total_size().value_or(0);
			//have_verified += torr.

			QString str;
			//if (metadata_progress.value_or(0) < 1.0) // magnet link with metadata still downloading
			//{
			//	//: torrent progress string first part, argument is amount of metadata loading done
			//	str = tr("Magnetized transfer - retrieving metadata (%1)")
			//		.arg(fmt->format_percent(metadata_progress));
			//}
			///*else*/ if (not finished) // downloading
			//{
			//	//: torrent progress string first part:
			//	//: %1 is how much we've got,
			//	//: %2 is how much we'll have when done,
			//	//: %3 is a percentage of the two
			//	str = tr("%1 of %2 (%3)")
			//		.arg(fmt->format_size(current_size))
			//		.arg(fmt->format_size(requested_size))
			//		.arg(fmt->format_percent(current_size / requested_size));
			//}
			//else
			//{
			//	if (partial)
			//	{
			//		//: First part of torrent progress string;
			//		//: %1 is how much we've got,
			//		//: %2 is the torrent's total size,
			//		//: %3 is a percentage of the two,
			//		//: %4 is how much we've uploaded,
			//		str = tr("%1 of %2 (%3), uploaded %4")
			//			.arg(fmt->format_size(current_size))
			//			.arg(fmt->format_size(total_size))
			//			.arg(fmt->format_percent(current_size / total_size))
			//			.arg(fmt->format_size(tor.ever_uploaded()));
			//	}
			//	else
			//	{
			//		//: First part of torrent progress string;
			//		//: %1 is the torrent's total size,
			//		//: %2 is how much we've uploaded,
			//		str = tr("%1, uploaded %2")
			//			.arg(fmt->format_size(total_size))
			//			.arg(fmt->format_size(tor.ever_uploaded()));
			//	}
			//
			//
			//	if (seed_ratio)
			//	{
			//		str = str % ' ' % tr("(Ratio: %3 Goal: %4)")
			//			.arg(fmt->format_ratio(ratio))
			//			.arg(fmt->format_ratio(seed_ratio));
			//	}
			//	else
			//	{
			//		str = str % ' ' % tr("(Ratio: %1)")
			//			.arg(fmt->format_ratio(ratio));
			//	}
			//}

		}

	}

	void TorrentDetailView::SetTorrent(const torrent & torr)
	{
		m_haveLabel->setText(m_fmt->format_percent(torr.ratio()));
	}

	void TorrentDetailView::SetTorrent(const QModelIndex & idx)
	{

	}

	TorrentDetailView::TorrentDetailView(QWidget * parent)
	    : QFrame(parent)
	{
		setupUi();
		retranslateUi();
	}

	void TorrentDetailView::setupUi()
	{
		QString valuePlaceholder = tr("...");

		QVBoxLayout * layout = new QVBoxLayout;

		QFormLayout * activityLayout = new QFormLayout;
		QFormLayout * detailsLayout = new QFormLayout;

		layout->setObjectName("mainLayout");
		activityLayout->setObjectName("activityLayout");
		detailsLayout->setObjectName("detailsLayout");

		activityLayout->setLabelAlignment(Qt::AlignLeft);
		detailsLayout->setLabelAlignment(Qt::AlignLeft);

#define init(name) name = new QLabel(this); name->setObjectName(&#name[2]);

		init(m_activityGroupLabel);
		init(m_detailsGroupLabel);

#define init3(label, value, layout)                                                                \
		label = new QLabel(this); label->setObjectName(&#label[2]);                                \
		value = new QLabel(this); value->setObjectName(&#value[2]);                                \
		value->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);  \
		value->setText(valuePlaceholder);                                                          \
		label->setBuddy(value);                                                                    \
		layout->addRow(label, value);

#undef init
#define init(label, value) init3(label, value, activityLayout)

		init(m_haveLabel, m_haveValueLabel);
		init(m_availLabel, m_availValueLabel);
		init(m_uploadedLabel, m_uploadedValueLabel);
		init(m_downloadedLabel, m_downloadedValueLabel);
		init(m_stateLabel, m_stateValueLabel);
		init(m_runningTimeLabel, m_runningTimeValueLabel)
		init(m_remainingTimeLabel, m_remainingTimeValueLabel);
		init(m_lastActivityLabel, m_lastActivityValueLabel);
		init(m_errorLabel, m_errorValueLabel);

#undef init
#define init(label, value) init3(label, value, detailsLayout)

		init(m_sizeLabel, m_sizeValueLabel);
		init(m_locationLabel, m_locationValueLabel);
		init(m_hashLabel, m_hashValueLabel);
		init(m_privacyLabel, m_privacyValueLabel);
		init(m_originLabel, m_originValueLabel);

#undef init
#undef init3

		m_commentLabel = new QLabel(this);
		m_commentLabel->setObjectName("commentLabel");
		m_commentText = new QTextBrowser(this);
		m_commentText->setObjectName("commentText");
		m_commentLabel->setBuddy(m_commentText);
		detailsLayout->addRow(m_commentLabel, m_commentText);

		activityLayout->setContentsMargins(20, -1, -1, -1);
		detailsLayout->setContentsMargins(20, -1, -1, -1);

		layout->addWidget(m_activityGroupLabel);
		layout->addLayout(activityLayout);
		layout->addSpacing(20);
		layout->addWidget(m_detailsGroupLabel);
		layout->addLayout(detailsLayout);

		setLayout(layout);

		auto boldFont = this->font();
		boldFont.setBold(true);

		m_activityGroupLabel->setFont(boldFont);
		m_detailsGroupLabel->setFont(boldFont);

		//for (QObject * child : children())
		//{
		//	auto name = child->objectName();
		//	if (not name.contains("ValueLabel"))
		//		continue;
		//
		//	QLabel * valLabel = static_cast<QLabel *>(child);
		//	QLabel * buddy = findChild<QLabel *>(name.replace("Value", ""));
		//
		//	assert(buddy);
		//	buddy->setBuddy(valLabel);
		//	valLabel->setText(valuePlaceholder);
		//	valLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
		//}

	}

	void TorrentDetailView::retranslateUi()
	{
		m_activityGroupLabel->setText(tr("Activity"));
		m_detailsGroupLabel->setText(tr("Details"));

		m_haveLabel->setText(tr("Have:"));
		m_availLabel->setText(tr("Availability:"));
		m_uploadedLabel->setText(tr("Uploaded:"));
		m_downloadedLabel->setText(tr("Downloaded:"));
		m_stateLabel->setText(tr("State:"));
		m_runningTimeLabel->setText(tr("Running Time:"));
		m_remainingTimeLabel->setText(tr("Remaining Time:"));
		m_lastActivityLabel->setText(tr("Last Activity:"));
		m_errorLabel->setText(tr("Error:"));

		m_sizeLabel->setText(tr("Size:"));
		m_locationLabel->setText(tr("Location:"));
		m_hashLabel->setText(tr("Hash:"));
		m_privacyLabel->setText(tr("Privacy:"));
		m_originLabel->setText(tr("Origin:"));
		m_commentLabel->setText(tr("Comment:"));
	}
}
