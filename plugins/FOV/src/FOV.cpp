/*
 * FOV plug-in for Stellarium
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

#include "FOV.hpp"
#include "FOVWindow.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelMovementMgr.hpp"

#include <QSettings>

StelModule* FOVStelPluginInterface::getStelModule() const
{
	return new FOV();
}

StelPluginInfo FOVStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "FOV";
	info.displayedName = N_("Field of View");
	info.authors = "Alexander Wolf";
	info.contact = "http://stellarium.org";
	info.description = N_("This plugin allows stepwise zooming via keyboard shortcuts like in the <em>Cartes du Ciel</em> planetarium program.");
	info.version = FOV_VERSION;
	return info;
}

FOV::FOV()
{
	setObjectName("FOV");
	mainWindow = new FOVWindow();
	conf = StelApp::getInstance().getSettings();
}

FOV::~FOV()
{
	delete mainWindow;
}

void FOV::init()
{
	FOVitem.clear();
	FOVitem << -1 << -1 << -1 << -1 << -1 << -1 << -1 << -1 << -1 << -1;
	// Default values of FOV (degrees)
	FOVdefault.clear();
	FOVdefault << 0.5 << 180.0 << 90.0 << 60.0 << 45.0 << 20.0 << 10.0 << 5.0 << 2.0 << 1.0;

	if (!conf->childGroups().contains("fov"))
	{
		qDebug() << "FOV: no fov section exists in main config file - creating with defaults";
		restoreDefaultConfigIni();
	}

	// populate settings from main config file.
	readSettingsFromConfig();

	// key bindings
	QString section = N_("Field of View");
	for (int i=0; i<10; i++)
	{
		QString name = QString("actionSetFOV%1").arg(i);
		QString shortcut = QString("Ctrl+Alt+%1").arg(i);
		QString text = q_("Set FOV to %1%2").arg(getQuickFOV(i)).arg(QChar(0x00B0));
		addAction(name, section, text, "setFOV()", shortcut);
	}
}

void FOV::deinit()
{
	//
}

double FOV::getCallOrder(StelModuleActionName) const
{
	return 0.;
}

void FOV::update(double)
{
	//
}

bool FOV::configureGui(bool show)
{
	if (show)
	{
		mainWindow->setVisible(true);
	}

	return true;
}

void FOV::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void FOV::restoreDefaultConfigIni(void)
{
	conf->beginGroup("fov");

	// delete all existing FOV settings...
	conf->remove("");

	for (int i=0; i<10; i++)
	{
		conf->setValue(QString("fov_quick_%1").arg(i), FOVdefault.at(i));
	}

	conf->endGroup();
}

void FOV::readSettingsFromConfig(void)
{
	conf->beginGroup("fov");

	for(int i=0; i<10; i++)
	{
		setQuickFOV(conf->value(QString("fov_quick_%1").arg(i), FOVdefault.at(i)).toDouble(), i);
	}

	conf->endGroup();
}

void FOV::saveSettingsToConfig(void)
{
	conf->beginGroup("fov");

	for(int i=0; i<10; i++)
	{
		conf->setValue(QString("fov_quick_%1").arg(i), getQuickFOV(i));
	}

	conf->endGroup();
}

double FOV::getQuickFOV(const int item) const
{
	return FOVitem.at(item);
}

void FOV::setQuickFOV(const double value, const int item)
{
	FOVitem[item] = value;
}

void FOV::setFOV()
{
	// Find out for which FOV is the command
	if (sender() == NULL)
	{
		return;
	}
	// XXX: we could use a QSignalMapper instead of this trick.
	int slotNumber = sender()->objectName().right(1).toInt();

	StelMovementMgr *movementManager = StelApp::getInstance().getCore()->getMovementMgr();
	movementManager->zoomTo(getQuickFOV(slotNumber), 1.f); // One second for zooming
}
