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

#include "raw/RawControllerTest.hpp"

#include "raw/AdvancedRawViewWidget.hpp"
#include "raw/RawController.hpp"
#include "raw/RawModel.hpp"
#include "widgets/MessageBox.hpp"

#include <QDir>


RawControllerTest::RawControllerTest()
{
    MessageBox::enableTesting(true);

    RawModel *model = new RawModel(true, "/usr/bin/dcraw", "");
    view = new AdvancedRawViewWidget();
    controller = new RawController(model, view);
}

RawControllerTest::~RawControllerTest()
{
    delete view;
    delete controller;
}

void RawControllerTest::initTestCase() {}

void RawControllerTest::cleanupTestCase()
{
    QString input("/test_dcraw.bin");
    input.prepend(QDir::tempPath());

    QFile file(input);

    if (file.exists()) {
        QVERIFY(file.remove());
    }
}

void RawControllerTest::test_setRawStatus_true()
{
    controller->setRawStatus(true);

    QVERIFY(view->isPathTextEnabledInput());
    QVERIFY(view->isButtonEnabledInput());
    QVERIFY(view->isOptionsTextEnabledInput());
}

void RawControllerTest::test_setRawStatus_false()
{
    controller->setRawStatus(false);

    QVERIFY(!view->isPathTextEnabledInput());
    QVERIFY(!view->isButtonEnabledInput());
    QVERIFY(!view->isOptionsTextEnabledInput());
}

void RawControllerTest::test_setRawStatus_notNull()
{
    controller->setRawStatus(10);

    QVERIFY(view->isPathTextEnabledInput());
    QVERIFY(view->isButtonEnabledInput());
    QVERIFY(view->isOptionsTextEnabledInput());
}

void RawControllerTest::test_checkDcrawPath_emptyString()
{
    QString input("");

    bool expected = false;
    bool result = controller->checkDcrawPath(input);

    QCOMPARE(result, expected);
}

void RawControllerTest::test_checkDcrawPath_fileNotExists()
{
    QString input("/test_dcraw.bin");
    input.prepend(QDir::tempPath());

    QFile file(input);
    QVERIFY(!file.exists());

    bool expected = false;
    bool result = controller->checkDcrawPath(input);

    QCOMPARE(result, expected);
}

void RawControllerTest::test_checkDcrawPath_fileNotExecutable()
{
    QString input("/test_dcraw.bin");
    input.prepend(QDir::tempPath());

    QFile file(input);
    QVERIFY(!file.exists());
    file.open(QFile::ReadWrite);
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    QVERIFY(file.exists());

    bool expected = false;
    bool result = controller->checkDcrawPath(input);

    QCOMPARE(result, expected);

    QVERIFY(file.remove());
}

void RawControllerTest::test_checkDcrawPath_fileExecutable()
{
    QString input("/test_dcraw.bin");
    input.prepend(QDir::tempPath());

    QFile file(input);
    QVERIFY(!file.exists());
    file.open(QFile::ReadWrite);
    file.setPermissions(QFile::ExeOwner);
    QVERIFY(file.exists());

    bool expected = true;
    bool result = controller->checkDcrawPath(input);

    QCOMPARE(result, expected);

    QVERIFY(file.remove());
}

void RawControllerTest::test_saveSettings_fileExecutable()
{
    QString input("/test_dcraw.bin");
    input.prepend(QDir::tempPath());

    QFile file(input);
    QVERIFY(!file.exists());
    file.open(QFile::ReadWrite);
    file.setPermissions(QFile::ExeOwner);
    QVERIFY(file.exists());

    view->setEnabledChecked(true);
    view->setPathText(file.fileName());
    controller->saveSettings();

    RawModel *model = controller->model;
    QCOMPARE(model->isEnabled(), true);
    QCOMPARE(model->dcrawPath(), view->pathText());
    QCOMPARE(model->dcrawOptions(), view->optionsText());

    QVERIFY(file.remove());
}

void RawControllerTest::test_saveSettings_fileNotExists()
{
    QString input("/test_dcraw.bin");
    input.prepend(QDir::tempPath());

    QFile file(input);
    QVERIFY(!file.exists());

    view->setEnabledChecked(true);
    view->setPathText(file.fileName());
    controller->saveSettings();

    RawModel *model = controller->model;
    QCOMPARE(model->isEnabled(), false);
    QCOMPARE(model->dcrawPath(), view->pathText());
    QCOMPARE(model->dcrawOptions(), view->optionsText());
}

void RawControllerTest::test_saveSettings_rawSupportDisabled()
{
    view->setEnabledChecked(false);
    controller->saveSettings();

    RawModel *model = controller->model;
    QCOMPARE(model->isEnabled(), false);
    QCOMPARE(model->dcrawPath(), view->pathText());
    QCOMPARE(model->dcrawOptions(), view->optionsText());
}


QTEST_MAIN(RawControllerTest)
#include "RawControllerTest.moc"
