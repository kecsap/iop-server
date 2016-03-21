/*
 *  This file is part of the iop-server
 *
 *  Copyright (C) 2015-2016 Csaba Kertész (csaba.kertesz@gmail.com)
 *
 *  iop-server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  iop-server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include "GameWatcher.hpp"

#include <MPContext.hpp>

#include <core/MALog.hpp>

#include <MCLog.hpp>

#include <qapplication.h>
#include <qguiapplication.h>
#include <qqmlapplicationengine.h>
#include <qquickwindow.h>

#include <boost/scoped_ptr.hpp>

namespace
{
void Usage()
{
  printf("Usage: iop-server [option ...]\n"
         "\n"
         "  -a, --audiofile STRING       Audio file for debugging\n"
         "  -v, --videofilename STRING   Video file for debugging\n"
         "  -i, --ipaddress STRING       IP address of the wall pi\n"
         "  -d, --debug                  Debug mode with GUI\n"
         "  -h, --help                   Print this text\n"
         "\n\n");

  exit(1);
}
}

int main(int argc, char *argv[])
{
  printf("Internet of Ping Server\n");

  boost::scoped_ptr<MPContext> Context(new MPContext(argc, argv, true));
  MSContext::ArgSearchResult Result;
  QString AudioFile;
  QString VideoFile;
  QString IPAddress;
  bool DebugMode = false;

  MCLog::SetCustomHandler(new MALog(100000), true);
  MCLog::SetDebugStatus(true, true);
  // Scan for -h or --help argument
  Result = Context->FindArgument("-h", "--help");
  if (Result.SearchResult != MSContext::ca_ArgumentNotFound)
  {
    Usage();
    return 0;
  }
  // Scan for -a or --audiofile argument
  Result = Context->FindArgument("-a", "--audiofile");
  if (Result.SearchResult == MSContext::ca_ArgumentFoundWithParameter)
  {
    AudioFile = *Result.Parameter;
  }
  // Scan for -v or --videofile argument
  Result = Context->FindArgument("-v", "--videofile");
  if (Result.SearchResult == MSContext::ca_ArgumentFoundWithParameter)
  {
    VideoFile = *Result.Parameter;
  }
  // Scan for -i or --ipaddress argument
  Result = Context->FindArgument("-i", "--ipaddress");
  if (Result.SearchResult == MSContext::ca_ArgumentFoundWithParameter)
  {
    IPAddress = *Result.Parameter;
  }
  // Scan for -d or --debug argument
  Result = Context->FindArgument("-d", "--debug");
  if (Result.SearchResult != MSContext::ca_ArgumentNotFound)
  {
    DebugMode = true;
  }
  QQmlApplicationEngine Engine;
  QQuickWindow* View = NULL;

  if (DebugMode)
  {
    Engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    View = qobject_cast<QQuickWindow*>(Engine.rootObjects()[0]);
    Engine.addImageProvider(QLatin1String("camera"), new ImageProvider);
  }
  GameWatcher Watcher(AudioFile, VideoFile, IPAddress, (DebugMode ? Engine.rootObjects()[0] : NULL));

//  if (DebugMode)
//    View->showFullScreen();

  return QApplication::instance()->exec();
}

