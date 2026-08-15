#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStyleFactory>
#include <QStyle>

#include "GuiWindow.hpp"

namespace {

QIcon ResolveAppIcon() {
	const QDir app_dir(QCoreApplication::applicationDirPath());

	const QStringList candidates = {
		app_dir.filePath("stevemodmaker.svg"),
		app_dir.filePath("../share/icons/hicolor/scalable/apps/stevemodmaker.svg"),
		app_dir.filePath("../packaging/stevemodmaker.svg"),
		QStringLiteral("packaging/stevemodmaker.svg")
	};

	for (const QString& path : candidates) {
		if (QFileInfo::exists(path)) {
			const QIcon icon(path);
			if (!icon.isNull()) {
				return icon;
			}
		}
	}

	const QIcon themed = QIcon::fromTheme("stevemodmaker");
	if (!themed.isNull()) {
		return themed;
	}

	return QApplication::style()->standardIcon(QStyle::SP_DesktopIcon);
}

} // namespace

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	app.setStyle(QStyleFactory::create("Fusion"));
	app.setWindowIcon(ResolveAppIcon());

	GuiWindow window;
	window.setWindowIcon(app.windowIcon());
	window.show();

	return app.exec();
}
