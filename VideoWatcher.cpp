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

#include "VideoWatcher.hpp"

#include <MECalibration.hpp>
#include <MECapture.hpp>
#include <MEImage.hpp>

#include <MCBinaryData.hpp>
#include <MCLog.hpp>

#include <qfile.h>
#include <QtConcurrentRun>

#include <boost/bind.hpp>

VideoWatcher::VideoWatcher(const QString& video_file) : CaptureDevice(new MECapture), CapturedImage(new MEImage),
  FinalImage(new MEImage), FrameCount(0), Undistort(true)
{
  // Load the calibration data
  MCBinaryData DataBuffer;

  DataBuffer.LoadFromQtResource(":/rpi.cal");
  Calibration.reset(MECalibration::Decode(DataBuffer));
  // Start the capture device
  if (!video_file.isEmpty())
  {
    CaptureDevice->Start(video_file.toStdString());
  } else {
    CaptureDevice->SetImageWidth(640);
    CaptureDevice->SetImageHeight(360);
    CaptureDevice->Start(0);
  }
  FpsTimer.start();
  // Start the capture device
  QFuture<void> CaptureTask = QtConcurrent::run(boost::bind(&VideoWatcher::CaptureImage, this));

  connect(&CaptureWatcher, SIGNAL(finished()), this, SLOT(CaptureFinished()));
  CaptureWatcher.setFuture(CaptureTask);
}


VideoWatcher::~VideoWatcher()
{
}


const MEImage& VideoWatcher::GetCapturedImage()
{
  return *FinalImage;
}


void VideoWatcher::CaptureImage()
{
  CaptureDevice->CaptureFrame(*CapturedImage);
}


void VideoWatcher::CaptureFinished()
{
  QFuture<void> CaptureTask;

  FrameCount++;
  if (FrameCount % 4 != 0)
  {
    // Start a new capture
    CaptureTask = QtConcurrent::run(boost::bind(&VideoWatcher::CaptureImage, this));
    CaptureWatcher.setFuture(CaptureTask);
    return;
  }
  *FinalImage = *CapturedImage;
  CheckFiles();
  if (Undistort && FinalImage->GetWidth() == 640 && FinalImage->GetHeight() == 360)
  {
    FinalImage->MirrorHorizontal();
    Calibration->Undistort(*FinalImage);
    FinalImage->MirrorHorizontal();
    Calibration->Undistort(*FinalImage);
  }
  // Calculate fps
  if (FrameCount % 100 == 0)
  {
    MC_LOG("Capture speed: %1.2f fps", (float)1000 / FpsTimer.elapsed()*FrameCount);
    FpsTimer.start();
    FrameCount = 0;
    MC_LOG("Average brightness level: %1.2f", FinalImage->AverageBrightnessLevel());
  }
  // Check if the lights are on or off
  if (FinalImage->AverageBrightnessLevel() < 10)
  {
    Q_EMIT(VideoEvent(IOP::IdleEvent));
  } else {
    Q_EMIT(VideoEvent(IOP::NormalEvent));
  }
  // Check if the capture process stopped by some reason
  if (!CaptureDevice->IsCapturing())
  {
    MC_LOG("Capture stopped");
    exit(0);
  }
  // Start a new capture
  CaptureTask = QtConcurrent::run(boost::bind(&VideoWatcher::CaptureImage, this));
  CaptureWatcher.setFuture(CaptureTask);
  Q_EMIT(VideoEvent(IOP::CaptureEvent));
}


void VideoWatcher::CheckFiles()
{
  if (QFile("no_calibration").exists() && Undistort)
  {
    Undistort = false;
    MC_LOG("Disable the calibration");
  } else
  if (!QFile("no_calibration").exists() && !Undistort)
  {
    Undistort = true;
    MC_LOG("Enable the calibration");
  }
}
