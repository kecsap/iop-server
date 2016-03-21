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

#ifndef VideoWatcher_hpp
#define VideoWatcher_hpp

#include "Defines.hpp"

#include <qfuturewatcher.h>
#include <qobject.h>
#include <QTime>

#include <boost/scoped_ptr.hpp>

class MECalibration;
class MECapture;
class MEImage;

class VideoWatcher : public QObject
{
  Q_OBJECT

public:
  VideoWatcher(const QString& video_file);
  virtual ~VideoWatcher();

  const MEImage& GetCapturedImage();

public Q_SLOTS:
  void CaptureFinished();

private:
  void CaptureImage();
  void CheckFiles();

Q_SIGNALS:
  void VideoEvent(IOP::VideoEventType event);

protected:
  QFutureWatcher<void> CaptureWatcher;
  boost::scoped_ptr<MECapture> CaptureDevice;
  boost::scoped_ptr<MEImage> CapturedImage;
  boost::scoped_ptr<MEImage> FinalImage;
  boost::scoped_ptr<MECalibration> Calibration;
  QTime FpsTimer;
  int FrameCount;
  bool Undistort;
};

#endif
