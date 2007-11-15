/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __MILKYWAY_H__
#define __MILKYWAY_H__

#include <string>
#include "StelModule.hpp"
#include "vecmath.h"
#include "STextureTypes.hpp"

class Navigator;
class ToneReproducer;
class LoadingBar;

//! @class MilkyWay 
//! Manages the displaying of the Milky Way.
class MilkyWay : public StelModule
{
	Q_OBJECT

public:
	MilkyWay();
	virtual ~MilkyWay();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the class.  Here we load the texture for the Milky Way and 
	//! get the display settings from the ini parser object, namely the flag which
	//! determines if the Milky Way is displayed or not, and the intensity setting.
	//! @param conf ini parser object which contains the milky way settings.
	//! @param lb the LoadingBar object used to display loading progress.
	virtual void init(const InitParser& conf, LoadingBar& lb);

	//! Draw the Milky Way.
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	
	//! Update and time-dependent state.  Updates the fade level while the 
	//! Milky way rendering is being changed from on to off or off to on.
	virtual void update(double deltaTime);
	
	//! Does nothing in the MilkyWay module.
	virtual void updateI18n() {;}
	
	//! Does nothing in the MilkyWay module.
	virtual void updateSkyCulture(LoadingBar& lb) {;}
	
	//! Used to determine the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const {return 1.;}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Get Milky Way intensity.
	float getIntensity() const {return intensity;}
	//! Set Milky Way intensity.
	void setIntensity(float aintensity) {intensity = aintensity;}
	
	//! Get the color used for rendering the milky way
	Vec3f getColor() const {return color;}
	//! Sets the color to use for rendering the milky way
	void setColor(const Vec3f& c) {color=c;}
	
	//! Sets whether to show the Milky Way
	void setFlagShow(bool b);
	//! Gets whether the Milky Way is displayed
	bool getFlagShow(void) const;
	
private:
	//! Set the texture to use for the Milky Way.
	//! @param texFile the path to a texture file to be loaded.
	void setTexture(const QString& texFile);
	
	float radius;
	STextureSP tex;
	Vec3f color;
	float intensity;
	float tex_avg_luminance;
	class LinearFader* fader;
};

#endif // __MILKYWAY_H__
