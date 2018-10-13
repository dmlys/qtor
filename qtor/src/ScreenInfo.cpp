#include <string>
#include <ostream>
#include <sstream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <QtCore/QString>
#include <QtGui/QScreen>
#include <QtGui/QGuiApplication>
#include <QtTools/ToolsBase.hpp>

#include "ScreenInfo.hpp"

namespace QtTools
{
	// Helper function to return display orientation as a string.
	static std::string Text(Qt::ScreenOrientation orientation)
	{
		switch (orientation)
		{
			case Qt::PrimaryOrientation:           return "Primary";
			case Qt::LandscapeOrientation:         return "Landscape";
			case Qt::PortraitOrientation:          return "Portrait";
			case Qt::InvertedLandscapeOrientation: return "Inverted landscape";
			case Qt::InvertedPortraitOrientation:  return "Inverted portrait";
			default: return "Unknown";
		}
	}

	static std::string Text(const QRect & rect)
	{
		return fmt::format("{} {} {}x{}", rect.x(), rect.y(), rect.width(), rect.height());
	}

	static std::string Text(const QSize & sz)
	{
		return fmt::format("{}x{}", sz.width(), sz.height());
	}

	static std::string Text(const QSizeF & sz)
	{
		return fmt::format("{}x{}", sz.width(), sz.height());
	}


	std::ostream & ScreenInfo(std::ostream & os)
	{
		auto screens = QGuiApplication::screens();
		fmt::print(os, "Number of screens: {}\n", screens.size());
		fmt::print(os, "Primary screen: {}\n", QGuiApplication::primaryScreen()->name());

		for (const QScreen * screen : screens)
		{
			fmt::print(os, "Information for screen: {}\n", screen->name());
			fmt::print(os, "  Refresh rate: {}Hz\n", screen->refreshRate());
			fmt::print(os, "  Depth: {}\n", screen->depth());
			fmt::print(os, "  Primary orientation: {}\n", Text(screen->primaryOrientation()));
			fmt::print(os, "  Orientation: {}\n", Text(screen->orientation()));

			fmt::print(os, "  Geometry: {}\n", Text(screen->geometry()));
			fmt::print(os, "  Size: {}\n", Text(screen->size()));
			fmt::print(os, "  Virtual geometry: {}\n", Text(screen->virtualGeometry()));
			fmt::print(os, "  Virtual size: {}\n", Text(screen->virtualSize()));

			fmt::print(os, "  Available geometry: {}\n", Text(screen->availableGeometry()));
			fmt::print(os, "  Available size: {}\n", Text(screen->availableSize()));
			fmt::print(os, "  Available virtual geometry: {}\n", Text(screen->availableVirtualGeometry()));
			fmt::print(os, "  Available virtual size: {}\n", Text(screen->availableVirtualSize()));

			fmt::print(os, "  Logical DPI: {}\n", screen->logicalDotsPerInch());
			fmt::print(os, "  Logical DPI X: {}\n", screen->logicalDotsPerInchX());
			fmt::print(os, "  Logical DPI Y: {}\n", screen->logicalDotsPerInchY());

			fmt::print(os, "  Physical DPI: {}\n", screen->physicalDotsPerInch());
			fmt::print(os, "  Physical DPI X: {}\n", screen->physicalDotsPerInchX());
			fmt::print(os, "  Physical DPI Y: {}\n", screen->physicalDotsPerInchY());
			fmt::print(os, "  Physical size: {} mm\n", Text(screen->physicalSize()));
		}

		return os;
	}

	QString ScreenInfo()
	{
		std::ostringstream os;
		ScreenInfo(os);

		return ToQString(os.str());
	}
}

