/*
 * Copyright (C) 2013 Alexander Wolf
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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "StelTranslator.hpp"
#include "StelTextureMgr.hpp"
#include "LabelMgr.hpp"
#include "Nova.hpp"
#include "Novae.hpp"
#include "NovaeDialog.hpp"
#include "StelProgressController.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QProgressBar>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QList>
#include <QSettings>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QDir>

#define CATALOG_FORMAT_VERSION 1 /* Version of format of catalog */

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* NovaeStelPluginInterface::getStelModule() const
{
	return new Novae();
}

StelPluginInfo NovaeStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Novae);

	StelPluginInfo info;
	info.id = "Novae";
	info.displayedName = N_("Bright Novae");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("A plugin that shows some bright novae in the Milky Way galaxy.");
	info.version = NOVAE_PLUGIN_VERSION;
	return info;
}

/*
 Constructor
*/
Novae::Novae()
	: NovaCnt(0)
	, texPointer(NULL)
	, updateState(CompleteNoUpdates)
	, downloadMgr(NULL)
	, progressBar(NULL)
	, updateTimer(NULL)
	, messageTimer(NULL)
	, updatesEnabled(false)
	, updateFrequencyDays(0)
{
	setObjectName("Novae");
	configDialog = new NovaeDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(conf->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Novae::~Novae()
{
	delete configDialog;
}

/*
 Reimplementation of the getCallOrder method
*/
double Novae::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Novae::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Novae");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Novae"))
		{
			qDebug() << "Novae: no Novae section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		novaeJsonPath = StelFileMgr::findFile("modules/Novae", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/novae.json";
		if (novaeJsonPath.isEmpty())
			return;

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");
		addAction("actionShow_Novae_ConfigDialog", N_("Bright Novae"), N_("Bright Novae configuration window"), configDialog, "visible");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Novae: init error:" << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(novaeJsonPath).exists())
	{
		if (!checkJsonFileFormat() || getJsonFileVersion()<CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Novae: novae.json does not exist - copying default file to" << QDir::toNativeSeparators(novaeJsonPath);
		restoreDefaultJsonFile();
	}

	qDebug() << "Novae: loading catalog file:" << QDir::toNativeSeparators(novaeJsonPath);

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

/*
 Draw our module. This should print name of first Nova in the main window
*/
void Novae::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const NovaP& n, nova)
	{
		if (n && n->initialized)
		{
			n->draw(core, &painter);
		}
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, painter);
	}
}

void Novae::drawPointer(StelCore* core, StelPainter &painter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nova");
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

QList<StelObjectP> Novae::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const NovaP& n, nova)
	{
		if (n->initialized)
		{
			equPos = n->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(n));
			}
		}
	}

	return result;
}

