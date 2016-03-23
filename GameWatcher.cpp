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

#include "AudioWatcher.hpp"
#include "ImageSender.hpp"
#include "VideoWatcher.hpp"

#include <MEDefs.hpp>
#include <MEImage.hpp>

#include <qmetatype.h>

static ME::ImageSPtr StaticImage;

ImageProvider::ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
{
}


QImage ImageProvider::requestImage(const QString& id, QSize* size, const QSize& requested_size)
{
  MC_UNUSED(id);
  MC_UNUSED(size);
  MC_UNUSED(requested_size);

  if (StaticImage.get())
  {
    MC::BinaryDataSPtr EncodedImage(StaticImage->Compress(ME::JpegFormat));

    if (EncodedImage.get())
    {
      if (size)
        *size = QSize(640, 360);

      return QImage::fromData(EncodedImage->GetData(), EncodedImage->GetSize());
    }
  }
  // Provide a grayscale image
  QImage Image(640, 360, QImage::Format_RGB888);

  memset(Image.bits(), 100, 640*360*3);
  return Image;
}


GameWatcher::GameWatcher(const QString& audio_file, const QString& video_file, const QString& wallpi_ip,
                         QObject* root_object) : Page(NULL), InIdle(true)
{
  qRegisterMetaType<IOP::VideoEventType>("IOP::VideoEventType");
  AudioListener.reset(new AudioWatcher(audio_file));
  if (!wallpi_ip.isEmpty())
    ImageSocket.reset(new ImageSender(wallpi_ip));
  VideoListener.reset(new VideoWatcher(video_file));
  connect(VideoListener.get(), SIGNAL(VideoEvent(IOP::VideoEventType)), this, SLOT(VideoEvent(IOP::VideoEventType)));
  if (root_object)
  {
    Page = root_object->findChild<QObject*>("fuckYoo");
  }
  connect(&FileWatcher, SIGNAL(fileChanged(const QString&)), this, SLOT(FileChanged(const QString&)));
}


GameWatcher::~GameWatcher()
{
}


void GameWatcher::AudioEvent(IOP::AudioEventType event)
{
}


void GameWatcher::VideoEvent(IOP::VideoEventType event)
{
  if (event == IOP::CaptureEvent)
  {
    StaticImage.reset(new MEImage(VideoListener->GetCapturedImage()));
    if (InIdle)
    {
      StaticImage->DrawText(350, 320, "Lights off", 1.1, MEColor(255, 255, 255));
    }
    if (Page)
    {
//      StaticImage->DrawRectangle(0, 0, 140, 120, MEColor(44, 244, 30), false);
      Page->setProperty("cameraImage", "image://camera/image.jpg"+QString::number(MCRand<int>(0, 100000)));
    }
    if (ImageSocket.get())
    {
      MC::BinaryDataSPtr ImageData(StaticImage->Compress());

      ImageSocket->Send(*ImageData);
    }
  } else
  if (event == IOP::NormalEvent && InIdle)
  {
    InIdle = false;
  } else
  if (event == IOP::IdleEvent && !InIdle)
  {
    InIdle = true;
  }
}


void GameWatcher::ShowStatusText(const QString& text, int duration)
{
  printf("Show: %s\n", qPrintable(text));
  if (Page)
  {
    Page->setProperty("resultText", text);
    StatusTextResetTimer.start(duration);
  }
}


void GameWatcher::ResetStatusText()
{
  if (Page)
    Page->setProperty("resultText", "");
}


void GameWatcher::FileChanged(const QString& /*path*/)
{
/*  if (path.contains("no_calibration"))
  {
    if (QFile(path).exists())
      Calibration = true;
    else
      Calibration = false;
  } */
}
