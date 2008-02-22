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

#ifndef _S_FONT_H
#define _S_FONT_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string>
#include <QString>
#include <QChar>

#include "StelUtils.hpp"
#include "typeface.h"

using namespace std;

/*
 * Wrapper class on the TypeFace class used to display fonts in openGL
 */
class SFont
{
public:
	SFont(float size_i, const QString& ttfFileName);
	~SFont() {;}
    
	void print(float x, float y, const QString& s, int upsidedown = 1) const
	{
		typeFace.render(s, Vec2f(x, y), upsidedown==1);
	}
    
	void print_char(const QChar c) const
	{
		typeFace.renderGlyphs(c);
	}

	//! @deprecated - use QString / QChar
	void print_char(const wchar_t c) const
	{
		print_char(QChar(c));
	}

	void print_char_outlined(const QChar c) const;

	void print_char_outlined(const wchar_t c) const {
		print_char_outlined(QChar(c));
	}
	
	//! @deprecated - use QString / QChar
	void print_char_outlined(const unsigned char c) const
	{
		print_char_outlined(QChar(c));
	}

	//! @deprecated - use QString / QChar
	void print_char(const unsigned char c) const
	{
		print_char(QChar(c));
	}

	float getStrLen(const QString& s) const {return typeFace.width(s);}

	//! @deprecated - use QString / QChar
	float getStrLen(const wstring& s) const {return typeFace.width(QString::fromStdWString(s));}

	//! @deprecated - use QString / QChar
	float getStrLen(const string& s)  const {return typeFace.width(QString::fromStdString(s));}

	float getLineHeight(void) const {return typeFace.lineHeight();}
	float getAscent(void) const {return typeFace.ascent();}
	float getDescent(void) const {return typeFace.descent();}
	float getSize(void) const {return typeFace.pointSize();}
	
private:
	mutable TypeFace typeFace;
};

#endif  //_S_FONT_H
