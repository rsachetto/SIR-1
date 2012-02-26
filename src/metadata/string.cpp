/*
 * This file is part of SIR, an open-source cross-platform Image tool
 * 2007-2010  Rafael Sachetto
 * 2011-2012  Marek Jędryka
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact e-mail: Rafael Sachetto <rsachetto@gmail.com>
 *                 Marek Jędryka   <jedryka89@gmail.com>
 * Program URL: http://sir.projet-libre.org/
 *
 */

#include "string.h"
#include <QObject>
#include <QStringList>
#include <cmath>

/** Default constructor.\n
  * Constructs empty string.
  */
MetadataUtils::String::String() : QString() {}

/** Constructs string from \b str. */
MetadataUtils::String::String(const char *str) : QString(str) {}

/** Constructs string from \b other. */
MetadataUtils::String::String(const QString &other) : QString(other) {}

/** Assigns \b str to this string and returns a reference to this string. */
MetadataUtils::String & MetadataUtils::String::operator =(const char *str) {
    QString::operator =(str);
    return *this;
}

/** Assigns \b other to this string and returns a reference to this string. */
MetadataUtils::String & MetadataUtils::String::operator =(const QString& other) {
    QString::operator =(other);
    return *this;
}

/** Appends \b unit to this string if the string is a number;
  * otherwise the string will be assigns noData() string.
  * \sa noData
  */
void MetadataUtils::String::appendUnit(const char *unit) {
    if ( (this->toDouble() == -1.) || this->isEmpty() )
        *this = noData();
    else
        this->append(unit);
}

/** Returns copy of this string converted to local 8 bit code on Windows,
  * otherwise to will be converted to UTF-8.
  */
std::string MetadataUtils::String::toNativeStdString() const {
#ifdef Q_OS_WIN32
    return toLocal8Bit().constData();
#else
    return toUtf8().constData();
#endif
}

/** Converts this string to improper fraction and returns this fraction. */
Exiv2::Rational MetadataUtils::String::toRational() const {
    Exiv2::Rational result;
    if (this == noData()) {
        result.first = -1;
        result.second = 1;
        return result;
    }
    QStringList list = this->split(' ');
    QString last = list.last();
    if (last == "s")
        list.removeLast();
    else if (last.right(1) == "s")
        last.remove(last.lastIndexOf("s"),1);
    last = list.last();
    if (last.contains('/')) {
        QStringList fractionList = last.split('/');
        result.first = fractionList.first().toInt();
        result.second = fractionList.last().toInt();
        if (list.count() == 2)
            result.first += list.first().toInt() * result.second;
    }
    else {
        result.first = last.toInt();
        result.second = 1;
    }
    return result;
}

/** Converts this string to improper fraction of exponent and returns this fraction. */
Exiv2::Rational MetadataUtils::String::toRationalPower() const {
    Exiv2::Rational rational = toRational();
    Exiv2::Rational result;
    const quint16 multiplier = 10000;
    result.first = multiplier * log((double)rational.first / rational.second);
    result.second = multiplier * log(0.5);
    return result;
}

/** Returns string converted from \b s. */
MetadataUtils::String MetadataUtils::String::fromStdString(const std::string &s) {
    return MetadataUtils::String( QString::fromStdString(s) );
}

/** Returns translated \em no \em data string. */
QString MetadataUtils::String::noData() {
    return QObject::tr("no data");
}
