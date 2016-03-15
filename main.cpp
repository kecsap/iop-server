/*
 *  This file is part of the iop_sound_prototype
 *
 *  Copyright (C) 2015-2016 Csaba Kertész (csaba.kertesz@gmail.com)
 *
 *  iop_sound_prototype is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  iop_sound_prototype is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include "SoundRecorder.hpp"

#include <qguiapplication.h>
#include <qqmlapplicationengine.h>
#include <qquickwindow.h>

#include <boost/scoped_ptr.hpp>

int main(int argc, char *argv[])
{
  QGuiApplication App(argc, argv);
/*  QQmlApplicationEngine Engine;
  QQuickWindow* View = NULL;
  Engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
  View = qobject_cast<QQuickWindow*>(Engine.rootObjects()[0]);
*/
//  SoundRecorder Recorder();
//  SoundRecorder Recorder("test_audio.wav", Engine.rootObjects()[0]); // OPPONENT
//  SoundRecorder Recorder("test_audio2.wav", Engine.rootObjects()[0]);  // SERVER
//  SoundRecorder Recorder("test_audio3.wav", Engine.rootObjects()[0]); // MESSY -> NO IDEA
//  SoundRecorder Recorder("test_audio4.wav", Engine.rootObjects()[0]); // OPPONENT

//  SoundRecorder Recorder("test_audio.wav", NULL); // OPPONENT
  SoundRecorder Recorder("test_audio2.wav", NULL);  // SERVER
//  SoundRecorder Recorder("test_audio3.wav", NULL); // MESSY -> NO IDEA
//  SoundRecorder Recorder("test_audio4.wav", NULL); // OPPONENT

//  View->showFullScreen();
  return App.exec();
}

