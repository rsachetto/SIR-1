/* This file is part of SIR, an open-source cross-platform Image tool
 * 2007-2010  Rafael Sachetto <rsachetto@gmail.com>
 * 2011-2016  Marek Jędryka   <jedryka89@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Program URL: http://marek629.github.io/SIR/
 */

#include "raw/RawModel.hpp"

#include "Settings.hpp"
#include "raw/RawModelValidator.hpp"
#include "raw/RawToolbox.hpp"


RawModel::RawModel()
{
    this->enabled = false;
    this->tab = BasicTab;
}

RawModel::RawModel(bool enabled, const QString &dcrawPath,
                   const QString &dcrawOptions)
{
    this->enabled = enabled;
    this->dcrawPathString = dcrawPath;
    this->dcrawOptionsString = dcrawOptions;

    this->tab = BasicTab;
}

RawModel::RawModel(const RawModel &other)
{
    swap(other);
}

RawModel::~RawModel() {}

void RawModel::swap(const RawModel &other)
{
    this->enabled = other.enabled;
    this->dcrawPathString = other.dcrawPathString;
    this->dcrawOptionsString = other.dcrawOptionsString;
    this->tab = other.tab;
}

bool RawModel::isValid() const
{
    return RawModelValidator().isValidModel(*this);
}

void RawModel::load(const Settings &settings)
{
    enabled = settings.value("Raw/enabled", false).toBool();
    dcrawPathString = settings.value("Raw/dcrawPath", "/usr/bin/dcraw").toString();
    dcrawOptionsString = settings.value("Raw/dcrawOptions", "").toString();

    QString tabString = settings.value("Raw/tab", "basic").toString();
    if (tabString.toLower() == "advanced") {
        tab = AdvancedTab;
    } else {
        tab = BasicTab;
    }
}

void RawModel::save(Settings *settings)
{
    settings->beginGroup("Raw");

    settings->setValue("enabled", enabled);
    settings->setValue("dcrawPath", dcrawPathString);
    settings->setValue("dcrawOptions", dcrawOptionsString);

    QString tabString = (tab == AdvancedTab) ? "advanced" : "basic";
    settings->setValue("tab", tabString);

    settings->endGroup();
}

bool RawModel::isEnabled() const
{
    return enabled;
}

void RawModel::setEnabled(bool value)
{
    enabled = value;
}

QString RawModel::dcrawPath() const
{
    return dcrawPathString;
}

void RawModel::setDcrawPath(const QString &value)
{
    dcrawPathString = value;
}

QString RawModel::dcrawOptions() const
{
    return dcrawOptionsString;
}

void RawModel::setDcrawOptions(const QString &value)
{
    dcrawOptionsString = value;
}

RawModel::RawTab RawModel::rawTab() const
{
    return tab;
}

void RawModel::setRawTab(RawModel::RawTab value)
{
    tab = value;
}
