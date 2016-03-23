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

#include "TableMarkers.hpp"

#include <MECalibration.hpp>
#include <MECapture.hpp>
#include <MEImage.hpp>
#include <MEMotionDetection.hpp>

#include <MCBinaryData.hpp>
#include <MCLog.hpp>

#include <qfile.h>
#include <QtConcurrentRun>

#include <opencv/cv.h>

#include <boost/bind.hpp>

VideoWatcher::VideoWatcher(const QString& video_file) : CaptureDevice(new MECapture),
  CapturedImage(new MEImage), FinalImage(new MEImage), FrameCount(0),
  RotationAngle(MCFloatInfinity()), Undistort(true), DebugCorners(false),
  DebugMotions(false)
{
  // Set the calibration data manually because the portable archive does not work by some reason
//  MCBinaryData DataBuffer;

//  DataBuffer.LoadFromQtResource(":/rpi.cal");
//  Calibration.reset(MECalibration::Decode(DataBuffer));

  MC::FloatTable Intrinsics;
  MC::FloatList DistortionCoefficients;

  Intrinsics.resize(3);
  Intrinsics[0].push_back(930);
  Intrinsics[0].push_back(0);
  Intrinsics[0].push_back(320);
  Intrinsics[1].push_back(0);
  Intrinsics[1].push_back(900);
  Intrinsics[1].push_back(180);
  Intrinsics[2].push_back(0);
  Intrinsics[2].push_back(0);
  Intrinsics[2].push_back(1);
  DistortionCoefficients.push_back(0.06);
  DistortionCoefficients.push_back(-4.3);
  DistortionCoefficients.push_back(0.03);
  DistortionCoefficients.push_back(0.04);
  DistortionCoefficients.push_back(0.1);
  Calibration.reset(new MECalibration(640, 360, Intrinsics, DistortionCoefficients));

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
  // Set table marker finder
  Markers.reset(new TableMarkers);
  // Set motion detection
  MotionDetection.reset(new MEMotionDetection);
  MotionDetection->SetParameter(MEMotionDetection::mdp_HUHistogramsPerPixel, (float)4);
  MotionDetection->SetMode(MEMotionDetection::md_LBPHistograms);
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
  *FinalImage = *CapturedImage;
  // Start a new capture
  CaptureTask = QtConcurrent::run(boost::bind(&VideoWatcher::CaptureImage, this));
  CaptureWatcher.setFuture(CaptureTask);
  CheckFiles();
  // Check if the capture process stopped by some reason
  if (!CaptureDevice->IsCapturing())
  {
    MC_LOG("Capture stopped");
    exit(0);
  }
  FinalImage->ConvertBGRToRGB();
  if (Undistort && FinalImage->GetWidth() == 640 && FinalImage->GetHeight() == 360)
  {
    FinalImage->MirrorHorizontal();
    Calibration->Undistort(*FinalImage);
    FinalImage->MirrorHorizontal();
    Calibration->Undistort(*FinalImage);
    // TODO: The rotational correction is too CPU expensive
//    if (!MCIsFloatInfinity(RotationAngle))
//    {
//      FinalImage->Rotate(RotationCenter.X, RotationCenter.Y, RotationAngle);
//    }
  }
  // Check if the lights are off
  if (FinalImage->AverageBrightnessLevel() < 10)
  {
    Markers->Reset();
    MotionDetection->Reset();
    Q_EMIT(VideoEvent(IOP::IdleEvent));
    Q_EMIT(VideoEvent(IOP::CaptureEvent));
    return;
  }
  // Calculate fps
  if (FrameCount % 300 == 0)
  {
    MC_LOG("Capture speed: %1.2f fps", (float)1000 / FpsTimer.elapsed()*FrameCount);
    FpsTimer.start();
    FrameCount = 0;
    MC_LOG("Average brightness level: %1.2f", FinalImage->AverageBrightnessLevel());
  }
  // Corner detection
  Markers->AddImage(*FinalImage);
    // TODO: The rotational correction is too CPU expensive
//  if (Markers->IsReady() && !Markers->IsAnyMissingCorner() && MCIsFloatInfinity(RotationAngle))
//  {
//    // Get the rotational angle and reset the marker detection
//    Markers->GetRotationalCorrection(*FinalImage, RotationAngle, RotationCenter);
//    Markers->Reset();
//  }
  // Motion detection
  MEImage MotionFrame = *FinalImage;

  MotionFrame.Resize(640 / 4, 360 / 4);
  MotionDetection->DetectMotions(MotionFrame);
  // Composite debug signs
  if (DebugMotions)
  {
    MotionDetection->GetMotionsMask(*FinalImage);
    // Convert the grayscale image back to RGB
    FinalImage->Resize(640, 360);
    FinalImage->ConvertToRGB();
  }
  if (DebugCorners)
    Markers->DrawDebugSigns(*FinalImage);
  if (Markers->IsReady() && Markers->IsAnyMissingCorner())
  {
    Markers->DrawMissingCorners(*FinalImage);
    FinalImage->DrawText(160, 320, "Table not detected", 1, MEColor(255, 255, 255));
    Q_EMIT(VideoEvent(IOP::MissingCornersEvent));
  }
  Q_EMIT(VideoEvent(IOP::NormalEvent));
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

  if (QFile("debug_corners").exists() && !DebugCorners)
  {
    DebugCorners = true;
    MC_LOG("Enable corner detection debugging");
  } else
  if (!QFile("debug_corners").exists() && DebugCorners)
  {
    DebugCorners = false;
    MC_LOG("Disable corner detection debugging");
  }

  if (QFile("debug_motions").exists() && !DebugMotions)
  {
    DebugMotions = true;
    MC_LOG("Enable motion detection debugging");
  } else
  if (!QFile("debug_motions").exists() && DebugMotions)
  {
    DebugMotions = false;
    MC_LOG("Disable motion detection debugging");
  }
}
