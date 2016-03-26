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

#ifndef GameWatcher_hpp
#define GameWatcher_hpp

#include "Defines.hpp"

#include <qobject.h>
#include <qquickimageprovider.h>
#include <QTime>

#include <boost/scoped_ptr.hpp>

class AudioWatcher;
class ImageSender;
class VideoWatcher;

class ImageProvider : public QQuickImageProvider
{
public:
  ImageProvider();

  virtual QImage requestImage(const QString& id, QSize* size, const QSize& requested_size);
};

class GameWatcher : public QObject
{
  Q_OBJECT

public:
  GameWatcher(const QString& audio_file, const QString& video_file, const QString& wallpi_ip, QObject* root_object);
  virtual ~GameWatcher();

public Q_SLOTS:

  void AudioEvent(IOP::AudioEventType event);
  void VideoEvent(IOP::VideoEventType event);
  void ShowStatusText(const QString& text);

protected:
  QObject* Page;
  QString StatusText;
  QTime StatusTextTimer;
  boost::scoped_ptr<AudioWatcher> AudioListener;
  boost::scoped_ptr<VideoWatcher> VideoListener;
  boost::scoped_ptr<ImageSender> ImageSocket;
  bool InIdle;
};

#endif
