/*
 * Copyright (C) 2012 Alexander Wolf
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "config.h"

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "LabelMgr.hpp"
#include "Exoplanets.hpp"
#include "Exoplanet.hpp"
#include "ExoplanetsDialog.hpp"
#include "StelActionMgr.hpp"
#include "StelProgressController.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QVariantMap>
#include <QVariant>
#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QPixmap>
#include <QDir>
#include <QSettings>

#define CATALOG_FORMAT_VERSION 1 /* Version of format of catalog */

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* ExoplanetsStelPluginInterface::getStelModule() const
{
	return new Exoplanets();
}

StelPluginInfo ExoplanetsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Exoplanets);

	StelPluginInfo info;
	info.id = "Exoplanets";
	info.displayedName = N_("Exoplanets");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("This plugin plots the position of stars with exoplanets. Exoplanets data is derived from the 'Extrasolar Planets Encyclopaedia' at exoplanet.eu");
	info.version = EXOPLANETS_PLUGIN_VERSION;
	return info;
}

/*
 Constructor
*/
Exoplanets::Exoplanets()
	: PSCount(0)
	, EPCountAll(0)
	, EPCountPH(0)
	, updateState(CompleteNoUpdates)
	, downloadMgr(NULL)
	, updateTimer(0)
	, messageTimer(0)
	, updatesEnabled(false)
	, updateFrequencyHours(0)
	, enableAtStartup(false)
	, flagShowExoplanets(false)
	, flagShowExoplanetsButton(false)
	, toolbarButton(NULL)
	, progressBar(NULL)
{
	setObjectName("Exoplanets");
	exoplanetsConfigDialog = new ExoplanetsDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(conf->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Exoplanets::~Exoplanets()
{
	StelApp::getInstance().getStelObjectMgr().unSelect();

	delete exoplanetsConfigDialog;
}

void Exoplanets::deinit()
{
	ep.clear();
	Exoplanet::markerTexture.clear();
	texPointer.clear();
}

void Exoplanets::update(double) //deltaTime
{
	//
}

/*
 Reimplementation of the getCallOrder method
*/
double Exoplanets::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Exoplanets::init()
{
	upgradeConfigIni();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Exoplanets");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Exoplanets"))
		{
			qDebug() << "Exoplanets: no Exoplanets section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		jsonCatalogPath = StelFileMgr::findFile("modules/Exoplanets", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/exoplanets.json";
		if (jsonCatalogPath.isEmpty())
			return;

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");
		Exoplanet::markerTexture = StelApp::getInstance().getTextureManager().createTexture(":/Exoplanets/exoplanet.png");

		// key bindings and other actions
		addAction("actionShow_Exoplanets", N_("Exoplanets"), N_("Show exoplanets"), "showExoplanets", "Ctrl+Alt+E");
		addAction("actionShow_Exoplanets_ConfigDialog", N_("Exoplanets"), N_("Exoplanets configuration window"), exoplanetsConfigDialog, "visible");

		setFlagShowExoplanets(getEnableAtStartup());
		setFlagShowExoplanetsButton(flagShowExoplanetsButton);
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Exoplanets: init error: " << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(jsonCatalogPath).exists())
	{
		if (!checkJsonFileFormat() || getJsonFileFormatVersion()<CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Exoplanets: exoplanets.json does not exist - copying default catalog to " << QDir::toNativeSeparators(jsonCatalogPath);
		restoreDefaultJsonFile();
	}

	qDebug() << "Exoplanets: loading catalog file:" << QDir::toNativeSeparators(jsonCatalogPath);

	readJsonFile();

	// Set up download manager and the update schedule
	downloadMgr = new QNetworkAccessManager(this);
	connect(downloadMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(updateDownloadComplete(QNetworkReply*)));
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void Exoplanets::draw(StelCore* core)
{
	if (!flagShowExoplanets)
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const ExoplanetP& eps, ep)
	{
		if (eps && eps->initialized)
			eps->draw(core, &painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void Exoplanets::drawPointer(StelCore* core, StelPainter& painter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Exoplanet");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		painter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

QList<StelObjectP> Exoplanets::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	if (!flagShowExoplanets)
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->initialized)
		{
			equPos = eps->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(eps));
			}
		}
	}

	return result;
}

StelObjectP Exoplanets::searchByName(const QString& englishName) const
{
	if (!flagShowExoplanets)
		return NULL;

	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(eps);
	}

	return NULL;
}

StelObjectP Exoplanets::searchByNameI18n(const QString& nameI18n) const
{
	if (!flagShowExoplanets)
		return NULL;

	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(eps);
	}

	return NULL;
}

