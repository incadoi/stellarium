/*
 * Copyright (C) 2012 Ivan Marti-Vidal
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

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QSettings>
#include <QString>
#include <QTimer>

//#include <QtNetwork> // Why do we need a full part of the framwork again?

#include "Observability.hpp"
#include "ObservabilityDialog.hpp"

#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StarMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFader.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelObserver.hpp"
#include "StelProjector.hpp"
#include "StelSkyDrawer.hpp"
#include "StelUtils.hpp"
#include "ZoneArray.hpp"


StelModule* ObservabilityStelPluginInterface::getStelModule() const
{
	return new Observability();
}

StelPluginInfo ObservabilityStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Observability);

	StelPluginInfo info;
	info.id = "Observability";
	info.displayedName = N_("Observability Analysis");
	info.authors = "Ivan Marti-Vidal (Onsala Space Observatory)"; // non-translatable field
	info.contact = "i.martividal@gmail.com";
	info.description = N_("Displays an analysis of a selected object's observability (rise, set, and transit times) for the current date, as well as when it is observable through the year. An object is assumed to be observable if it is above the horizon during a fraction of the night. Also included are the dates of the largest separation from the Sun and acronychal and cosmical rising and setting. (Explanations are provided in the 'About' tab of the plugin's configuration window.)");
	info.version = OBSERVABILITY_PLUGIN_VERSION;
	return info;
}



Observability::Observability()
	: ObserverLoc(0.), flagShowReport(false), button(NULL)
{
	setObjectName("Observability");
	configDialog = new ObservabilityDialog();

	// TODO: Migrate to static const? --BM
	// Some useful constants:
	Rad2Deg = 180./3.1415927; // Convert degrees into radians
	Rad2Hr = 12./3.1415927;  // Convert hours into radians
	UA = 1.4958e+8;         // Astronomical Unit in Km.
	TFrac = 0.9972677595628414;  // Convert sidereal time into Solar time
	JDsec = 1./86400.;      // A second in days.
	halfpi = 1.57079632675; // pi/2
	MoonT = 29.530588; // Moon synodic period (used as first estimate of Full Moon).
	RefFullMoon = 2451564.696; // Reference Julian date of a Full Moon.
	MoonPerilune = 0.0024236308; // Smallest Earth-Moon distance (in AU).
	
	nextFullMoon = 0.0;
	prevFullMoon = 0.0;
	refractedHorizonAlt = 0.0;
	selName = "";

	// Dummy initial values for parameters and data vectors:
	mylat = 1000.; mylon = 1000.;
	myJD = 0.0;
	curYear = 0;
	isStar = true;
	isMoon = false;
	isSun = false;
	isScreen = true;

	//Get pointer to the Earth:
	PlanetP Earth = GETSTELMODULE(SolarSystem)->getEarth();
	myEarth = Earth.data();

	// Get pointer to the Moon/Sun:
	PlanetP Moon = GETSTELMODULE(SolarSystem)->getMoon();
	myMoon = Moon.data();

	// I think this can be done in a more simple way...--BM
	for (int i=0;i<366;i++) {
		sunRA[i] = 0.0; sunDec[i] = 0.0;
		objectRA[i] = 0.0; objectDec[i]=0.0;
		sunSidT[0][i]=0.0; sunSidT[1][i]=0.0;
		objectSidT[0][i]=0.0; objectSidT[1][i]=0.0;
		objectH0[i] = 0.0;
		yearJD[i] = 0.0;
	};

}

Observability::~Observability()
{
	// Shouldn't this be in the deinit()? --BM
	if (configDialog != NULL)
		delete configDialog;
}

void Observability::updateMessageText()
{
	// Set names of the months:
	monthNames.clear();
	monthNames << qc_("Jan", "short month name")
	           << qc_("Feb", "short month name")
	           << qc_("Mar", "short month name")
	           << qc_("Apr", "short month name")
	           << qc_("May", "short month name")
	           << qc_("Jun", "short month name")
	           << qc_("Jul", "short month name")
	           << qc_("Aug", "short month name")
	           << qc_("Sep", "short month name")
	           << qc_("Oct", "short month name")
	           << qc_("Nov", "short month name")
	           << qc_("Dec", "short month name");

	// TRANSLATORS: Short for "hours".
	msgH		= q_("h");
	// TRANSLATORS: Short for "minutes".
	msgM		= q_("m");
	// TRANSLATORS: Short for "seconds".
	msgS		= q_("s");
	msgSetsAt	= q_("Sets at %1 (in %2)");
	msgRoseAt	= q_("Rose at %1 (%2 ago)");
	msgSetAt	= q_("Set at %1 (%2 ago)");
	msgRisesAt	= q_("Rises at %1 (in %2)");
	msgCircumpolar	= q_("Circumpolar.");
	msgNoRise	= q_("No rise.");
	msgCulminatesAt	= q_("Culminates at %1 (in %2) at %3 deg.");
	msgCulminatedAt	= q_("Culminated at %1 (%2 ago) at %3 deg.");
	msgSrcNotObs	= q_("Source is not observable.");
	msgNoACRise	= q_("No acronychal nor cosmical rise/set.");
	msgGreatElong	= q_("Greatest elongation: %1 (at %2 deg.)");
	msgLargSSep	= q_("Largest Sun separation: %1 (at %2 deg.)");
	msgNone		= q_("None");
	// TRANSLATORS: The space at the end is significant - another sentence may follow.
	msgAcroRise	= q_("Acronychal rise/set: %1/%2. ");
	// TRANSLATORS: The space at the end is significant - another sentence may follow.
	msgNoAcroRise	= q_("No acronychal rise/set. ");
	msgCosmRise	= q_("Cosmical rise/set: %1/%2.");
	msgNoCosmRise	= q_("No cosmical rise/set.");
	msgWholeYear	= q_("Observable during the whole year.");
	msgNotObs	= q_("Not observable at dark night.");
	msgAboveHoriz	= q_("Nights above horizon: %1");
	msgToday	= q_("TODAY:");
	msgThisYear	= q_("THIS YEAR:");
	// TRANSLATORS: The space at the end is significant - another sentence may follow.
	msgPrevFullMoon	= q_("Previous Full Moon: %1 %2 at %3:%4. ");
	msgNextFullMoon	= q_("Next Full Moon: %1 %2 at %3:%4. ");
}

double Observability::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void Observability::init()
{
	loadConfiguration();
	
	StelAction* actionShow = addAction("actionShow_Observability",
	                                   N_("Observability"),
	                                   N_("Observability"),
	                                   "flagShowReport");
	// actionShow->setChecked(flagShowReport); //Unnecessary?
	addAction("actionShow_Observability_ConfigDialog",
	          N_("Observability"),
	          N_("Observability configuration window"),
	          configDialog, "visible");

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	button = new StelButton(NULL,
	                        QPixmap(":/observability/bt_observab_on.png"),
	                        QPixmap(":/observability/bt_observab_off.png"),
	                        QPixmap(":/graphicGui/glow32x32.png"),
	                        actionShow);
	gui->getButtonBar()->addButton(button, "065-pluginsGroup");
	
	updateMessageText();
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(updateMessageText()));
}

/////////////////////////////////////////////
// MAIN CODE:
void Observability::draw(StelCore* core)
{
	if (!flagShowReport)
		return; // Button is off.

/////////////////////////////////////////////////////////////////
// PRELIMINARS:
	bool locChanged, yearChanged;
	StelObjectP selectedObject;
	PlanetP ssObject, parentPlanet;

// Only execute plugin if we are on Earth.
	if (core->getCurrentLocation().planetName != "Earth")
		return;

// Set the painter:
	StelPainter painter(core->getProjection2d());
	painter.setColor(fontColor[0],fontColor[1],fontColor[2],1);
	font.setPixelSize(fontSize);
	painter.setFont(font);

// Get current date, location, and check if there is something selected.
	double currlat = (core->getCurrentLocation().latitude)/Rad2Deg;
	double currlon = (core->getCurrentLocation().longitude)/Rad2Deg;
	double currheight = (6371.+(core->getCurrentLocation().altitude)/1000.)/UA;
	double currJD = core->getJDay();
	double currJDint;
//	GMTShift = StelUtils::getGMTShiftFromQT(currJD)/24.0;
	GMTShift = StelApp::getInstance().getLocaleMgr().getGMTShift(currJD)/24.0;

//	qDebug() << QString("%1%2 ").arg(GMTShift);

	double currLocalT = 24.*modf(currJD + GMTShift,&currJDint);

	int auxm, auxd, auxy;
	StelUtils::getDateFromJulianDay(currJD, &auxy, &auxm, &auxd);
	bool isSource = StelApp::getInstance().getStelObjectMgr().getWasSelected();
	bool show_Year = show_Best_Night || show_Good_Nights || show_AcroCos; 

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE CHECK THE CHANGED PARAMETERS W.R.T. THE PREVIOUS FRAME:

// Update JD.
	myJD = currJD;

// If the year changed, we must recompute the Sun's position for each new day:
	if (auxy != curYear)
	{
		yearChanged = true;
		curYear = auxy;
		updateSunData(core);
	}
	else
	{
		yearChanged = false;
	};

// Have we changed the latitude or longitude?
	if (currlat == mylat && currlon == mylon)
	{
		locChanged = false;
	}
	else
	{
		locChanged = true;
		mylat = currlat; mylon = currlon;
		double temp1 = currheight*std::cos(currlat);
		ObserverLoc[0] = temp1*std::cos(currlon);
		ObserverLoc[1] = temp1*std::sin(currlon);
		ObserverLoc[2] = currheight*std::sin(currlat);
	};



// Add refraction, if necessary:
	Vec3d TempRefr;	
	TempRefr[0] = std::cos(horizonAltitude);  
	TempRefr[1] = 0.0; 
	TempRefr[2] = std::sin(horizonAltitude);  
	Vec3d CorrRefr = core->altAzToEquinoxEqu(TempRefr,StelCore::RefractionAuto);
	TempRefr = core->equinoxEquToAltAz(CorrRefr,StelCore::RefractionOff);
	double RefracAlt = std::asin(TempRefr[2]);

	// If the diference is larger than 1 arcminute...
	if (std::abs(refractedHorizonAlt-RefracAlt)>2.91e-4)
	{
		//... configuration for refraction changed notably.
		refractedHorizonAlt = RefracAlt;
		configChanged = true;
		souChanged = true;
	};





// If we have changed latitude (or year), we update the vector of Sun's hour 
// angles at twilight, and re-compute Sun/Moon ephemeris (if selected):
	if (locChanged || yearChanged || configChanged) 
	{
		updateSunH();
		lastJDMoon = 0.0;
	};

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE DEAL WITH THE SOURCE (OR SCREEN-CENTER) POSITION:

	if (isScreen) // Always re-compute everything for the screen center.
		souChanged = true; 

	if (isSource) // There is something selected!
	{ 

// Get the selected source and its name:
		selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]; 

// Don't do anything for satellites:
		if(selectedObject->getType() == "Satellite")
			return;

		QString name = selectedObject->getEnglishName();
		isMoon = ("Moon" == name);
		isSun = ("Sun" == name);

// If Moon is not selected (or was unselected), force re-compute of Full Moon next time it is selected:
		if (!isMoon)
		{
			prevFullMoon = 0.0;
			nextFullMoon = 0.0;
		};

//Update position:
		EquPos = selectedObject->getEquinoxEquatorialPos(core);
		EquPos.normalize();
		LocPos = core->equinoxEquToAltAz(EquPos, StelCore::RefractionOff);

// Check if the user has changed the source (or if the source is Sun/Moon). 
		if (name == selName) 
		{
			souChanged = false;
		}
		else 
		{ // Check also if the (new) source belongs to the Solar System:

			souChanged = true;
			selName = name;

			Planet* planet = dynamic_cast<Planet*>(selectedObject.data());
			isStar = (planet == NULL);

			if (!isStar && !isMoon && !isSun)  // Object in the Solar System, but is not Sun nor Moon.
			{ 

				int gene = -1;

			// If object is a planet's moon, we get its parent planet:
				ssObject = GETSTELMODULE(SolarSystem)->searchByEnglishName(selName);
				// TODO: Isn't it easier just to use the planet object we just cast? --BM

				parentPlanet = ssObject->getParent();
				if (parentPlanet) 
				{
					while (parentPlanet)
					{
						gene += 1;
						parentPlanet = parentPlanet->getParent();
					}
				}
				for (int g=0; g<gene; g++)
				{
					ssObject = ssObject->getParent();
				}
				
			// Now get a pointer to the planet's instance:
				myPlanet = ssObject.data();
			}
		}
	}
	else // There is no source selected!
	{
	// If no source is selected, get the position vector of the screen center:
		selName.clear();
		isStar = true;
		isMoon = false;
		isSun = false;
		isScreen = true;
		Vec3d currentPos = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		currentPos.normalize();
		EquPos = core->j2000ToEquinoxEqu(currentPos);
		LocPos = core->j2000ToAltAz(currentPos, StelCore::RefractionOff);
	}


// Convert EquPos to RA/Dec:
	toRADec(EquPos, selRA, selDec);

// Compute source's altitude (in radians):
	alti = std::asin(LocPos[2]);

// Force re-computation of ephemeris if the location changes or the user changes the configuration:
	if (locChanged || configChanged)
	{ 
		souChanged=true;
		configChanged=false;
	};

/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////
// NOW WE COMPUTE RISE/SET/TRANSIT TIMES FOR THE CURRENT DAY:
	double currH = calculateHourAngle(mylat,alti,selDec);
	horizH = calculateHourAngle(mylat,refractedHorizonAlt,selDec);
	QString RS1, RS2, Cul; // strings with Rise/Set/Culmination times
	double risingTime = 0, settingTime = 0; // Actual Rise/Set times (in GMT).
	int d1,m1,s1,d2,m2,s2,dc,mc,sc; // Integers for the time spans in hh:mm:ss.
	bool solvedMoon = false; // Check if solutions were found for Sun, Moon, or planet.
	bool transit = false; // Is the source above the horizon? Did it culminate?

	int ephHour, ephMinute, ephSecond;  // Local time for selected ephemeris

	if (show_Today)
	{
		// Today's ephemeris (rise, set, and transit times)
		if (!isStar) 
		{
			int type = (isSun) ? 1:0;
			type += (isMoon) ? 2:0;
			type += (!isSun && !isMoon) ? 3:0;
			
			// Returns false if the calculation fails...
			solvedMoon = calculateSolarSystemEvents(core, type);
			currH = std::abs(24.*(MoonCulm-myJD)/TFrac);
			transit = MoonCulm-myJD<0.0;
			if (solvedMoon)
			{  // If failed, Set and Rise will be dummy.
				settingTime = std::abs(24.*(MoonSet-myJD)/TFrac);
				risingTime = std::abs(24.*(MoonRise-myJD)/TFrac);
			}
		}
		else if (horizH>0.0)
		{ // The source is not circumpolar and can be seen from this latitude.
			
			if ( LocPos[1]>0.0 ) // The source is at the eastern side...
			{
				if ( currH>horizH ) // ... and below the horizon.
				{
					settingTime = 24.-currH-horizH;
					risingTime = currH-horizH;
					hasRisen = false;
				}
				else  // ... and above the horizon.
				{
					risingTime = horizH-currH;
					settingTime = 2.*horizH-risingTime;
					hasRisen = true;
				}
			}
			else // The source is at the western side...
			{
				if ( currH>horizH ) // ... and below the horizon. 
				{
					settingTime = currH-horizH;
					risingTime = 24.-currH-horizH;
					hasRisen = false;
				}
				else // ... and above the horizon.
				{
					risingTime = horizH+currH;
					settingTime = horizH-currH;
					hasRisen = true;
				}
			}
		}
		
		if ((solvedMoon && MoonRise>0.0) || (!isSun && !isMoon && horizH>0.0))
		{
			double2hms(TFrac*settingTime, d1, m1, s1);
			double2hms(TFrac*risingTime, d2, m2, s2);
			
			//		Strings with time spans for rise/set/transit:
			RS1 = (d1==0)?"":QString("%1%2 ").arg(d1).arg(msgH);
			RS1 += (m1==0)?"":QString("%1%2 ").arg(m1).arg(msgM);
			RS1 += QString("%1%2").arg(s1).arg(msgS);
			RS2 = (d2==0)?"":QString("%1%2 ").arg(d2).arg(msgH);
			RS2 += (m2==0)?"":QString("%1%2 ").arg(m2).arg(msgM);
			RS2 += QString("%1%2").arg(s2).arg(msgS);
			if (hasRisen) 
			{
				double2hms(toUnsignedRA(currLocalT+TFrac*settingTime+12.),
				           ephHour, ephMinute, ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QChar('0')); // Local time for set.

				double2hms(toUnsignedRA(currLocalT-TFrac*risingTime+12.),
				           ephHour, ephMinute, ephSecond); // Local time for rise.
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				
				//RS1 = q_("Sets at %1 (in %2)").arg(SetTime).arg(RS1);
				//RS2 = q_("Rose at %1 (%2 ago)").arg(RiseTime).arg(RS2);
				RS1 = msgSetsAt.arg(SetTime).arg(RS1);
				RS2 = msgRoseAt.arg(RiseTime).arg(RS2);
			}
			else 
			{
				double2hms(toUnsignedRA(currLocalT-TFrac*settingTime+12.),
				           ephHour, ephMinute, ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				
				double2hms(toUnsignedRA(currLocalT+TFrac*risingTime+12.),
				           ephHour, ephMinute, ephSecond);
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				
				//RS1 = q_("Set at %1 (%2 ago)").arg(SetTime).arg(RS1);
				//RS2 = q_("Rises at %1 (in %2)").arg(RiseTime).arg(RS2);
				RS1 = msgSetAt.arg(SetTime).arg(RS1);
				RS2 = msgRisesAt.arg(RiseTime).arg(RS2);
			}
		}
		else // The source is either circumpolar or never rises:
		{
			(alti>refractedHorizonAlt)? RS1 = msgCircumpolar: RS1 = msgNoRise;
			RS2 = "";
		};
		
		// 	Culmination:
		
		if (isStar)
		{
			culmAlt = std::abs(mylat-selDec); // 90.-altitude at transit.
			transit = LocPos[1]<0.0;
		};
		
		if (culmAlt < (halfpi - refractedHorizonAlt)) // Source can be observed.
		{
			double altiAtCulmi = Rad2Deg*(halfpi-culmAlt-refractedHorizonAlt);
			double2hms(TFrac*currH,dc,mc,sc);
			
			//String with the time span for culmination:
			Cul = (dc==0)?"":QString("%1%2 ").arg(dc).arg(msgH);
			Cul += (mc==0)?"":QString("%1%2 ").arg(mc).arg(msgM);
			Cul += QString("%1%2").arg(sc).arg(msgS);
			if (!transit)
			{ 
				double2hms(toUnsignedRA(currLocalT + TFrac*currH + 12.),
				           ephHour, ephMinute, ephSecond); // Local time at transit.
				CulmTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				//Cul = q_("Culminates at %1 (in %2) at %3 deg.").arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
				Cul = msgCulminatesAt.arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
			}
			else
			{
				double2hms(toUnsignedRA(currLocalT - TFrac*currH + 12.),
				           ephHour, ephMinute, ephSecond);
				CulmTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				//Cul = q_("Culminated at %1 (%2 ago) at %3 deg.").arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
				Cul = msgCulminatedAt.arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
			}
		}
	} // This comes from show_Today==True
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// NOW WE ANALYZE THE SOURCE OBSERVABILITY FOR THE WHOLE YEAR:

// Compute yearly ephemeris (only if necessary, and not for Sun nor Moon):


	if (isSun) 
	{
		lineBestNight.clear();
		lineObservableRange.clear();
	}
	else if (!isMoon && show_Year)
	{

		if (!isStar && (souChanged || yearChanged)) // Object moves.
			updatePlanetData(core); // Re-compute ephemeris.
		else
		{ // Object is fixed on the sky.
			double auxH = calculateHourAngle(mylat,refractedHorizonAlt,selDec);
			double auxSidT1 = toUnsignedRA(selRA - auxH); 
			double auxSidT2 = toUnsignedRA(selRA + auxH); 
			for (int i=0;i<nDays;i++) {
				objectH0[i] = auxH;
				objectRA[i] = selRA;
				objectDec[i] = selDec;
				objectSidT[0][i] = auxSidT1;
				objectSidT[1][i] = auxSidT2;
			};
		};

// Determine source observability (only if something changed):
		if ((souChanged || locChanged || yearChanged))
		{
			lineBestNight.clear();
			lineObservableRange.clear();

			// Check if the target cannot be seen.
			if (culmAlt >= (halfpi - refractedHorizonAlt))
			{
				//ObsRange = q_("Source is not observable.");
				//AcroCos = q_("No Acronychal nor Cosmical rise/set.");
				lineObservableRange = msgSrcNotObs;
				lineAcroCos = msgNoACRise;
			}
			else
			{ // Source can be seen.

///////////////////////////
// - Part 1. Determine the best observing night (i.e., opposition to the Sun):
				if (show_Best_Night)
				{
					int selday = 0;
					double deltaPhs = -1.0; // Initial dummy value
					double tempPhs; 	
					for (int i=0; i<nDays; i++) // Maximize the Sun-object separation.
					{
						tempPhs = Lambda(objectRA[i], objectDec[i],
						                 sunRA[i], sunDec[i]);
						if (tempPhs > deltaPhs)
						{
							selday = i;
							deltaPhs = tempPhs;
						}
					}

					if (selName=="Mercury" || selName=="Venus")
					{
						lineBestNight = msgGreatElong;
					}
					else 
					{
						lineBestNight = msgLargSSep;
					}
					
					lineBestNight = lineBestNight
					                .arg(formatAsDate(selday))
					                .arg(deltaPhs*Rad2Deg, 0, 'f', 1);
				}

///////////////////////////////
// - Part 2. Determine Acronychal and Cosmical rise and set:

				if (show_AcroCos)
				{
					int acroRise, acroSet, cosRise, cosSet;
					int result = calculateAcroCos(acroRise, acroSet,
					                              cosRise, cosSet);
					QString acroRiseStr, acroSetStr;
					QString cosRiseStr, cosSetStr;
					// TODO: Possible error? Day 0 is 1 Jan.
					acroRiseStr = (acroRise>0)?formatAsDate(acroRise):msgNone;
					acroSetStr = (acroSet>0)?formatAsDate(acroSet):msgNone;
					cosRiseStr = (cosRise>0)?formatAsDate(cosRise):msgNone;
					cosSetStr = (cosSet>0)?formatAsDate(cosSet):msgNone;

					if (result==3 || result==1)
						lineAcroCos =  msgAcroRise
						               .arg(acroRiseStr)
						               .arg(acroSetStr);
					else
						lineAcroCos =  msgNoAcroRise;
					
					if (result==3 || result==2)
						lineAcroCos += msgCosmRise
						               .arg(cosRiseStr)
						               .arg(cosSetStr);
					else
						lineAcroCos += msgNoCosmRise;
				}


////////////////////////////
// - Part 3. Determine range of good nights 
// (i.e., above horizon before/after twilight):

				if (show_Good_Nights)
				{
					int selday = 0;
					int selday2 = 0;
					bool bestBegun = false; // Are we inside a good time range?
					bool atLeastOne = false;
					QString dateRange;
					bool poleNight, twiGood;

					for (int i=0; i<nDays; i++)
					{

						poleNight = sunSidT[0][i]<0.0 && std::abs(sunDec[i]-mylat)>=halfpi; // Is it night during 24h?
						twiGood = (poleNight && std::abs(objectDec[i]-mylat)<halfpi)?true:CheckRise(i);
						
						if (twiGood && bestBegun == false)
						{
							selday = i;
							bestBegun = true;
							atLeastOne = true;
						};

						if (!twiGood && bestBegun == true)
						{
							selday2 = i;
							bestBegun = false;
							if (selday2 > selday)
							{
								// FIXME: This kind of concatenation is bad for i18n.
								if (!dateRange.isEmpty())
									dateRange += ", ";
								dateRange += QString("%1").arg(formatAsDateRange(selday, selday2));
							};
						};
					};

					// Check if there were good dates till the end of the year.
					if (bestBegun)
					{
						// FIXME: This kind of concatenation is bad for i18n.
						 if (!dateRange.isEmpty())
							 dateRange += ", ";
						dateRange += formatAsDateRange(selday, 0);
					};
					
					if (dateRange.isEmpty()) 
					{ 
						if (atLeastOne) 
						{
							//ObsRange = q_("Observable during the whole year.");
							lineObservableRange = msgWholeYear;
						}
						else
						{
							//ObsRange = q_("Not observable at dark night.");
							lineObservableRange = msgNotObs;
						};
					}
					else
					{
						// Nights when the target is above the horizon
						lineObservableRange = msgAboveHoriz.arg(dateRange);
					};

				}; // Comes from show_Good_Nights==True"
			}; // Comes from the "else" of "culmAlt>=..." 
		};// Comes from  "souChanged || ..."
	}; // Comes from the "else" with "!isMoon"

// Print all results:

	int lineSpacing = (int) (1.3* ( (double) fontSize));  // between lines
	int groupSpacing = 6*fontSize;  // between daily and yearly results
	int yLine = 8*fontSize+110;
	int xLine = 80;

	if (show_Today) 
	{
		//renderer->drawText(TextParams(xLine, yLine,q_("TODAY:")));
		painter.drawText(xLine, yLine, msgToday);
		painter.drawText(xLine + fontSize, yLine - lineSpacing, RS2);
		painter.drawText(xLine + fontSize, yLine - lineSpacing*2, RS1);
		painter.drawText(xLine + fontSize, yLine - lineSpacing*3, Cul);
		yLine -= groupSpacing;
	}
	
	if ((isMoon && show_FullMoon) || (!isSun && !isMoon && show_Year)) 
	{
		painter.drawText(xLine, yLine, msgThisYear);
		if (show_Best_Night || show_FullMoon)
		{
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineBestNight);
		}
		if (show_Good_Nights)
		{
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineObservableRange);
		}
		if (show_AcroCos)
		{
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineAcroCos);
		}
	}
}

// END OF MAIN CODE
///////////////////////////////////////////////////////


//////////////////////////////
// AUXILIARY FUNCTIONS

////////////////////////////////////
// Returns the hour angle for a given altitude:
double Observability::calculateHourAngle(double latitude,
                                         double elevation,
                                         double declination)
{
	double denom = std::cos(latitude)*std::cos(declination);
	double numer = (std::sin(elevation)-std::sin(latitude)*std::sin(declination));

	if ( std::abs(numer) > std::abs(denom) ) 
	{
		return -0.5/86400.; // Source doesn't reach that altitude.
	}
	else
	{
		return Rad2Hr * std::acos(numer/denom);
	}
}
////////////////////////////////////


////////////////////////////////////
// Returns the angular separation between two points on the Sky:
// RA is given in hours and Dec in radians.
double Observability::Lambda(double RA1, double Dec1, double RA2, double Dec2)
{
	return std::acos(std::sin(Dec1)*std::sin(Dec2)+std::cos(Dec1)*std::cos(Dec2)*std::cos((RA1-RA2)/Rad2Hr));
}
////////////////////////////////////


////////////////////////////////////
// Returns the hour angle for a given a Sid. Time:
double Observability::HourAngle2(double RA, double ST)
{
	double result = toUnsignedRA(RA-ST/15.);
	result -= (result > 12.) ? 24.0 : 0.0;
	return result;

}
////////////////////////////////////


////////////////////////////////////
// Converts a float time/angle span (in hours/degrees) in the (integer) format hh/dd,mm,ss:
void Observability::double2hms(double hfloat, int &h1, int &h2, int &h3)
{
	double f1,f2,f3;
	hfloat = std::abs(hfloat);
	double ffrac = std::modf(hfloat,&f1);
	double ffrac2 = std::modf(60.*ffrac,&f2);
	ffrac2 = std::modf(3600.*(ffrac-f2/60.),&f3);
	h1 = (int)f1 ; h2 = (int)std::abs(f2+0.0*ffrac2) ; h3 = (int)std::abs(f3);
} 
////////////////////////////////////


////////////////////////////////////
// Adds/subtracts 24hr to ensure a RA between 0 and 24hr:
double Observability::toUnsignedRA(double RA)
{
	double tempRA,tempmod;
	//FIXME: tempmod is unused variable; need fix
	if (RA<0.0) {tempmod = std::modf(-RA/24.,&tempRA); RA += 24.*(tempRA+1.0)+0.0*tempmod;};
	double auxRA = 24.*std::modf(RA/24.,&tempRA);
	auxRA += (auxRA<0.0)?24.0:((auxRA>24.0)?-24.0:0.0);
	return auxRA;
}
////////////////////////////////////


QString Observability::formatAsDate(int dayNumber)
{
	int day, month, year;
	StelUtils::getDateFromJulianDay(yearJD[dayNumber], &year, &month, &day);

	QString formatString = (getDateFormat()) ? "%1 %2" : "%2 %1";
	QString result = formatString.arg(day).arg(monthNames[month-1]);
	return result;
}

///////////////////////////////////////////////
// Returns the day and month of year (to put it in format '25 Apr')
QString Observability::formatAsDateRange(int startDay, int endDay)
{
	int sDay, sMonth, sYear, eDay, eMonth, eYear;
	QString range;
	StelUtils::getDateFromJulianDay(yearJD[startDay], &sYear, &sMonth, &sDay);
	StelUtils::getDateFromJulianDay(yearJD[endDay], &eYear, &eMonth, &eDay);
	if (endDay == 0)
	{
		eDay = 31;
		eMonth = 12;
	}
	if (startDay == 0)
	{
		sDay = 1;
		sMonth = 1;
	}
	
	// If it's the same month, display "X-Y Month" or "Month X-Y"
	if (sMonth == eMonth)
	{
		QString formatString = (getDateFormat()) ? "%1 - %2 %3" : "%3 %1 - %2";
		range = formatString.arg(sDay).arg(eDay).arg(monthNames[sMonth-1]);;
	}
	else
	{
		QString formatString = (getDateFormat()) ? "%1 %2 - %3 %4" 
		                                         : "%2 %1 - %4 %3";
		range = formatString.arg(sDay)
		                    .arg(monthNames[sMonth-1])
		                    .arg(eDay)
		                    .arg(monthNames[eMonth-1]);
	}

	return range;
}
//////////////////////////////////////////////

// Compute planet's position for each day of the current year:
void Observability::updatePlanetData(StelCore *core)
{
	double tempH;
	for (int i=0; i<nDays; i++)
	{
		getPlanetCoords(core, yearJD[i], objectRA[i], objectDec[i], false);
		tempH = calculateHourAngle(mylat, refractedHorizonAlt, objectDec[i]);
		objectH0[i] = tempH;
		objectSidT[0][i] = toUnsignedRA(objectRA[i]-tempH);
		objectSidT[1][i] = toUnsignedRA(objectRA[i]+tempH);
	}

// Return the planet to its current time:
	getPlanetCoords(core, myJD, objectRA[0], objectDec[0], true);
}

/////////////////////////////////////////////////
// Computes the Sun's RA and Dec (and the JD) for 
// each day of the current year.
void Observability::updateSunData(StelCore* core) 
{
	int day, month, year, sameYear;
// Get current date:
	StelUtils::getDateFromJulianDay(myJD,&year,&month,&day);

// Get JD for the Jan 1 of current year:
	StelUtils::getJDFromDate(&Jan1stJD,year,1,1,0,0,0);

// Check if we are on a leap year:
	StelUtils::getDateFromJulianDay(Jan1stJD+365., &sameYear, &month, &day);
	nDays = (year==sameYear)?366:365;
	
// Compute Earth's position throughout the year:
	Vec3d pos, sunPos;
	for (int i=0; i<nDays; i++)
	{
		yearJD[i] = Jan1stJD + (double)i;
		myEarth->computePosition(yearJD[i]);
		myEarth->computeTransMatrix(yearJD[i]);
		pos = myEarth->getHeliocentricEclipticPos();
		sunPos = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-pos));
		EarthPos[i] = -pos;
		toRADec(sunPos,sunRA[i],sunDec[i]);
	};

//Return the Earth to its current time:
	myEarth->computePosition(myJD);
	myEarth->computeTransMatrix(myJD);
}
///////////////////////////////////////////////////


////////////////////////////////////////////
// Computes Sun's Sidereal Times at twilight and culmination:
void Observability::updateSunH()
{
	double tempH, tempH00;

	for (int i=0; i<nDays; i++)
	{
		tempH = calculateHourAngle(mylat, twilightAltRad, sunDec[i]);
		tempH00 = calculateHourAngle(mylat, refractedHorizonAlt, sunDec[i]);
		if (tempH > 0.0)
		{
			sunSidT[0][i] = toUnsignedRA(sunRA[i]-tempH*(1.00278));
			sunSidT[1][i] = toUnsignedRA(sunRA[i]+tempH*(1.00278));
		}
		else
		{
			sunSidT[0][i] = -1000.0;
			sunSidT[1][i] = -1000.0;
		}
		
		if (tempH00>0.0)
		{
			sunSidT[2][i] = toUnsignedRA(sunRA[i]+tempH00);
			sunSidT[3][i] = toUnsignedRA(sunRA[i]-tempH00);
		}
		else
		{
			sunSidT[2][i] = -1000.0;
			sunSidT[3][i] = -1000.0;
		}
	}
}
////////////////////////////////////////////


///////////////////////////////////////////
// Checks if a source can be observed with the Sun below the twilight altitude.
bool Observability::CheckRise(int day)
{

	// If Sun can't reach twilight elevation, the target is not visible.
	if (sunSidT[0][day]<0.0 || sunSidT[1][day]<0.0)
		return false;

	// Iterate over the whole year:
	int nBin = 1000;
	double auxSid1 = sunSidT[0][day];
	auxSid1 += (sunSidT[0][day] < sunSidT[1][day]) ? 24.0 : 0.0;
	double deltaT = (auxSid1-sunSidT[1][day]) / ((double)nBin);

	double hour; 
	for (int j=0; j<nBin; j++)
	{
		hour = toUnsignedRA(sunSidT[1][day]+deltaT*(double)j - objectRA[day]);
		hour -= (hour>12.) ? 24.0 : 0.0;
		if (std::abs(hour)<objectH0[day] || (objectH0[day] < 0.0 && alti>0.0))
			return true;
	}

	return false;
}
///////////////////////////////////////////


///////////////////////////////////////////
// Finds the dates of Acronichal (Rise, Set) and Cosmical (Rise2, Set2) dates.
int Observability::calculateAcroCos(int &acroRise, int &acroSet,
                                    int &cosRise, int &cosSet)
{
	acroRise = -1;
	acroSet = -1;
	cosRise = -1;
	cosSet = -1;

	double bestDiffAcroRise = 12.0;
	double bestDiffAcroSet = 12.0;
	double bestDiffCosRise = 12.0;
	double bestDiffCosSet = 12.0;

	double hourDiffAcroRise, hourDiffAcroSet, hourDiffCosRise, hourCosDiffSet;
	bool success = false;

	for (int i=0; i<366; i++)
	{
		if (objectH0[i]>0.0 && sunSidT[2][i]>0.0 && sunSidT[3][i]>0.0)
		{
			success = true;
			hourDiffAcroRise = toUnsignedRA(objectRA[i] - objectH0[i]);
			hourDiffCosRise = hourDiffAcroRise-sunSidT[3][i];
			hourDiffAcroRise -= sunSidT[2][i];
			
			hourDiffAcroSet = toUnsignedRA(objectRA[i] + objectH0[i]);
			hourCosDiffSet = hourDiffAcroSet - sunSidT[2][i];
			hourDiffAcroSet -= sunSidT[3][i];
			
			// Acronychal rise/set:
			if (std::abs(hourDiffAcroRise) < bestDiffAcroRise) 
			{
				bestDiffAcroRise = std::abs(hourDiffAcroRise);
				acroRise = i;
			};
			if (std::abs(hourDiffAcroSet) < bestDiffAcroSet) 
			{
				bestDiffAcroSet = std::abs(hourDiffAcroSet);
				acroSet = i;
			};
			
			// Cosmical Rise/Set:
			if (std::abs(hourDiffCosRise) < bestDiffCosRise) 
			{
				bestDiffCosRise = std::abs(hourDiffCosRise);
				cosRise = i;
			};
			if (std::abs(hourCosDiffSet) < bestDiffCosSet) 
			{
				bestDiffCosSet = std::abs(hourCosDiffSet);
				cosSet = i;
			};
		};
	};

	acroRise *= (bestDiffAcroRise > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	acroSet *= (bestDiffAcroSet > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	cosRise *= (bestDiffCosRise > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	cosSet *= (bestDiffCosSet > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	int result = (acroRise>0 || acroSet>0) ? 1 : 0;
	result += (cosRise>0 || cosSet>0) ? 2 : 0;
	return (success) ? result : 0;
}
///////////////////////////////////////////


////////////////////////////////////////////
// Convert an Equatorial Vec3d into RA and Dec:
void Observability::toRADec(Vec3d vec3d, double& ra, double &dec)
{
	vec3d.normalize();
	dec = std::asin(vec3d[2]); // in radians
	ra = toUnsignedRA(std::atan2(vec3d[1],vec3d[0])*Rad2Hr); // in hours.
}
////////////////////////////////////////////



///////////////////////////
// Just return the sign of a double
double Observability::sign(double d)
{
	return (d<0.0)?-1.0:1.0;
}
//////////////////////////



//////////////////////////
// Get the coordinates of Sun or Moon for a given JD:
// getBack controls whether Earth and Moon must be returned to their original positions after computation.
void Observability::getSunMoonCoords(StelCore *core, double jd,
                                     double &raSun, double &decSun,
                                     double &raMoon, double &decMoon,
                                     double &eclLon, bool getBack) 
                                     //, Vec3d &AltAzVector)
{

	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(jd);
		myEarth->computeTransMatrix(jd);
		myMoon->computePosition(jd);
		myMoon->computeTransMatrix(jd);
	} 
	else // Compute coordinates:
	{
		myEarth->computePosition(jd);
		myEarth->computeTransMatrix(jd);
		Vec3d earthPos = myEarth->getHeliocentricEclipticPos();
		double curSidT;

// Sun coordinates:
		Vec3d sunPos = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-earthPos));
		toRADec(sunPos, raSun, decSun);

// Moon coordinates:
		curSidT = myEarth->getSiderealTime(jd)/Rad2Deg;
		RotObserver = (Mat4d::zrotation(curSidT))*ObserverLoc;
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
		myMoon->computePosition(jd);
		myMoon->computeTransMatrix(jd);
		Vec3d moonPos = myMoon->getHeliocentricEclipticPos();
		sunPos = (core->j2000ToEquinoxEqu(LocTrans*moonPos))-RotObserver;
		
		eclLon = moonPos[0]*earthPos[1] - moonPos[1]*earthPos[0];

		toRADec(sunPos,raMoon,decMoon);
	};
}
//////////////////////////////////////////////



//////////////////////////
// Get the Observer-to-Moon distance JD:
// getBack controls whether Earth and Moon must be returned to their original positions after computation.
void Observability::getMoonDistance(StelCore *core, double jd, double &distance, bool getBack)
{

	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(jd);
		myEarth->computeTransMatrix(jd);
		myMoon->computePosition(jd);
		myMoon->computeTransMatrix(jd);
	} 
	else
	{	// Compute coordinates:
		myEarth->computePosition(jd);
		myEarth->computeTransMatrix(jd);
		Vec3d earthPos = myEarth->getHeliocentricEclipticPos();
//		double curSidT;

// Sun coordinates:
//		Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos0));
//		toRADec(Pos2,RASun,DecSun);

// Moon coordinates:
//		curSidT = myEarth->getSiderealTime(JD)/Rad2Deg;
//		RotObserver = (Mat4d::zrotation(curSidT))*ObserverLoc;
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
		myMoon->computePosition(jd);
		myMoon->computeTransMatrix(jd);
		Pos1 = myMoon->getHeliocentricEclipticPos();
		Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1)); //-RotObserver;

		distance = std::sqrt(Pos2*Pos2);

//		toRADec(Pos2,RAMoon,DecMoon);
	};
}
//////////////////////////////////////////////




//////////////////////////////////////////////
// Get the Coords of a planet:
void Observability::getPlanetCoords(StelCore *core, double JD, double &RA, double &Dec, bool getBack)
{

	if (getBack)
	{
	// Return the planet to its current time:
		myPlanet->computePosition(JD);
		myPlanet->computeTransMatrix(JD);
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
	} else
	{
	// Compute planet's position:
		myPlanet->computePosition(JD);
		myPlanet->computeTransMatrix(JD);
		Pos1 = myPlanet->getHeliocentricEclipticPos();
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
		Pos2 = myEarth->getHeliocentricEclipticPos();
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos2));
		Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1);
		toRADec(Pos2,RA,Dec);
	};

}
//////////////////////////////////////////////



//////////////////////////////////////////////
// Solves Moon's, Sun's, or Planet's ephemeris by bissection.
bool Observability::calculateSolarSystemEvents(StelCore* core, int bodyType)
{

	const int NUM_ITER = 100;
	int i;
	double hHoriz, ra, dec, raSun, decSun, tempH, tempJd, tempEphH, curSidT, eclLon;
	//Vec3d Observer;

	hHoriz = calculateHourAngle(mylat, refractedHorizonAlt, selDec);
	bool raises = hHoriz > 0.0;


// Only recompute ephemeris from second to second (at least)
// or if the source has changed (i.e., Sun <-> Moon). This saves resources:
	if (std::abs(myJD-lastJDMoon)>JDsec || lastType!=bodyType || souChanged)
	{

//		qDebug() << q_("%1  %2   %3   %4").arg(Kind).arg(LastObject).arg(myJD,0,'f',5).arg(lastJDMoon,0,'f',5);

		lastType = bodyType;

		myEarth->computePosition(myJD);
		myEarth->computeTransMatrix(myJD);
		Vec3d earthPos = myEarth->getHeliocentricEclipticPos();

		if (bodyType == 1) // Sun position
		{
			Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-earthPos));
		}
		else if (bodyType==2) // Moon position
		{
			curSidT = myEarth->getSiderealTime(myJD)/Rad2Deg;
			RotObserver = (Mat4d::zrotation(curSidT))*ObserverLoc;
			LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
			myMoon->computePosition(myJD);
			myMoon->computeTransMatrix(myJD);
			Pos1 = myMoon->getHeliocentricEclipticPos();
			Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1))-RotObserver;
		}
		else // Planet position
		{
			myPlanet->computePosition(myJD);
			myPlanet->computeTransMatrix(myJD);
			Pos1 = myPlanet->getHeliocentricEclipticPos();
			LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
			Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1);
		};

		toRADec(Pos2,ra,dec);
		Vec3d moonAltAz = core->equinoxEquToAltAz(Pos2, StelCore::RefractionOff);
		hasRisen = moonAltAz[2] > refractedHorizonAlt;

// Initial guesses of rise/set/transit times.
// They are called 'Moon', but are also used for the Sun or planet:

		double Hcurr = -calculateHourAngle(mylat,alti,selDec)*sign(LocPos[1]);
		double SidT = toUnsignedRA(selRA + Hcurr);

		MoonCulm = -Hcurr; 
		MoonRise = (-hHoriz-Hcurr);
		MoonSet = (hHoriz-Hcurr);

		if (raises)
		{
			if (!hasRisen)
			{
				MoonRise += (MoonRise<0.0)?24.0:0.0;
				MoonSet -= (MoonSet>0.0)?24.0:0.0;
			}

// Rise time:
			tempEphH = MoonRise*TFrac;
			MoonRise = myJD + (MoonRise/24.);
			for (i=0; i<NUM_ITER; i++)
			{
	// Get modified coordinates:
				tempJd = MoonRise;
	
				if (bodyType<3)
				{
					getSunMoonCoords(core, tempJd,
					                 raSun, decSun,
					                 ra, dec,
					                 eclLon, false);
				} else
				{
					getPlanetCoords(core, tempJd, ra, dec, false);
				};

				if (bodyType==1) {ra = raSun; dec = decSun;};

	// Current hour angle at mod. coordinates:
				Hcurr = toUnsignedRA(SidT-ra);
				Hcurr -= (hasRisen)?0.0:24.;
				Hcurr -= (Hcurr>12.)?24.0:0.0;

	// H at horizon for mod. coordinates:
				hHoriz = calculateHourAngle(mylat,refractedHorizonAlt,dec);
	// Compute eph. times for mod. coordinates:
				tempH = (-hHoriz-Hcurr)*TFrac;
				if (hasRisen==false) tempH += (tempH<0.0)?24.0:0.0;
			// Check convergence:
				if (std::abs(tempH-tempEphH)<JDsec) break;
			// Update rise-time estimate:
				tempEphH = tempH;
				MoonRise = myJD + (tempEphH/24.);
			};

// Set time:  
			tempEphH = MoonSet;
			MoonSet = myJD + (MoonSet/24.);
			for (i=0; i<NUM_ITER; i++)
			{
	// Get modified coordinates:
				tempJd = MoonSet;
				
				if (bodyType < 3)
					getSunMoonCoords(core, tempJd,
					                 raSun, decSun,
					                 ra, dec,
					                 eclLon, false);
				else
					getPlanetCoords(core, tempJd, ra, dec, false);
				
				if (bodyType==1) {ra = raSun; dec = decSun;};
				
	// Current hour angle at mod. coordinates:
				Hcurr = toUnsignedRA(SidT-ra);
				Hcurr -= (hasRisen)?24.:0.;
				Hcurr += (Hcurr<-12.)?24.0:0.0;
	// H at horizon for mod. coordinates:
				hHoriz = calculateHourAngle(mylat, refractedHorizonAlt, dec);
	// Compute eph. times for mod. coordinates:
				tempH = (hHoriz-Hcurr)*TFrac;
				if (!hasRisen)
					tempH -= (tempH>0.0)?24.0:0.0;
		// Check convergence:
				if (std::abs(tempH-tempEphH)<JDsec)
					break;
		// Update set-time estimate:
				tempEphH = tempH;
				MoonSet = myJD + (tempEphH/24.);
			};
		} 
		else // Comes from if(raises)
		{
			MoonSet = -1.0;
			MoonRise = -1.0;
		};

// Culmination time:
		tempEphH = MoonCulm;
		MoonCulm = myJD + (MoonCulm/24.);

		for (i=0; i<NUM_ITER; i++)
		{
			// Get modified coordinates:
			tempJd = MoonCulm;

			if (bodyType<3)
			{
				getSunMoonCoords(core,tempJd,raSun,decSun,ra,dec,eclLon,false);
			} else
			{
				getPlanetCoords(core,tempJd,ra,dec,false);
			};


			if (bodyType==1) {ra = raSun; dec = decSun;};


	// Current hour angle at mod. coordinates:
			Hcurr = toUnsignedRA(SidT-ra);
			Hcurr += (LocPos[1]<0.0)?24.0:-24.0;
			Hcurr -= (Hcurr>12.)?24.0:0.0;

	// Compute eph. times for mod. coordinates:
			tempH = -Hcurr*TFrac;
	// Check convergence:
			if (std::abs(tempH-tempEphH)<JDsec) break;
			tempEphH = tempH;
			MoonCulm = myJD + (tempEphH/24.);
			culmAlt = std::abs(mylat-dec); // 90 - altitude at transit. 
		};

//		qDebug() << q_("%1").arg(MoonCulm,0,'f',5);


	lastJDMoon = myJD;

	}; // Comes from if (std::abs(myJD-lastJDMoon)>JDsec || LastObject!=Kind)




// Find out the days of Full Moon:
	if (bodyType==2 && show_FullMoon) // || show_SuperMoon))
	{

	// Only estimate date of Full Moon if we have changed Lunar month:
		if (myJD > nextFullMoon || myJD < prevFullMoon)
		{


	// Estimate the nearest (in time) Full Moon:
			double nT;
			double dT = std::modf((myJD-RefFullMoon)/MoonT,&nT);
			if (dT>0.5) {nT += 1.0;};
			if (dT<-0.5) {nT -= 1.0;};

			double TempFullMoon = RefFullMoon + nT*MoonT;

	// Improve the estimate iteratively (Secant method over Lunar-phase vs. time):

			dT = 0.1/1440.; // 6 seconds. Our time span for the finite-difference derivative estimate.
//			double Deriv1, Deriv2; // Variables for temporal use.
			double Sec1, Sec2, Temp1, Temp2; // Variables for temporal use.
			double iniEst1, iniEst2;  // JD values that MUST include the solution within them.
			double Phase1;

			for (int j=0; j<2; j++) 
			{ // Two steps: one for the previos Full Moon and the other for the next one.

				iniEst1 =  TempFullMoon - 0.25*MoonT; 
				iniEst2 =  TempFullMoon + 0.25*MoonT; 

				Sec1 = iniEst1; // TempFullMoon - 0.05*MoonT; // Initial estimates of Full-Moon dates
				Sec2 = iniEst2; // TempFullMoon + 0.05*MoonT; 

				getSunMoonCoords(core,Sec1,raSun,decSun,ra,dec,eclLon,false);
				Temp1 = eclLon; //Lambda(RA,Dec,RAS,DecS);
				getSunMoonCoords(core,Sec2,raSun,decSun,ra,dec,eclLon,false);
				Temp2 = eclLon; //Lambda(RA,Dec,RAS,DecS);


				for (int i=0; i<100; i++) // A limit of 100 iterations.
				{
					Phase1 = (Sec2-Sec1)/(Temp1-Temp2)*Temp1+Sec1;
					getSunMoonCoords(core,Phase1,raSun,decSun,ra,dec,eclLon,false);
					
					if (Temp1*eclLon < 0.0) 
					{
						Sec2 = Phase1;
						Temp2 = eclLon;
					} else {
						Sec1 = Phase1;
						Temp1 = eclLon;

					};

				//	qDebug() << QString("%1 %2 %3 %4 ").arg(Sec1).arg(Sec2).arg(Temp1).arg(Temp2);	


					if (std::abs(Sec2-Sec1) < 10.*dT)  // 1 minute accuracy; convergence.
					{
						TempFullMoon = (Sec1+Sec2)/2.;
				//		qDebug() << QString("%1%2 ").arg(TempFullMoon);	
						break;
					};
					
				};


				if (TempFullMoon > myJD) 
				{
					nextFullMoon = TempFullMoon;
					TempFullMoon -= MoonT;
				} else
				{
					prevFullMoon = TempFullMoon;
					TempFullMoon += MoonT;
				};

			};


	// Update the string shown in the screen: 
			int fullDay, fullMonth,fullYear, fullHour, fullMinute, fullSecond;
			double LocalPrev = prevFullMoon+GMTShift+0.5;  // Shift to the local time. 
			double LocalNext = nextFullMoon+GMTShift+0.5;
			double intMoon;
			double LocalTMoon = 24.*modf(LocalPrev,&intMoon);
			StelUtils::getDateFromJulianDay(intMoon, &fullYear, &fullMonth, &fullDay);
			double2hms(toUnsignedRA(LocalTMoon),fullHour,fullMinute,fullSecond);
			if (getDateFormat())
				lineBestNight = msgPrevFullMoon.arg(fullDay).arg(monthNames[fullMonth-1]).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));
			else
				lineBestNight = msgPrevFullMoon.arg(monthNames[fullMonth-1]).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));

			LocalTMoon = 24.*modf(LocalNext,&intMoon);
			StelUtils::getDateFromJulianDay(intMoon,&fullYear,&fullMonth,&fullDay);
			double2hms(toUnsignedRA(LocalTMoon),fullHour,fullMinute,fullSecond);			
			if (getDateFormat())
				lineBestNight += msgNextFullMoon.arg(fullDay).arg(monthNames[fullMonth-1]).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));
			else
				lineBestNight += msgNextFullMoon.arg(monthNames[fullMonth-1]).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));

			lineObservableRange.clear(); 
			lineAcroCos.clear();


	// Now, compute the days of all the Full Moons of the current year, and get the Earth/Moon distance:
//			double monthFrac, monthTemp, maxMoonDate;
//			monthFrac = std::modf((nextFullMoon-Jan1stJD)/MoonT,&monthTemp);
//			int PrevMonths = (int)(monthTemp+0.0*monthFrac); 
//			double BestDistance = 1.0; // initial dummy value for Sun-Moon distance;
//			double Distance; // temporal variable to save Earth-Moon distance at each month.

//			qDebug() << q_("%1 ").arg(PrevMonths);

//			for (int i=-PrevMonths; i<13 ; i++)
//			{
//				jd1 = nextFullMoon + MoonT*((double) i);
//				getMoonDistance(core,jd1,Distance,false); 
//				if (Distance < BestDistance)
//				{  // Month with the largest Full Moon:
//					BestDistance = Distance;
//					maxMoonDate = jd1;
//				};
//			};
//			maxMoonDate += GMTShift+0.5;
//			StelUtils::getDateFromJulianDay(maxMoonDate,&fullYear,&fullMonth,&fullDay);
//			double MoonSize = MoonPerilune/BestDistance*100.;
//			ObsRange = q_("Greatest Full Moon: %1 "+months[fullMonth-1]+" (%2% of Moon at Perilune)").arg(fullDay).arg(MoonSize,0,'f',2);
		};
	} 
	else if (bodyType <3)
	{
		lineBestNight.clear();
		lineObservableRange.clear(); 
		lineAcroCos.clear();
	}; 


// Return the Moon and Earth to its current position:
	if (bodyType<3)
	{
		getSunMoonCoords(core, myJD, raSun, decSun, ra, dec, eclLon, true);
	}
	else
	{
		getPlanetCoords(core, myJD, ra, dec, true);
	};


	return raises;
}




//////////////////////////////////
///  STUFF FOR THE GUI CONFIG

bool Observability::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Observability::resetConfiguration()
{
	// Remove the plug-in's group from the configuration,
	// after that it will be loaded with default values.
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->remove("Observability");
	loadConfiguration();
}

void Observability::loadConfiguration()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	conf->beginGroup("Observability");

	// Load settings from main config file
	fontSize = conf->value("font_size",15).toInt();
	font.setPixelSize(fontSize);
	fontColor = StelUtils::strToVec3f(conf->value("font_color", "0,0.5,1").toString());
	show_AcroCos = conf->value("show_AcroCos", true).toBool();
	show_Good_Nights = conf->value("show_Good_Nights", true).toBool();
	show_Best_Night = conf->value("show_Best_Night", true).toBool();
	show_Today = conf->value("show_Today", true).toBool();
	show_FullMoon = conf->value("show_FullMoon", true).toBool();
//	show_Crescent = conf->value("show_Crescent", true).toBool();
//	show_SuperMoon = conf->value("show_SuperMoon", true).toBool();

	// For backwards compatibility, the value of this key is stored with
	// inverted sign.
	// TODO: Skip the sign inversion when the key is changed.
	int altitude = -(conf->value("Sun_Altitude", 12).toInt());
	setTwilightAltitude(altitude); 

	altitude = conf->value("Horizon_Altitude", 0).toInt();
	setHorizonAltitude(altitude);
	
	conf->endGroup();
	
	// Load date format from main settings.
	// TODO: Handle date format properly.
	if (conf->value("localization/date_display_format", "system_default").toString() == "ddmmyyyy")
		setDateFormat(true);
	else
		setDateFormat(false);
}

void Observability::saveConfiguration()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QString fontColorStr = QString("%1,%2,%3").arg(fontColor[0],0,'f',2).arg(fontColor[1],0,'f',2).arg(fontColor[2],0,'f',2);
	// Set updated values
	conf->beginGroup("Observability");
	conf->setValue("font_size", fontSize);
	// For backwards compatibility, the value of this key is stored with
	// inverted sign.
	// TODO: Skip the sign inversion when the key is changed.
	conf->setValue("Sun_Altitude", -twilightAltDeg);
	conf->setValue("Horizon_Altitude", horizonAltDeg);
	conf->setValue("font_color", fontColorStr);
	conf->setValue("show_AcroCos", show_AcroCos);
	conf->setValue("show_Good_Nights", show_Good_Nights);
	conf->setValue("show_Best_Night", show_Best_Night);
	conf->setValue("show_Today", show_Today);
	conf->setValue("show_FullMoon", show_FullMoon);
//	conf->setValue("show_Crescent", show_Crescent);
//	conf->setValue("show_SuperMoon", show_SuperMoon);
	conf->endGroup();
}

void Observability::enableTodayField(bool enabled)
{
	show_Today = enabled;
	configChanged = true;
}

void Observability::enableAcroCosField(bool enabled)
{
	show_AcroCos = enabled;
	configChanged = true;
}

void Observability::enableGoodNightsField(bool enabled)
{
	show_Good_Nights = enabled;
	configChanged = true;
}

void Observability::enableOppositionField(bool enabled)
{
	show_Best_Night = enabled;
	configChanged = true;
}

void Observability::enableFullMoonField(bool enabled)
{
	show_FullMoon = enabled;
	configChanged = true;
}

bool Observability::getShowFlags(int iFlag)
{
	switch (iFlag)
	{
		case 1: return show_Today;
		case 2: return show_AcroCos;
		case 3: return show_Good_Nights;
		case 4: return show_Best_Night;
		case 5: return show_FullMoon;
//		case 6: return show_Crescent;
//		case 7: return show_SuperMoon;
	};

	return false;
}

Vec3f Observability::getFontColor(void)
{
	return fontColor;
}

int Observability::getFontSize(void)
{
	return fontSize;
}

int Observability::getTwilightAltitude()
{
	return twilightAltDeg;
}

int Observability::getHorizonAltitude()
{
	return horizonAltDeg;
}


void Observability::setFontColor(const Vec3f& color)
{
	fontColor = color; // Vector3::operator =() is overloaded. :)
}

void Observability::setFontSize(int size)
{
	fontSize = size;
}

void Observability::setTwilightAltitude(int altitude)
{
	twilightAltRad  = ((double) altitude)/Rad2Deg ;
	twilightAltDeg = altitude;
	configChanged = true;
}

void Observability::setHorizonAltitude(int altitude)
{
	horizonAltitude = ((double) altitude)/Rad2Deg ;
	horizonAltDeg = altitude;
	configChanged = true;
}


void Observability::showReport(bool b)
{
	flagShowReport = b;
}

