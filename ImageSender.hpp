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

#ifndef ImageSender_hpp
#define ImageSender_hpp

#include <QMutexLocker>
#include <qquickimageprovider.h>
#include <qtcpsocket.h>

#include <boost/scoped_ptr.hpp>

#include <vector>

class MECalibration;
class MEImage;

class ImageSender : public QTcpSocket, public QQuickImageProvider
{
  Q_OBJECT
public:
  ImageSender(const QString& host_name);
  virtual ~ImageSender();

private Q_SLOTS:
  void DataArrived();
  void ErrorOccured(QAbstractSocket::SocketError error);
  void StateChanged(QAbstractSocket::SocketState new_state);

public:
  virtual QImage requestImage(const QString& id, QSize* size, const QSize& requested_size);
  void SetImage(MEImage& data);
  void SendImage();

private:
  boost::scoped_ptr<MECalibration> Calibration;
  std::vector<char> ImageData;
  boost::scoped_ptr<MEImage> Image;
  QMutex SendMutex, ImageCopy;
};

#endif
