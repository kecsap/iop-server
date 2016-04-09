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

#include "ImageSender.hpp"

#include <MECalibration.hpp>
#include <MEImage.hpp>

#include <MCBinaryData.hpp>

#include <qglobal.h>
#include <qhostaddress.h>
#include <qmetatype.h>
#include <QNetworkAddressEntry>
#include <QTime>
#include <qtimer.h>

#include <boost/make_shared.hpp>

ImageSender::ImageSender(const QString& host_name) : QTcpSocket(), QQuickImageProvider(QQuickImageProvider::Image),
  Image(new MEImage)
{
  // Undistortion parameters for resolution 640x360
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

  connectToHost(QHostAddress(host_name), 10000);
  qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
  qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketState");
  connect(this, SIGNAL(readyRead()), this, SLOT(DataArrived()), Qt::QueuedConnection);
  connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
          this, SLOT(ErrorOccured(QAbstractSocket::SocketError)));
  connect(this, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
          this, SLOT(StateChanged(QAbstractSocket::SocketState)));
}


ImageSender::~ImageSender()
{
  disconnect(this, SIGNAL(readyRead()), this, SLOT(DataArrived()));
  disconnect(this, SIGNAL(error(QAbstractSocket::SocketError)));
}


void ImageSender::DataArrived()
{
  if (bytesAvailable() == 0)
    return;

  ImageData.resize(bytesAvailable());
  printf("Read: %d bytes\n", (int)bytesAvailable());
  readData((char*)&ImageData[0], bytesAvailable());
}


void ImageSender::ErrorOccured(QAbstractSocket::SocketError error)
{
  Q_UNUSED(error);
  printf("TCP socket error: %s\n", qPrintable(errorString()));
}

void ImageSender::StateChanged(QAbstractSocket::SocketState new_state)
{
  Q_UNUSED(new_state);
  printf("TCP state change: %d\n", new_state);
}


QImage ImageSender::requestImage(const QString& id, QSize* size, const QSize& requested_size)
{
  Q_UNUSED(id);
  Q_UNUSED(size);
  Q_UNUSED(requested_size);

  if (ImageData.size() > 0)
  {
    QImage Image;

    Image.loadFromData((uchar*)&ImageData[0], ImageData.size(), "JPG");

    if (size)
      *size = QSize(Image.width(), Image.height());

    ImageData.clear();
    return Image;
  }
  // Provide a grayscale image
  QImage Image(640, 360, QImage::Format_RGB888);

  memset(Image.bits(), 100, 640*360*3);
  return Image;
}


void ImageSender::SetImage(MEImage& image)
{
  QMutexLocker Lock(&ImageCopy);

  *Image = image;
}


void ImageSender::SendImage()
{
  MC::BinaryDataSPtr ImageData;

  {
    QMutexLocker Lock(&ImageCopy);

    if (Image->GetWidth() == 640 && Image->GetHeight() == 360)
      Calibration->Undistort(*Image);

    ImageData.reset(Image->Compress());
  }
  // Use a chunk limit (laziness)
  if (ImageData->GetSize() <= 65483)
  {
    QMutexLocker Lock(&SendMutex);

    writeData((char*)ImageData->GetData(), ImageData->GetSize());
  }
}
