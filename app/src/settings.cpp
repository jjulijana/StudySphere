#include <QApplication>
#include <QFile>

#include "../lib/settings.h"

#include <QFile>
#include <QTranslator>

Settings &Settings::instance(QApplication *app)
{
	static Settings settingsInstance(app); // Static local variable ensures a single instance
	return settingsInstance;
}

Settings::Settings(QApplication *app) : m_app(app), m_theme(BLUE), m_volume(1.0), m_sound(true)
{
	m_translator = new QTranslator;
}

Settings::~Settings()
{
	delete m_translator;
}

void Settings::setTheme(const int index)
{
	QString styleSheetName;

	switch (index) {
	case BLUE:
		styleSheetName = "Blue";
		m_theme = BLUE;
		break;
	case ORANGE:
		styleSheetName = "Orange";
		m_theme = ORANGE;
		break;
	case DARK:
		styleSheetName = "Dark";
		m_theme = DARK;
		break;

	default:
		return;
	}

	if (!styleSheetName.isEmpty()) {
		QString styleSheetPath = QString("../app/res/themes/%1.qss").arg(styleSheetName);
		QFile styleSheetFile(styleSheetPath);
		styleSheetFile.open(QFile::ReadOnly);
		QString styleSheet = QLatin1String(styleSheetFile.readAll());
		qApp->setStyleSheet(styleSheet);
	}
}

void Settings::setVolume(const double &newVolume)
{
	m_volume = newVolume;
}

void Settings::setSound(const bool &newSound)
{
	m_sound = newSound;
}

void Settings::setLanguage(const int index)
{
	QString languageFileName;

	switch (index) {
	case ENGLISH:
		languageFileName = "english";
		m_language = ENGLISH;
		break;
	case SERBIAN:
		languageFileName = "serbian";
		m_language = SERBIAN;
		break;

	default:
		return;
	}

	if (!languageFileName.isEmpty()) {
		QString filePath = QString("../app/res/languages/%1.qm").arg(languageFileName);
		if (m_translator->load(filePath)) {
			m_app->installTranslator(m_translator);
		}
	}
}
