/*
 * Equation Of Time plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SkyGui.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "Planet.hpp"
#include "EquationOfTime.hpp"
#include "EquationOfTimeWindow.hpp"

#include "sidereal_time.h"

#include <QFontMetrics>
#include <QSettings>
#include <QPixmap>
#include <cmath>

StelModule* EquationOfTimeStelPluginInterface::getStelModule() const
{
	return new EquationOfTime();
}

StelPluginInfo EquationOfTimeStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(EquationOfTime);

	StelPluginInfo info;
	info.id = "EquationOfTime";
	info.displayedName = N_("Equation of Time");
	info.authors = "Alexander Wolf";
	info.contact = "http://stellarium.org";
	info.description = N_("This plugin shows the solution of the equation of time.");
	info.version = EQUATIONOFTIME_PLUGIN_VERSION;
	return info;
}

EquationOfTime::EquationOfTime()
	: flagShowSolutionEquationOfTime(false)
	, flagUseInvertedValue(false)
	, flagUseMsFormat(false)
	, flagEnableAtStartup(false)
	, flagShowEOTButton(false)
	, fontSize(20)
	, toolbarButton(NULL)
{
	setObjectName("EquationOfTime");
	mainWindow = new EquationOfTimeWindow();
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

EquationOfTime::~EquationOfTime()
{
	delete mainWindow;
}

void EquationOfTime::init()
{
	StelApp &app = StelApp::getInstance();
	if (!conf->childGroups().contains("EquationOfTime"))
	{
		qDebug() << "EquationOfTime: no EquationOfTime section exists in main config file - creating with defaults";
		restoreDefaultConfigIni();
	}

	// populate settings from main config file.
	readSettingsFromConfig();

	addAction("actionShow_EquationOfTime", N_("Equation of Time"), N_("Show solution for Equation of Time"), "showEOT", "Ctrl+Alt+T");

	enableEquationOfTime(getFlagEnableAtStartup());
	setFlagShowEOTButton(flagShowEOTButton);

	// Initialize the message strings and make sure they are translated when
	// the language changes.
	updateMessageText();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateMessageText()));
}

void EquationOfTime::deinit()
{
	//
}

void EquationOfTime::draw(StelCore *core)
{
	if (!isEnabled())
		return;

	StelPainter sPainter(core->getProjection2d());
	sPainter.setColor(textColor[0], textColor[1], textColor[2], 1.f);
	font.setPixelSize(getFontSize());
	sPainter.setFont(font);

	QString timeText;
	double time = getSolutionEquationOfTime(core->getJDay());

	if (getFlagInvertedValue())
		time *= -1;

	if (getFlagMsFormat())
	{
		double seconds = std::abs(round((time - (int)time)*60));
		QString messageSecondsValue;
		if (seconds<10.)
			messageSecondsValue = QString("0%1").arg(QString::number(seconds, 'f', 0));
		else
			messageSecondsValue = QString("%1").arg(QString::number(seconds, 'f', 0));

		timeText = QString("%1: %2%3%4%5").arg(messageEquation, QString::number((int)time), messageEquationMinutes, messageSecondsValue, messageEquationSeconds);
	}
	else
		timeText = QString("%1: %2%3").arg(messageEquation, QString::number(time, 'f', 2), messageEquationMinutes);


	QFontMetrics fm(font);
	QSize fs = fm.size(Qt::TextSingleLine, timeText);	
	if (core->getCurrentPlanet().data()->getEnglishName()=="Earth")
		sPainter.drawText(gui->getSkyGui()->getSkyGuiWidth()/2 - fs.width()/2, gui->getSkyGui()->getSkyGuiHeight() - fs.height()*1.5, timeText);

	//qDebug() << timeText;
}

double EquationOfTime::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}

bool EquationOfTime::configureGui(bool show)
{
	if (show)
	{
		mainWindow->setVisible(true);
	}

	return true;
}

void EquationOfTime::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void EquationOfTime::restoreDefaultConfigIni(void)
{
	conf->beginGroup("EquationOfTime");

	// delete all existing Equation Of Time settings...
	conf->remove("");

	conf->setValue("enable_at_startup", false);
	conf->setValue("flag_use_ms_format", true);
	conf->setValue("flag_use_inverted_value", false);
	conf->setValue("flag_show_button", true);
	conf->setValue("text_color", "0,0.5,1");
	conf->setValue("font_size", 20);

	conf->endGroup();
}

void EquationOfTime::readSettingsFromConfig(void)
{
	conf->beginGroup("EquationOfTime");

	setFlagEnableAtStartup(conf->value("enable_at_startup", false).toBool());
	setFlagMsFormat(conf->value("flag_use_ms_format", true).toBool());
	setFlagInvertedValue(conf->value("flag_use_inverted_value", false).toBool());
	textColor = StelUtils::strToVec3f(conf->value("text_color", "0,0.5,1").toString());
	setFontSize(conf->value("font_size", 20).toInt());
	flagShowEOTButton = conf->value("flag_show_button", true).toBool();

	conf->endGroup();
}

void EquationOfTime::saveSettingsToConfig(void)
{
	conf->beginGroup("EquationOfTime");

	conf->setValue("enable_at_startup", getFlagEnableAtStartup());
	conf->setValue("flag_use_ms_format", getFlagMsFormat());
	conf->setValue("flag_use_inverted_value", getFlagInvertedValue());
	conf->setValue("flag_show_button", getFlagShowEOTButton());
	//conf->setValue("text_color", "0,0.5,1");
	conf->setValue("font_size", getFontSize());

	conf->endGroup();
}

double EquationOfTime::getSolutionEquationOfTime(const double JDay) const
{
	StelCore* core = StelApp::getInstance().getCore();

	double tau = (JDay - 2451545.0)/365250.0;
	double sunMeanLongitude = 280.4664567 + tau*(360007.6892779 + tau*(0.03032028 + tau*(1/49931 - tau*(1/15300 - tau/2000000))));

	// reduce the angle
	sunMeanLongitude = std::fmod(sunMeanLongitude, 360.);
	// force it to be the positive remainder, so that 0 <= angle < 360
	sunMeanLongitude = std::fmod(sunMeanLongitude + 360., 360.);

	Vec3d pos = GETSTELMODULE(StelObjectMgr)->searchByName("Sun")->getEquinoxEquatorialPos(core);
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, pos);

	// covert radians to degrees and reduce the angle
	double alpha = std::fmod(ra*180./M_PI, 360.);
	// force it to be the positive remainder, so that 0 <= angle < 360
	alpha = std::fmod(alpha + 360., 360.);

	double equation = 4*(sunMeanLongitude - 0.0057183 - alpha + get_nutation_longitude(JDay)*cos(get_mean_ecliptical_obliquity(JDay)));
	// The equation of time is always smaller 20 minutes in absolute value
	if (std::abs(equation)>20)
	{
		// If absolute value of the equation of time appears to be too large, add 24 hours (1440 minutes) to or subtract it from our result
		if (equation>0.)
			equation -= 1440.;
		else
			equation += 1440.;
	}

	return equation;
}

void EquationOfTime::updateMessageText()
{
	messageEquation = q_("Equation of Time");
	// TRANSLATORS: minutes.
	messageEquationMinutes = qc_("m", "time");
	// TRANSLATORS: seconds.
	messageEquationSeconds = qc_("s", "time");
}

void EquationOfTime::setFlagShowEOTButton(bool b)
{
	if (b==true) {
		if (toolbarButton==NULL) {
			// Create the button
			toolbarButton = new StelButton(NULL,
						       QPixmap(":/EquationOfTime/bt_EquationOfTime_On.png"),
						       QPixmap(":/EquationOfTime/bt_EquationOfTime_Off.png"),
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_EquationOfTime");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} else {
		gui->getButtonBar()->hideButton("actionShow_EquationOfTime");
	}
	flagShowEOTButton = b;
}

void EquationOfTime::enableEquationOfTime(bool b)
{
	flagShowSolutionEquationOfTime = b;
}

void EquationOfTime::setFlagInvertedValue(bool b)
{
	flagUseInvertedValue=b;
}

void EquationOfTime::setFlagMsFormat(bool b)
{
	flagUseMsFormat=b;
}
//! Enable plugin usage at startup
void EquationOfTime::setFlagEnableAtStartup(bool b)
{
	flagEnableAtStartup=b;
}
//! Set font size for message
void EquationOfTime::setFontSize(int size)
{
	fontSize=size;
}