QStringList Exoplanets::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;	
	if (!flagShowExoplanets)
		return result;

	if (maxNbItem==0)
		return result;

	QString epsn;
	bool find;
	foreach(const ExoplanetP& eps, ep)
	{
		epsn = eps->getNameI18n();
		find = false;
		if (useStartOfWords)
		{
			if (epsn.toUpper().left(objPrefix.length()) == objPrefix.toUpper())
				find = true;
		}
		else
		{
			if (epsn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
			result << epsn;
		}
	}

	result.sort();

	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Exoplanets::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!flagShowExoplanets)
		return result;

	if (maxNbItem==0)
		return result;

	QString epsn;
	bool find;
	foreach(const ExoplanetP& eps, ep)
	{
		epsn = eps->getNameI18n();
		find = false;
		if (useStartOfWords)
		{
			if (epsn.toUpper().left(objPrefix.length()) == objPrefix.toUpper())
				find = true;
		}
		else
		{
			if (epsn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
			result << epsn;
		}
	}

	result.sort();

	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}


QStringList Exoplanets::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach (const ExoplanetP& planet, ep)
		{
			result << planet->getEnglishName();
		}
	}
	else
	{
		foreach (const ExoplanetP& planet, ep)
		{
			result << planet->getNameI18n();
		}
	}
	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Exoplanets::restoreDefaultJsonFile(void)
{
	if (QFileInfo(jsonCatalogPath).exists())
		backupJsonFile(true);

	QFile src(":/Exoplanets/exoplanets.json");
	if (!src.copy(jsonCatalogPath))
	{
		qWarning() << "Exoplanets: cannot copy JSON resource to " + QDir::toNativeSeparators(jsonCatalogPath);
	}
	else
	{
		qDebug() << "Exoplanets: default exoplanets.json to " << QDir::toNativeSeparators(jsonCatalogPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(jsonCatalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("Exoplanets/last_update");
		lastUpdate = QDateTime::fromString("2012-05-24T12:00:00", Qt::ISODate);
	}
}

/*
  Creates a backup of the exoplanets.json file called exoplanets.json.old
*/
bool Exoplanets::backupJsonFile(bool deleteOriginal)
{
	QFile old(jsonCatalogPath);
	if (!old.exists())
	{
		qWarning() << "Exoplanets: no file to backup";
		return false;
	}

	QString backupPath = jsonCatalogPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Exoplanets: WARNING - could not remove old exoplanets.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Exoplanets: WARNING - failed to copy exoplanets.json to exoplanets.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of exoplanets.
*/
void Exoplanets::readJsonFile(void)
{
	setEPMap(loadEPMap());
	emit(updateStateChanged(updateState));
}

/*
  Parse JSON file and load exoplanets to map
*/
QVariantMap Exoplanets::loadEPMap(QString path)
{
	if (path.isEmpty())
	    path = jsonCatalogPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "Exoplanets: cannot open " << QDir::toNativeSeparators(path);
	else
	{
		map = StelJsonParser::parse(jsonFile.readAll()).toMap();
		jsonFile.close();
	}
	return map;
}

/*
  Set items for list of struct from data map
*/
void Exoplanets::setEPMap(const QVariantMap& map)
{
	ep.clear();
	PSCount = EPCountAll = EPCountPH = 0;
	QVariantMap epsMap = map.value("stars").toMap();
	foreach(QString epsKey, epsMap.keys())
	{
		QVariantMap epsData = epsMap.value(epsKey).toMap();
		epsData["designation"] = epsKey;

		PSCount++;

		ExoplanetP eps(new Exoplanet(epsData));
		if (eps->initialized)
		{
			ep.append(eps);
			EPCountAll += eps->getCountExoplanets();
			EPCountPH += eps->getCountHabitableExoplanets();
		}

	}
}

int Exoplanets::getJsonFileFormatVersion(void)
{
	int jsonVersion = -1;
	QFile jsonEPCatalogFile(jsonCatalogPath);
	if (!jsonEPCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Exoplanets: cannot open " << QDir::toNativeSeparators(jsonCatalogPath);
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&jsonEPCatalogFile).toMap();
	jsonEPCatalogFile.close();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	qDebug() << "Exoplanets: version of the format of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool Exoplanets::checkJsonFileFormat()
{
	QFile jsonEPCatalogFile(jsonCatalogPath);
	if (!jsonEPCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Exoplanets: cannot open " << QDir::toNativeSeparators(jsonCatalogPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&jsonEPCatalogFile).toMap();
		jsonEPCatalogFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "Exoplanets: file format is wrong! Error:" << e.what();
		return false;
	}

	return true;
}

ExoplanetP Exoplanets::getByID(const QString& id)
{
	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->initialized && eps->designation == id)
			return eps;
	}
	return ExoplanetP();
}

bool Exoplanets::configureGui(bool show)
{
	if (show)
		exoplanetsConfigDialog->setVisible(true);
	return true;
}

void Exoplanets::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void Exoplanets::restoreDefaultConfigIni(void)
{
	conf->beginGroup("Exoplanets");

	// delete all existing Exoplanets settings...
	conf->remove("");

	conf->setValue("distribution_enabled", false);
	conf->setValue("timeline_enabled", false);
	conf->setValue("enable_at_startup", false);
	conf->setValue("updates_enabled", true);	
	conf->setValue("url", "http://stellarium.org/json/exoplanets.json");
	conf->setValue("update_frequency_hours", 72);
	conf->setValue("flag_show_exoplanets_button", true);
	conf->setValue("habitable_exoplanet_marker_color", "1.0,0.5,0.0");
	conf->setValue("exoplanet_marker_color", "0.4,0.9,0.5");
	conf->endGroup();
}

void Exoplanets::readSettingsFromConfig(void)
{
	conf->beginGroup("Exoplanets");

	updateUrl = conf->value("url", "http://stellarium.org/json/exoplanets.json").toString();
	updateFrequencyHours = conf->value("update_frequency_hours", 72).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-05-24T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	setDisplayMode(conf->value("distribution_enabled", false).toBool());
	setTimelineMode(conf->value("timeline_enabled", false).toBool());
	enableAtStartup = conf->value("enable_at_startup", false).toBool();
	flagShowExoplanetsButton = conf->value("flag_show_exoplanets_button", true).toBool();
	setMarkerColor(conf->value("exoplanet_marker_color", "0.4,0.9,0.5").toString(), false);
	setMarkerColor(conf->value("habitable_exoplanet_marker_color", "1.0,0.5,0.0").toString(), true);

	conf->endGroup();
}

void Exoplanets::saveSettingsToConfig(void)
{
	conf->beginGroup("Exoplanets");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_hours", updateFrequencyHours);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("distribution_enabled", getDisplayMode());
	conf->setValue("timeline_enabled", getTimelineMode());
	conf->setValue("enable_at_startup", enableAtStartup);
	conf->setValue("flag_show_exoplanets_button", flagShowExoplanetsButton);
	conf->setValue("habitable_exoplanet_marker_color", getMarkerColor(true));
	conf->setValue("exoplanet_marker_color", getMarkerColor(false));

	conf->endGroup();
}

int Exoplanets::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyHours * 3600);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Exoplanets::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime())
		updateJSON();
}

