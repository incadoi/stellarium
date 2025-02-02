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

#ifndef _EQUATIONOFTIME_HPP_
#define _EQUATIONOFTIME_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QFont>
#include <QString>

class QPixmap;
class StelButton;
class EquationOfTimeWindow;

class EquationOfTime : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool showEOT
		   READ isEnabled
		   WRITE enableEquationOfTime)

public:
	EquationOfTime();
	virtual ~EquationOfTime();

	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore *core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Set up the plugin with default values.  This means clearing out the Pulsars section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);

	//! Get solution of equation of time
	//! Source: J. Meeus "Astronomical Algorithms" (2nd ed., with corrections as of August 10, 2009) p.183-187.
	//! @param JDay JD
	//! @return time in minutes
	double getSolutionEquationOfTime(const double JDay) const;

	//! Is plugin enabled?
	bool isEnabled() const
	{
		return flagShowSolutionEquationOfTime;
	}

	//! Get font size for messages
	int getFontSize(void)
	{
		return fontSize;
	}
	//! Get status of usage minutes and seconds for value of equation
	bool getFlagMsFormat(void) const
	{
		return flagUseMsFormat;
	}
	//! Get status of usage inverted values for equation of time
	bool getFlagInvertedValue(void) const
	{
		return flagUseInvertedValue;
	}
	bool getFlagEnableAtStartup(void) const
	{
		return flagEnableAtStartup;
	}
	bool getFlagShowEOTButton(void) const
	{
		return flagShowEOTButton;
	}

public slots:
	//! Enable plugin usage
	void enableEquationOfTime(bool b);
	//! Enable usage inverted value for equation of time (switch sign of equation)
	void setFlagInvertedValue(bool b);
	//! Enable usage minutes and seconds for value
	void setFlagMsFormat(bool b);
	//! Enable plugin usage at startup
	void setFlagEnableAtStartup(bool b);
	//! Set font size for message
	void setFontSize(int size);
	//! Display plugin button on toolbar
	void setFlagShowEOTButton(bool b);

private slots:
	void updateMessageText();

private:
	// if existing, delete EquationOfTime section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	EquationOfTimeWindow* mainWindow;
	QSettings* conf;
	StelGui* gui;

	QFont font;
	bool flagShowSolutionEquationOfTime;
	bool flagUseInvertedValue;
	bool flagUseMsFormat;
	bool flagEnableAtStartup;
	bool flagShowEOTButton;
	QString messageEquation;
	QString messageEquationMinutes;
	QString messageEquationSeconds;
	Vec3f textColor;
	int fontSize;
	StelButton* toolbarButton;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class EquationOfTimeStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /* _EQUATIONOFTIME_HPP_ */
