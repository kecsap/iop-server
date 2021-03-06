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

#include <qcoreapplication.h>
#include <qfile.h>
#include <QtConcurrentRun>
#include <qtimer.h>

#include <opencv/cv.h>

#include <boost/bind.hpp>

VideoWatcher::VideoWatcher(const QString& video_file, bool normal_playback) : FrameWidth(320), FrameHeight(180),
  FrameDuration(34), FrameCount(0), OverallFrameCount(0), WaitDuration(0), CaptureDevice(new MECapture),
  CapturedImage(new MEImage), OriginalImage(new MEImage), FinalImage(new MEImage),
  RotationAngle(MCFloatInfinity()), Undistort(true), DebugCorners(false), DebugMotions(false)
{
  DebugMessageImage.reset(new MEImage(FrameWidth*2, FrameHeight*2, 3));
  // Set the calibration data manually because the portable archive does not work by some reason
//  MCBinaryData DataBuffer;

//  DataBuffer.LoadFromQtResource(":/rpi.cal");
//  Calibration.reset(MECalibration::Decode(DataBuffer));

  MC::FloatTable Intrinsics;
  MC::FloatList DistortionCoefficients;

  Intrinsics.resize(3);
  Intrinsics[0].push_back(166);
  Intrinsics[0].push_back(0);
  Intrinsics[0].push_back(160);
  Intrinsics[1].push_back(0);
  Intrinsics[1].push_back(154);
  Intrinsics[1].push_back(90);
  Intrinsics[2].push_back(0);
  Intrinsics[2].push_back(0);
  Intrinsics[2].push_back(1);
  DistortionCoefficients.push_back(-0.12);
  DistortionCoefficients.push_back(0.02);
  DistortionCoefficients.push_back(0.030);
  DistortionCoefficients.push_back(0.01);
  DistortionCoefficients.push_back(0.03);
  Calibration.reset(new MECalibration(FrameWidth, FrameHeight, Intrinsics, DistortionCoefficients));

  // Start the capture device
  if (!video_file.isEmpty())
  {
    CaptureDevice->Start(video_file.toStdString());
    if (!normal_playback)
    {
      // The playback must go with full frame-rate and it will be adjusted automatically
      CaptureDevice->SetPlaybackFPS(1000);
    }
  } else {
    // Capture resolution is 640x360
    CaptureDevice->SetImageWidth(FrameWidth*2);
    CaptureDevice->SetImageHeight(FrameHeight*2);
    CaptureDevice->Start(0);
  }
  FpsTimer.start();
  // Start the capture device
  QFuture<void> CaptureTask = QtConcurrent::run(boost::bind(&VideoWatcher::CaptureImage, this));

  connect(&CaptureWatcher, SIGNAL(finished()), this, SLOT(CaptureFinished()));
  CaptureWatcher.setFuture(CaptureTask);
  // Set table marker finder
  Markers.reset(new TableMarkers(FrameWidth, FrameHeight));
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
  if (DebugCorners || DebugMotions)
  {
    FinalImage->Resize(FrameWidth*2, FrameHeight*2);
    FinalImage->Addition(*DebugMessageImage, ME::MaskAddition);
    return *FinalImage;
  }

  return *OriginalImage;
}


void VideoWatcher::AudioTimestamp(int timestamp)
{
  WaitDuration = ((int)(FrameDuration*OverallFrameCount)-timestamp) / 2;
}


void VideoWatcher::CaptureImage()
{
  CaptureDevice->CaptureFrame(*CapturedImage);
}


void VideoWatcher::CaptureFinished()
{
  QFuture<void> CaptureTask;
  static bool AudioStarted = false;

  // In debug mode, keep the audio and video playback in sync
  if (WaitDuration > 0)
    MCSleep(WaitDuration);

  FrameCount++;
  OverallFrameCount++;
  *OriginalImage = *CapturedImage;
  if (AudioStarted == false)
  {
    QTimer::singleShot(100, this, SIGNAL(StartAudio()));
    AudioStarted = true;
  }
  // Start a new capture
  CaptureTask = QtConcurrent::run(boost::bind(&VideoWatcher::CaptureImage, this));
  CaptureWatcher.setFuture(CaptureTask);
  if (FrameCount % 3 == 1)
    return;

  OriginalImage->ConvertBGRToRGB();
  *FinalImage = *OriginalImage;
  CheckFiles();
  // Check if the capture process stopped by some reason
  if (!CaptureDevice->IsCapturing())
  {
    MC_LOG("Capture stopped");
    QCoreApplication::quit();
  }
  // Be sure that the image has the expected size
  if (FinalImage->GetWidth() != FrameWidth && FinalImage->GetHeight() != FrameHeight)
  {
    FinalImage->Resize(FrameWidth, FrameHeight);
  }
  if (Undistort && FinalImage->GetWidth() == FrameWidth && FinalImage->GetHeight() == FrameHeight)
  {
    Calibration->Undistort(*FinalImage);
    // TODO: The rotational correction is too CPU expensive
    if (!MCIsFloatInfinity(RotationAngle))
    {
      FinalImage->Rotate(RotationCenter.X, RotationCenter.Y, RotationAngle);
    }
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
  if (Markers->IsReady() && !Markers->IsAnyMissingCorner() && MCIsFloatInfinity(RotationAngle))
  {
    // Get the rotational angle and reset the marker detection
    Markers->GetRotationalCorrection(*FinalImage, RotationAngle, RotationCenter);
    MC_LOG("Detected rotation: %1.2f degrees", RotationAngle);
    Markers->Reset();
  }
  // Motion detection
  MEImage MotionFrame = *FinalImage;

  MotionFrame.Resize(FrameWidth / 4, FrameHeight / 4);
  MotionDetection->DetectMotions(MotionFrame);
  /*
   * Draw the debug signs and texts on the original image
   */
  DebugMessageImage->Clear();
  // Composite debug signs
  if (DebugMotions)
  {
    MEImage MaskImage;

    MotionDetection->GetMotionsMask(MaskImage);
    // Convert the grayscale image back to RGB
    MaskImage.ConvertToRGB();
    MaskImage.Resize(FrameWidth, FrameHeight);
    FinalImage->Addition(MaskImage, ME::MaskAddition);
  }
  if (DebugCorners)
    Markers->DrawDebugSigns(*FinalImage);
  if (Markers->IsReady() && Markers->IsAnyMissingCorner())
  {
    Markers->DrawMissingCorners(*FinalImage);
    DebugMessageImage->DrawText(160, 320, "Table not detected", 1, MEColor(255, 255, 255));
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
