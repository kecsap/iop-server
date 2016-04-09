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

#ifndef TableMarkers_hpp
#define TableMarkers_hpp

#include <MCSamples.hpp>

class MEImage;
class MEPoint;

class TableMarkers
{
public:
  TableMarkers(int image_width, int image_height);
  virtual ~TableMarkers();

  void Reset();
  void AddImage(MEImage& image);
  bool IsReady();
  bool IsAnyMissingCorner();
  void GetRotationalCorrection(MEImage& image, float& angle, MEPoint& center);
  void DrawMissingCorners(MEImage& image);
  void DrawDebugSigns(MEImage& image);

private:
  MEPoint FindCorner(MEImage& image, int x1, int y1, int x2, int y2);

protected:
  const int ImageWidth;
  const int ImageHeight;
  const int FrameLimit;
  const int CornerSampleCount;
  const int CornerRegionWidth;
  const int CornerRegionHeight;
  const int RegionGap;
  int FrameCount;
  MCSamples<int> Corner1X;
  MCSamples<int> Corner1Y;
  MCSamples<int> Corner2X;
  MCSamples<int> Corner2Y;
  MCSamples<int> Corner3X;
  MCSamples<int> Corner3Y;
  MCSamples<int> Corner4X;
  MCSamples<int> Corner4Y;
};

#endif
