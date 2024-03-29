#ifndef SETTINGS_H
#define SETTINGS_H

#include <QApplication>
#include <QTranslator>
#include <iostream>

enum Language
{
	SERBIAN = 0,
	ENGLISH
};

enum Theme
{
	BLUE = 0,
	ORANGE,
	DARK
};

class Settings
{
  private:
	Language m_language;
	Theme m_theme;
	double m_volume;
	bool m_sound;
	QApplication *m_app;
	QTranslator *m_translator;

	Settings(QApplication *app);
	~Settings();

	Settings(const Settings &) = delete;
	Settings &operator=(const Settings &) = delete;

  public:
	static Settings &instance(QApplication *app);

	void setLanguage(const int index);
	void setTheme(const int index);
	void setVolume(const double &newVolume);
	void setSound(const bool &newSound);
};

#endif // SETTINGS_H
