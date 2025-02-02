/*
 * Angle Measure plug-in for Stellarium
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

#include "config.h"

#include "AngleMeasure.hpp"
#include "AngleMeasureDialog.hpp"
#include "ui_angleMeasureDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

AngleMeasureDialog::AngleMeasureDialog()
	: am(NULL)
{
	ui = new Ui_angleMeasureDialog();
}

AngleMeasureDialog::~AngleMeasureDialog()
{
	delete ui;
}

void AngleMeasureDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void AngleMeasureDialog::createDialogContent()
{
	am = GETSTELMODULE(AngleMeasure);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	ui->useDmsFormatCheckBox->setChecked(am->isDmsFormat());
	connect(ui->useDmsFormatCheckBox, SIGNAL(toggled(bool)), am, SLOT(useDmsFormat(bool)));
	ui->showPositionAngleCheckBox->setChecked(am->isPaDisplayed());
	connect(ui->showPositionAngleCheckBox, SIGNAL(toggled(bool)), am, SLOT(showPositionAngle(bool)));

	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveAngleMeasureSettings()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(resetAngleMeasureSettings()));

	setAboutHtml();
}

void AngleMeasureDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Angle Measure Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + ANGLEMEASURE_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Matthew Gates</td></tr>";
	html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td>Bogdan Marinov<br />Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Angle Measure plugin is a small tool which is used to measure the angular distance between two points on the sky (and calculation of position angle between those two points).") + "</p>";
	html += "<p>" + q_("*goes misty eyed* I recall measuring the size of the Cassini Division when I was a student. It was not the high academic glamor one might expect... It was cloudy... It was rainy... The observatory lab had some old scopes set up at one end, pointing at a <em>photograph</em> of Saturn at the other end of the lab. We measured. We calculated. We wished we were in Hawaii.") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Angle Measure plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/AngleMeasure_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);

	ui->aboutTextBrowser->setHtml(html);
}

void AngleMeasureDialog::saveAngleMeasureSettings()
{
	am->saveSettings();
}

void AngleMeasureDialog::resetAngleMeasureSettings()
{
	am->restoreDefaultSettings();
}
