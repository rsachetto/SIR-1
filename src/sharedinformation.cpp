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

#include "sharedinformation.h"

/** Default constructor.\n
  * Sets default field values.
  */
SharedInformation::SharedInformation() {
    // synchronization threads settings
    overwriteAll = false;
    noOverwriteAll = false;
    overwriteResult = 1;
    abort = false;
    enlargeAll = false;
    noEnlargeAll = false;
    enlargeResult = 1;
    // image settings
    width = 0;
    height = 0;
    hasWidth = false;
    hasHeight = false;
    maintainAspect = true;
    sizeBytes = 0;
    sizeUnit = 0;
    quality = 100;
    rotate = false;
    angle = 0.;
    // metadata settings
    saveMetadata = true;
    realRotate = false;
    updateThumbnail = true;
    rotateThumbnail = false;
}
