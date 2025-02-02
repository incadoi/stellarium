/*
 * Stellarium
 * Copyright (C) 2009, 2012 Matthew Gates
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

#ifndef _SATELLITE_HPP_
#define _SATELLITE_HPP_ 1

#include <QDateTime>
#include <QFont>
#include <QList>
#include <QOpenGLFunctions_1_2>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelSphereGeometry.hpp"

#include "StelPainter.hpp"
#include "gSatWrapper.hpp"


class StelPainter;
class StelLocation;

//! Radio communication channel properties.
typedef struct
{
	double frequency; //!< Channel frequency in MHz.
	QString modulation; //!< Signal modulation mode.
	QString description; //!< Channel description.
} CommLink;

//! Description of the data roles used in SatellitesListModel.
enum SatelliteDataRole {
	SatIdRole = Qt::UserRole,
	SatDescriptionRole,
	SatFlagsRole,
	SatGroupsRole,
	FirstLineRole,
	SecondLineRole
};

//! Type for sets of satellite group IDs.
typedef QSet<QString> GroupSet;

//! Flag type reflecting internal flags of Satellite.
enum SatFlag
{
	SatNoFlags = 0x0,
	SatDisplayed = 0x1,
	SatNotDisplayed = 0x2,
	SatUser = 0x4,
	SatOrbit = 0x8,
	SatNew = 0x10,
	SatError = 0x20
};
typedef QFlags<SatFlag> SatFlags;
Q_DECLARE_OPERATORS_FOR_FLAGS(SatFlags)

// Allows the type to be used by QVariant
Q_DECLARE_METATYPE(GroupSet)
Q_DECLARE_METATYPE(SatFlags)

//! @class Satellite
//! A representation of a satellite in Earth orbit.
//! Details about the satellite are passed with a JSON-representation structure
//! that contains a @ref satcat "satellite catalog" entry.
//! 
//! Thanks to operator<() overloading, container classes (QList, QMap, etc)
//! with Satellite or SatelliteP objects can be sorted by satellite name/ID.
class Satellite : public StelObject, protected QOpenGLFunctions
{
	friend class Satellites;
	friend class SatellitesDialog;
	friend class SatellitesListModel;
	
public:
	//! \param identifier unique identifier (currently the Catalog Number)
	//! \param data a QMap which contains the details of the satellite
	//! (TLE set, description etc.)
	Satellite(const QString& identifier, const QVariantMap& data);
	~Satellite();

	//! Get a QVariantMap which describes the satellite.  Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	virtual QString getType(void) const
	{
		return "Satellite";
	}
	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @param flags a set of flags with information types to include.
	//! Supported types for Satellite objects:
	//! - Name: designation in large type with the description underneath
	//! - RaDecJ2000, RaDecOfDate, HourAngle, AltAzi
	//! - Extra: range, rage rate and altitude of satellite above the Earth, comms frequencies, modulation types and so on.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const;
	virtual float getVMagnitude(const StelCore* core) const;
	virtual double getAngularSize(const StelCore* core) const;
	virtual QString getNameI18n(void) const
	{
		return name;
	}
	virtual QString getEnglishName(void) const
	{
		return name;
	}
	//! Returns the (NORAD) catalog number. (For now, the ID string.)
	QString getCatalogNumberString() const {return id;}

	//! Set new tleElements.  This assumes the designation is already set, populates
	//! the tleElements values and configures internal orbit parameters.
	void setNewTleElements(const QString& tle1, const QString& tle2);

	// calculate faders, new position
	void update(double deltaTime);

	double getDoppler(double freq) const;
	static float showLabels;
	static double roundToDp(float n, int dp);

	// when the observer location changes we need to
	void recalculateOrbitLines(void);
	
	void setNew() {newlyAdded = true;}
	bool isNew() const {return newlyAdded;}
	
	//! Get internal flags as a single value.
	SatFlags getFlags();
	//! Sets the internal flags in one operation (only display flags)!
	void setFlags(const SatFlags& flags);
	
	//! Parse TLE line to extract International Designator and launch year.
	//! Sets #internationalDesignator and #jdLaunchYearJan1.
	void parseInternationalDesignator(const QString& tle1);
	
	//! Needed for sorting lists (if this ever happens...).
	//! Compares #name fields. If equal, #id fields, which can't be.
	bool operator<(const Satellite& another) const;

	//! Calculation of illuminated fraction of the satellite.
	float calculateIlluminatedFraction() const;

private:
	//draw orbits methods
	void computeOrbitPoints();
	void drawOrbit(StelPainter& painter);
	//! returns 0 - 1.0 for the DRAWORBIT_FADE_NUMBER segments at
	//! each end of an orbit, with 1 in the middle.
	float calculateOrbitSegmentIntensity(int segNum);

private:
	bool initialized;
	//! Flag indicating whether the satellite should be displayed.
	//! Should not be confused with the pedicted visibility of the 
	//! actual satellite to the observer.
	bool displayed;
	//! Flag indicating whether an orbit section should be displayed.
	bool orbitDisplayed;  // draw orbit enabled/disabled
	//! Flag indicating that the satellite is user-defined.
	//! This means that its TLE set shouldn't be updated and the satellite
	//! itself shouldn't be removed if auto-remove is enabled.
	bool userDefined;
	//! Flag indicating that the satellite was added during the current session.
	bool newlyAdded;
	bool orbitValid;

	//! Identifier of the satellite, must be unique within the list.
	//! Currently, the Satellite Catalog Number/NORAD Number is used,
	//! as it is unique and it is contained in both lines of TLE sets.
	QString id;
	//! Human-readable name of the satellite.
	//! Usually the string in the "Title line" of TLE sets.
	QString name;
	//! Longer description of the satellite.
	QString description;
	//! International Designator / COSPAR designation / NSSDC ID.
	QString internationalDesignator;
	//! Julian date of Jan 1st of the launch year.
	//! Used to hide satellites before their launch date.
	//! Extracted from TLE set with parseInternationalDesignator().
	//! It defaults to 1 Jan 1957 if extraction fails.
	double jdLaunchYearJan1;
	//! Standard visual magnitude of the satellite.
	double stdMag;
	//! Contains the J2000 position.
	Vec3d XYZ;
	QPair< QByteArray, QByteArray > tleElements;
	double height, range, rangeRate;
	QList<CommLink> comms;
	Vec3f hintColor;
	//! Identifiers of the groups to which the satellite belongs.
	//! See @ref groups.
	GroupSet groups;
	QDateTime lastUpdated;

	static StelTextureSP hintTexture;
	static SphericalCap  viewportHalfspace;
	static float hintBrightness;
	static float hintScale;
	static int   orbitLineSegments;
	static int   orbitLineFadeSegments;
	static int   orbitLineSegmentDuration; //measured in seconds
	static bool  orbitLinesFlag;
	static bool  realisticModeFlag;
	//! Mask controlling which info display flags should be honored.
	static StelObject::InfoStringGroupFlags flagsMask;

	void draw(StelCore *core, StelPainter& painter, float maxMagHints);

	//Satellite Orbit Position calculation
	gSatWrapper *pSatWrapper;
	Vec3d	position;
	Vec3d	velocity;
	Vec3d	latLongSubPointPosition;
	Vec3d	elAzPosition;
	int	visibility;
	double	phaseAngle; // phase angle for the satellite

	//Satellite Orbit Draw
	QFont     font;
	Vec3f    orbitColor;
	double    lastEpochCompForOrbit; //measured in Julian Days
	double    epochTime;  //measured in Julian Days
	QList<Vec3d> orbitPoints; //orbit points represented by ElAzPos vectors
};

typedef QSharedPointer<Satellite> SatelliteP;
bool operator<(const SatelliteP& left, const SatelliteP& right);

#endif // _SATELLITE_HPP_ 

