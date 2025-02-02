/*
 * Stellarium
 * Copyright (C) 2010 Bogdan Marinov
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
 
#ifndef _COMET_HPP_
#define _COMET_HPP_

#include "Planet.hpp"

/*! \class Comet
	\author Bogdan Marinov, Georg Zotti (orbit computation enhancements, tails)

	Some of the code in this class is re-used from the parent Planet class.
	\todo Implement better comet rendering (star-like objects, no physical body).
	\todo (Long-term) Photo realistic comet rendering, see https://blueprints.launchpad.net/stellarium/+spec/realistic-comet-rendering
	2013-12: New algorithms for position computation following Paul Heafner: Fundamental Ephemeris Computations (Willmann-Bell 1999).
	2014-01: Parabolic tails appropriately scaled/rotated. Much is currently empirical, leaving room for physics-based improvements.
  */
class Comet : public Planet
{
public:
	Comet(const QString& englishName,
	       int flagLighting,
	       double radius,
	       double oblateness,
	       Vec3f color,
	       float albedo,
	       const QString& texMapName,
	       posFuncType _coordFunc,
	       void* userDataPtr,
	       OsculatingFunctType *osculatingFunc,
	       bool closeOrbit,
	       bool hidden,
		   const QString &pType,
		   float dustTailWidthFact=1.5f,
		   float dustTailLengthFact=0.4f,
		   float dustTailBrightnessFact=1.5f
		  );

	virtual ~Comet();

	//Inherited from StelObject via Planet
	//! Get a string with data about the Comet.
	//! Comets support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - PlainText
	//  - Size <- Size of what?
	//! \param core the StelCore object
	//! \param flags a set of InfoStringGroup items to include in the return value.
	//! \return a QString containing an HMTL encoded description of the Comet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup &flags) const;
	//The Comet class inherits the "Planet" type because the SolarSystem class
	//was not designed to handle different types of objects.
	//virtual QString getType() const {return "Comet";}
	//! \todo Find better sources for the g,k system
	virtual float getVMagnitude(const StelCore* core) const;

	//! \brief sets absolute magnitude and slope parameter.
	//! These are the parameters in the IAU's two-parameter magnitude system
	//! for comets. They are used to calculate the apparent magnitude at
	//! different distances from the Sun. They are not used in the same way
	//! as the same parameters in MinorPlanet.
	void setAbsoluteMagnitudeAndSlope(const double magnitude, const double slope);

	//! set value for semi-major axis in AU
	void setSemiMajorAxis(const double value);

	//! get sidereal period for comet, days, or returns 0 if not possible (paraboloid, hyperboloid orbit)
	virtual double getSiderealPeriod() const;

	//! re-implementation of Planet's draw()
	virtual void draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont);

private:
	//! @returns estimates for (Coma diameter [AU], gas tail length [AU]).
	//! Using the formula from Guide found by the GSoC2012 initiative at http://www.projectpluto.com/update7b.htm#comet_tail_formula
	Vec2f getComaDiameterAndTailLengthAU() const;
	void drawTail(StelCore* core, StelProjector::ModelViewTranformP transfo, bool gas);
	void drawComa(StelCore* core, StelProjector::ModelViewTranformP transfo);

	//! compute a coma, faked as simple disk to be tilted towards the observer.
	//! @param diameter Diameter of Coma [AU]
	void computeComa(const float diameter);

	//! compute tail shape. This is a paraboloid shell with triangular mesh (indexed vertices).
	//! Try to call not for every frame...
	//! To be more efficient, the arrays are only computed if they are empty.
	//! @param parameter the parameter p of the parabola. z=r²/2p (r²=x²+y²)
	//! @param lengthfactor The parabola will be lengthened. This shifts the visible focus, so it must be here.
	//! @param vertexArr vertex array, collects x0, y0, z0, x1, y1, z1, ...
	//! @param texCoordArr texture coordinates u0, v0, u1, v1, ...
	//! @param colorArr vertex colors (if not textured) r0, g0, b0, r1, g1, b1, ...
	//! @param indices into the former arrays (zero-starting), triplets forming triangles: t0,0, t0,1, t0,2, t1,0, t1,1, t1,2, ...
	//! @param xOffset for the dust tail, this may introduce a bend. Units are x per sqrt(z).
	void computeParabola(const float parameter, const float topradius, const float zshift, QVector<double>& vertexArr, QVector<float>& texCoordArr, QVector<unsigned short>& indices, const float xOffset=0.0f);

	double absoluteMagnitude;
	double slopeParameter;
	double semiMajorAxis;
	bool isCometFragment;
	bool nameIsProvisionalDesignation;

	//GZ Tail additions
	float dustTailWidthFactor;      //!< empirical individual broadening of the dust tail end, compared to the gas tail end. Actually, dust tail width=2*comaWidth*dustTailWidthFactor. Default 1.5
	float dustTailLengthFactor;     //!< empirical individual length of dust tail relative to gas tail. Taken from ssystem.ini, typical value 0.3..0.5, default 0.4
	float dustTailBrightnessFactor; //!< empirical individual brightness of dust tail relative to gas tail. Taken from ssystem.ini, default 1.5
	QVector<double> gastailVertexArr;  // computed frequently, describes parabolic shape (along z axis) of gas tail.
	QVector<double> dusttailVertexArr; // computed frequently, describes parabolic shape (along z axis) of dust tail.
	QVector<float> gastailTexCoordArr; // computed only once per comet!
	//QVector<float> dusttailTexCoordArr; // currently identical to gastailVertexArr, has been taken out.
	QVector<unsigned short> gastailIndices; // computed only once per comet!
	//QVector<unsigned short> dusttailIndices; // actually no longer required. Re-use gas tail indices.
	QVector<double> comaVertexArr;
	QVector<float> comaTexCoordArr;
	StelTextureSP comaTexture;
	StelTextureSP gasTailTexture;
	//StelTextureSP dusttailTexture;  // it seems not really necessary to have different textures.
};

#endif //_COMET_HPP_