StelObjectP Novae::searchByName(const QString& englishName) const
{
	foreach(const NovaP& n, nova)
	{
		if (n->getEnglishName().toUpper() == englishName.toUpper() || n->getDesignation().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(n);
	}

	return NULL;
}

StelObjectP Novae::searchByNameI18n(const QString& nameI18n) const
{
	foreach(const NovaP& n, nova)
	{
		if (n->getNameI18n().toUpper() == nameI18n.toUpper() || n->getDesignation().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(n);
	}

	return NULL;
}

QStringList Novae::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem==0)
		return result;

	QString sn;
	bool find;
	foreach(const NovaP& n, nova)
	{
		sn = n->getNameI18n();
		find = false;
		if (useStartOfWords)
		{
			if (objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
				find = true;
		}
		else
		{
			if (sn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
				result << sn;
		}
	}

	result.sort();
	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Novae::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem==0)
		return result;

	QString sn;
	bool find;
	foreach(const NovaP& n, nova)
	{
		sn = n->getEnglishName();
		find = false;
		if (useStartOfWords)
		{
			if (objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
				find = true;
		}
		else
		{
			if (sn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
				result << sn;
		}

		sn = n->getDesignation();
		find = false;
		if (useStartOfWords)
		{
			if (objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
				find = true;
		}
		else
		{
			if (sn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
				result << sn;
		}
	}

	result.sort();
	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Novae::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach (const NovaP& n, nova)
		{
			result << n->getEnglishName();
		}
	}
	else
	{
		foreach (const NovaP& n, nova)
		{
			result << n->getNameI18n();
		}
	}
	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Novae::restoreDefaultJsonFile(void)
{
	if (QFileInfo(novaeJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/Novae/novae.json");
	if (!src.copy(novaeJsonPath))
	{
		qWarning() << "Novae: cannot copy JSOIN resource to" + QDir::toNativeSeparators(novaeJsonPath);
	}
	else
	{
		qDebug() << "Novae: copied default novae.json to" << QDir::toNativeSeparators(novaeJsonPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(novaeJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("Novae/last_update");
		lastUpdate = QDateTime::fromString("2012-05-24T12:00:00", Qt::ISODate);
	}
}

/*
  Creates a backup of the novae.json file called novae.json.old
*/
bool Novae::backupJsonFile(bool deleteOriginal)
{
	QFile old(novaeJsonPath);
	if (!old.exists())
	{
		qWarning() << "Novae: no file to backup";
		return false;
	}

	QString backupPath = novaeJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Novae: WARNING - could not remove old novae.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Novae: WARNING - failed to copy novae.json to novae.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of novae.
*/
void Novae::readJsonFile(void)
{
	setNovaeMap(loadNovaeMap());
}

/*
  Parse JSON file and load novae to map
*/
QVariantMap Novae::loadNovaeMap(QString path)
{
	if (path.isEmpty())
	    path = novaeJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "Novae: cannot open" << QDir::toNativeSeparators(path);
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
void Novae::setNovaeMap(const QVariantMap& map)
{
	nova.clear();
	novalist.clear();
	NovaCnt=0;
	QVariantMap novaeMap = map.value("nova").toMap();
	foreach(QString novaeKey, novaeMap.keys())
	{
		QVariantMap novaeData = novaeMap.value(novaeKey).toMap();
		novaeData["designation"] = QString("%1").arg(novaeKey);

		novalist.insert(novaeData.value("name").toString(), novaeData.value("peakJD").toDouble());
		NovaCnt++;

		NovaP n(new Nova(novaeData));
		if (n->initialized)
			nova.append(n);

	}
}

int Novae::getJsonFileVersion(void)
{	
	int jsonVersion = -1;
	QFile novaeJsonFile(novaeJsonPath);
	if (!novaeJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Novae: cannot open" << QDir::toNativeSeparators(novaeJsonPath);
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&novaeJsonFile).toMap();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	novaeJsonFile.close();
	qDebug() << "Novae: version of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool Novae::checkJsonFileFormat()
{
	QFile novaeJsonFile(novaeJsonPath);
	if (!novaeJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Novae: cannot open" << QDir::toNativeSeparators(novaeJsonPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&novaeJsonFile).toMap();
		novaeJsonFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "Novae: file format is wrong! Error:" << e.what();
		return false;
	}

	return true;
}

NovaP Novae::getByID(const QString& id)
{
	foreach(const NovaP& n, nova)
	{
		if (n->initialized && n->designation == id)
			return n;
	}
	return NovaP();
}

bool Novae::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Novae::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void Novae::restoreDefaultConfigIni(void)
{
	conf->beginGroup("Novae");

	// delete all existing Novae settings...
	conf->remove("");

	conf->setValue("updates_enabled", true);
	conf->setValue("url", "http://stellarium.org/json/novae.json");
	conf->setValue("update_frequency_days", 100);
	conf->endGroup();
}

void Novae::readSettingsFromConfig(void)
{
	conf->beginGroup("Novae");

	updateUrl = conf->value("url", "http://stellarium.org/json/novae.json").toString();
	updateFrequencyDays = conf->value("update_frequency_days", 100).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2013-08-28T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();

	conf->endGroup();
}

void Novae::saveSettingsToConfig(void)
{
	conf->beginGroup("Novae");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_days", updateFrequencyDays);
	conf->setValue("updates_enabled", updatesEnabled );

	conf->endGroup();
}

int Novae::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyDays * 3600 * 24);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Novae::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyDays * 3600 * 24) <= QDateTime::currentDateTime())
		updateJSON();
}

void Novae::updateJSON(void)
{
	if (updateState==Novae::Updating)
	{
		qWarning() << "Novae: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "Novae: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	conf->setValue("Novae/last_update", lastUpdate.toString(Qt::ISODate));

	updateState = Novae::Updating;
	emit(updateStateChanged(updateState));

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().addProgressBar();

	progressBar->setValue(0);
	progressBar->setRange(0, 100);
	progressBar->setFormat("Update novae");
	

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Bright Novae Plugin %1; http://stellarium.org/)").arg(NOVAE_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	updateState = Novae::CompleteUpdates;
	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void Novae::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Novae: FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		QString jsonFilePath = StelFileMgr::findFile("modules/Novae", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/novae.json";
		if (jsonFilePath.isEmpty())
		{
			qWarning() << "Novae: cannot write JSON data to file";
		}
		else
		{
			QFile jsonFile(jsonFilePath);
			if (jsonFile.exists())
				jsonFile.remove();

			if(jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				jsonFile.write(reply->readAll());
				jsonFile.close();
			}
		}
	}

	if (progressBar)
	{
		progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = NULL;
	}
}

void Novae::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Novae::messageTimeout(void)
{
	foreach(int i, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}

QString Novae::getNovaeList()
{
	QString smonth[] = {q_("January"), q_("February"), q_("March"), q_("April"), q_("May"), q_("June"), q_("July"), q_("August"), q_("September"), q_("October"), q_("November"), q_("December")};
	QStringList out;
	int year, month, day;
	QList<double> vals = novalist.values();
	qSort(vals);
	foreach(double val, vals)
	{
		StelUtils::getDateFromJulianDay(val, &year, &month, &day);
		out << QString("%1 (%2 %3 %4)").arg(novalist.key(val)).arg(day).arg(smonth[month-1]).arg(year);
	}

	return out.join(", ");
}

float Novae::getLowerLimitBrightness()
{
	float lowerLimit = 10.f;
	QFile novaeJsonFile(novaeJsonPath);
	if (!novaeJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Novae: cannot open" << QDir::toNativeSeparators(novaeJsonPath);
		return lowerLimit;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&novaeJsonFile).toMap();
	if (map.contains("limit"))
	{
		lowerLimit = map.value("limit").toFloat();
	}

	novaeJsonFile.close();
	return lowerLimit;
}
