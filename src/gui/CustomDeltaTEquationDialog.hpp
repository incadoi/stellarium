/*
 * Stellarium
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

// AW: Methods copied largely from AddRemoveLandscapesDialog

#ifndef _CUSTOMDELTATEQUATIONDIALOG_HPP_
#define _CUSTOMDELTATEQUATIONDIALOG_HPP_

#include "StelDialog.hpp"
#include "StelCore.hpp"

#include <QObject>
#include <QSettings>

class Ui_CustomDeltaTEquationDialogForm;

//! @class CustomDeltaTEquationDialog
class CustomDeltaTEquationDialog : public StelDialog
{
	Q_OBJECT

public:
	CustomDeltaTEquationDialog();
	virtual ~CustomDeltaTEquationDialog();

public slots:
	void retranslate();
	void setVisible(bool);

protected:
	//! Initialize the dialog widgets and connect the signals/slots.
	virtual void createDialogContent();
	Ui_CustomDeltaTEquationDialogForm *ui;

private slots:
	void saveSettings(void) const;

	void setNDot(const QString& v);
	void setYear(const QString& v);
	void setCoeffA(const QString& v);
	void setCoeffB(const QString& v);
	void setCoeffC(const QString& v);

private:
	QSettings* conf;
	StelCore* core;

	float year;
	float ndot;
	Vec3f coeff;

	void setDescription(void) const;

};

#endif // _CUSTOMDELTATEQUATIONDIALOG_HPP_
