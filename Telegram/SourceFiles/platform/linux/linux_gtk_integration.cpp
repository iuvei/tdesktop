/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/linux/linux_gtk_integration.h"

#ifdef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#error "GTK integration depends on D-Bus integration."
#endif // DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#include "base/platform/linux/base_linux_gtk_integration.h"
#include "base/platform/linux/base_linux_dbus_utilities.h"
#include "base/platform/base_platform_info.h"

#ifndef DESKTOP_APP_DISABLE_WEBKITGTK
#include "webview/platform/linux/webview_linux_webkit2gtk.h"
#endif // !DESKTOP_APP_DISABLE_WEBKITGTK

#include <QtCore/QProcess>

#include <giomm.h>

namespace Platform {
namespace internal {
namespace {

constexpr auto kBaseService = "org.telegram.desktop.BaseGtkIntegration-%1"_cs;
constexpr auto kWebviewService = "org.telegram.desktop.GtkIntegration.WebviewHelper-%1-%2"_cs;

using BaseGtkIntegration = base::Platform::GtkIntegration;

} // namespace

QString GtkIntegration::AllowedBackends() {
	return Platform::IsWayland()
		? qsl("wayland,x11")
		: Platform::IsX11()
			? qsl("x11,wayland")
			: QString();
}

int GtkIntegration::Exec(
		Type type,
		const QString &parentDBusName,
		const QString &serviceName) {
	Glib::init();
	Gio::init();

	if (type == Type::Base) {
		BaseGtkIntegration::SetServiceName(serviceName);
		if (const auto integration = BaseGtkIntegration::Instance()) {
			return integration->exec(parentDBusName);
		}
#ifndef DESKTOP_APP_DISABLE_WEBKITGTK
	} else if (type == Type::Webview) {
		Webview::WebKit2Gtk::SetServiceName(serviceName.toStdString());
		return Webview::WebKit2Gtk::Exec(parentDBusName.toStdString());
#endif // !DESKTOP_APP_DISABLE_WEBKITGTK
	}

	return 1;
}

void GtkIntegration::Start(Type type) {
	if (type != Type::Base
		&& type != Type::Webview) {
		return;
	}

	const auto d = QFile::encodeName(QDir(cWorkingDir()).absolutePath());
	char h[33] = { 0 };
	hashMd5Hex(d.constData(), d.size(), h);

	if (type == Type::Base) {
		BaseGtkIntegration::SetServiceName(kBaseService.utf16().arg(h));
	} else if (type == Type::Webview) {
#ifndef DESKTOP_APP_DISABLE_WEBKITGTK
		Webview::WebKit2Gtk::SetServiceName(
			kWebviewService.utf16().arg(h).arg("%1").toStdString());
#endif // !DESKTOP_APP_DISABLE_WEBKITGTK

		return;
	}

	const auto dbusName = [] {
		try {
			static const auto connection = Gio::DBus::Connection::get_sync(
				Gio::DBus::BusType::BUS_TYPE_SESSION);

			return QString::fromStdString(connection->get_unique_name());
		} catch (...) {
			return QString();
		}
	}();

	if (dbusName.isEmpty()) {
		return;
	}

	QProcess::startDetached(cExeDir() + cExeName(), {
		qsl("-basegtkintegration"),
		dbusName,
		kBaseService.utf16().arg(h),
	});
}

void GtkIntegration::Autorestart(Type type) {
	if (type != Type::Base) {
		return;
	}

	try {
		static const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SESSION);

		base::Platform::DBus::RegisterServiceWatcher(
			connection,
			Glib::ustring(BaseGtkIntegration::ServiceName().toStdString()),
			[=](
				const Glib::ustring &service,
				const Glib::ustring &oldOwner,
				const Glib::ustring &newOwner) {
				if (newOwner.empty()) {
					Start(type);
				} else if (const auto integration = BaseGtkIntegration::Instance()) {
					integration->load(AllowedBackends());
				}
			});
	} catch (...) {
	}
}

} // namespace internal
} // namespace Platform