void Exoplanets::updateJSON(void)
{
	if (updateState==Exoplanets::Updating)
	{
		qWarning() << "Exoplanets: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "Exoplanets: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	conf->setValue("Exoplanets/last_update", lastUpdate.toString(Qt::ISODate));

	updateState = Exoplanets::Updating;
	emit(updateStateChanged(updateState));

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().addProgressBar();

	progressBar->setValue(0);
	progressBar->setRange(0, 100);
	progressBar->setFormat("Update exoplanets");

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Exoplanets Plugin %1; http://stellarium.org/)").arg(EXOPLANETS_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	updateState = Exoplanets::CompleteUpdates;
	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void Exoplanets::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Exoplanets: FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		try
		{
			QString jsonFilePath = StelFileMgr::findFile("modules/Exoplanets", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/exoplanets.json";
			QFile jsonFile(jsonFilePath);
			if (jsonFile.exists())
				jsonFile.remove();

			if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				jsonFile.write(reply->readAll());
				jsonFile.close();
			}
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "Exoplanets: cannot write JSON data to file:" << e.what();
		}

	}

	if (progressBar)
	{
		progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = NULL;
	}

	readJsonFile();
}

void Exoplanets::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Exoplanets::messageTimeout(void)
{
	foreach(int i, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}

void Exoplanets::upgradeConfigIni(void)
{
	// Upgrade settings for Exoplanets plugin
	if (conf->contains("Exoplanets/flag_show_exoplanets"))
	{
		bool b = conf->value("Exoplanets/flag_show_exoplanets", false).toBool();
		if (!conf->contains("Exoplanets/enable_at_startup"))
			conf->setValue("Exoplanets/enable_at_startup", b);
		conf->remove("Exoplanets/flag_show_exoplanets");
	}
}

// Define whether the button toggling exoplanets should be visible
void Exoplanets::setFlagShowExoplanetsButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=NULL)
	{
		if (b==true)
		{
			if (toolbarButton==NULL) {
				// Create the exoplanets button
				toolbarButton = new StelButton(NULL,
							       QPixmap(":/Exoplanets/btExoplanets-on.png"),
							       QPixmap(":/Exoplanets/btExoplanets-off.png"),
							       QPixmap(":/graphicGui/glow32x32.png"),
							       "actionShow_Exoplanets");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Exoplanets");
		}
	}
	flagShowExoplanetsButton = b;
}

bool Exoplanets::getDisplayMode()
{
	return Exoplanet::distributionMode;
}

void Exoplanets::setDisplayMode(bool b)
{
	Exoplanet::distributionMode=b;
}

bool Exoplanets::getTimelineMode()
{
	return Exoplanet::timelineMode;
}

void Exoplanets::setTimelineMode(bool b)
{
	Exoplanet::timelineMode=b;
}

QString Exoplanets::getMarkerColor(bool habitable)
{
	Vec3f c;
	if (habitable)
		c = Exoplanet::habitableExoplanetMarkerColor;
	else
		c = Exoplanet::exoplanetMarkerColor;
	return QString("%1,%2,%3").arg(c[0]).arg(c[1]).arg(c[2]);
}

void Exoplanets::setMarkerColor(QString c, bool h)
{
	Vec3f nc = StelUtils::strToVec3f(c);
	if (h)
		Exoplanet::habitableExoplanetMarkerColor = nc;
	else
		Exoplanet::exoplanetMarkerColor = nc;
}
