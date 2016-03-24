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

#include "TableMarkers.hpp"

#include <MEImage.hpp>

#include <MCLog.hpp>
#include <MCSampleStatistics.hpp>

#include <opencv/cv.h>

TableMarkers::TableMarkers() : FrameLimit(60), CornerSampleCount(40),
  CornerRegionWidth(80), CornerRegionHeight(60), RegionGap(5), FrameCount(0),
  Corner1X(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner1Y(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner2X(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner2Y(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner3X(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner3Y(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner4X(CornerSampleCount, *new MCArithmeticMean<int>("Mean")),
  Corner4Y(CornerSampleCount, *new MCArithmeticMean<int>("Mean"))
{
}


TableMarkers::~TableMarkers()
{
}


void TableMarkers::Reset()
{
  FrameCount = 0;
  Corner1X.Reset();
  Corner1Y.Reset();
  Corner2X.Reset();
  Corner2Y.Reset();
  Corner3X.Reset();
  Corner3Y.Reset();
  Corner4X.Reset();
  Corner4Y.Reset();
}


void TableMarkers::AddImage(MEImage& image)
{
  if (FrameCount == FrameLimit)
    return;

  MEPoint Point1 = FindCorner(image, RegionGap, RegionGap, CornerRegionWidth, CornerRegionHeight);
  MEPoint Point2 = FindCorner(image, image.GetWidth()-1-CornerRegionWidth, RegionGap, image.GetWidth()-1-RegionGap, CornerRegionHeight);
  MEPoint Point3 = FindCorner(image, RegionGap, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-RegionGap);
  MEPoint Point4 = FindCorner(image, image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
                              image.GetWidth()-1-RegionGap, image.GetHeight()-1-RegionGap);

//  MEPoint Point1 = image.FindCorner(RegionGap, RegionGap, CornerRegionWidth, CornerRegionHeight);
//  MEPoint Point2 = image.FindCorner(image.GetWidth()-1-CornerRegionWidth, RegionGap, image.GetWidth()-1-RegionGap, CornerRegionHeight);
//  MEPoint Point3 = image.FindCorner(RegionGap, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-RegionGap);
//  MEPoint Point4 = image.FindCorner(image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
//                                    image.GetWidth()-1-RegionGap, image.GetHeight()-1-RegionGap);

  FrameCount++;
  if (Point1.IsValid())
  {
    Corner1X << Point1.X;
    Corner1Y << Point1.Y;
  }
  if (Point2.IsValid())
  {
    Corner2X << Point2.X;
    Corner2Y << Point2.Y;
  }
  if (Point3.IsValid())
  {
    Corner3X << Point3.X;
    Corner3Y << Point3.Y;
  }
  if (Point4.IsValid())
  {
    Corner4X << Point4.X;
    Corner4Y << Point4.Y;
  }
}


bool TableMarkers::IsReady()
{
  return FrameCount >= FrameLimit;
}


bool TableMarkers::IsAnyMissingCorner()
{
  if (FrameCount < FrameLimit)
    return false;

  bool Ready = true;

  Ready = Ready && Corner1X.IsValid() && Corner1Y.IsValid();
  Ready = Ready && Corner2X.IsValid() && Corner2Y.IsValid();
  Ready = Ready && Corner3X.IsValid() && Corner3Y.IsValid();
  Ready = Ready && Corner4X.IsValid() && Corner4Y.IsValid();
  return !Ready;
}


void TableMarkers::GetRotationalCorrection(MEImage& image, float& angle, MEPoint& center)
{
  if (IsAnyMissingCorner())
    return;

  const int PosX1 = (int)Corner1X.GetStatistic("Mean")->GetResult();
  const int PosY1 = (int)Corner1Y.GetStatistic("Mean")->GetResult();
  const int PosX2 = (int)Corner2X.GetStatistic("Mean")->GetResult();
  const int PosY2 = (int)Corner2Y.GetStatistic("Mean")->GetResult();
  const int PosX3 = (int)Corner3X.GetStatistic("Mean")->GetResult();
  const int PosY3 = (int)Corner3Y.GetStatistic("Mean")->GetResult();
  const int PosX4 = (int)Corner4X.GetStatistic("Mean")->GetResult();
  const int PosY4 = (int)Corner4Y.GetStatistic("Mean")->GetResult();
  MEPoint Point1((PosX1+PosX2) / 2, (PosY1+PosY2) / 2);
  MEPoint Point2((PosX3+PosX4) / 2, (PosY3+PosY4) / 2);
  float Degree = MEGetInteriorAngle(Point2, Point1, MEPoint((PosX1+PosX2) / 2, image.GetHeight()));

  angle = (180-Degree) / 3;
  center.X = (PosX1+PosX2+PosX3+PosX4) / 4;
  center.Y = (PosY1+PosY2+PosY3+PosY4) / 4;
}


void TableMarkers::DrawMissingCorners(MEImage& image)
{
  if (FrameCount < FrameLimit)
    return;

  if (!Corner1X.IsValid() || !Corner1Y.IsValid())
  {
    image.DrawRectangle(RegionGap, RegionGap, CornerRegionWidth, CornerRegionHeight, MEColor(255, 70, 70), false);
  }
  if (!Corner2X.IsValid() || !Corner2Y.IsValid())
  {
    image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, RegionGap, image.GetWidth()-1-RegionGap, CornerRegionHeight,
                        MEColor(255, 70, 70), false);
  }
  if (!Corner3X.IsValid() || !Corner3Y.IsValid())
  {
    image.DrawRectangle(RegionGap, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-RegionGap,
                        MEColor(255, 70, 70), false);
  }
  if (!Corner4X.IsValid() || !Corner4Y.IsValid())
  {
    image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
                        image.GetWidth()-1-RegionGap, image.GetHeight()-1-RegionGap, MEColor(255, 70, 70), false);
  }
}


void TableMarkers::DrawDebugSigns(MEImage& image)
{
  image.DrawRectangle(RegionGap, RegionGap, CornerRegionWidth, CornerRegionHeight, MEColor(0, 255, 0), false);
  image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, RegionGap, image.GetWidth()-1-RegionGap, CornerRegionHeight,
                      MEColor(0, 255, 0), false);
  image.DrawRectangle(RegionGap, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-RegionGap,
                      MEColor(0, 255, 0), false);
  image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
                      image.GetWidth()-1-RegionGap, image.GetHeight()-1-RegionGap, MEColor(0, 255, 0), false);

  const int PosX1 = (int)Corner1X.GetStatistic("Mean")->GetResult();
  const int PosY1 = (int)Corner1Y.GetStatistic("Mean")->GetResult();
  const int PosX2 = (int)Corner2X.GetStatistic("Mean")->GetResult();
  const int PosY2 = (int)Corner2Y.GetStatistic("Mean")->GetResult();
  const int PosX3 = (int)Corner3X.GetStatistic("Mean")->GetResult();
  const int PosY3 = (int)Corner3Y.GetStatistic("Mean")->GetResult();
  const int PosX4 = (int)Corner4X.GetStatistic("Mean")->GetResult();
  const int PosY4 = (int)Corner4Y.GetStatistic("Mean")->GetResult();

  // Draw the detected corners
  if (Corner1X.IsValid() && Corner1Y.IsValid())
    image.DrawCircle(PosX1, PosY1, 5, MEColor(0, 0, 255));
  if (Corner2X.IsValid() && Corner2Y.IsValid())
    image.DrawCircle(PosX2, PosY2, 5, MEColor(0, 0, 255));
  if (Corner3X.IsValid() && Corner3Y.IsValid())
    image.DrawCircle(PosX3, PosY3, 5, MEColor(0, 0, 255));
  if (Corner4X.IsValid() && Corner4Y.IsValid())
    image.DrawCircle(PosX4, PosY4, 5, MEColor(0, 0, 255));

  // Draw intermediate points and lines
  if (Corner1X.IsValid() && Corner1Y.IsValid() && Corner2X.IsValid() && Corner2Y.IsValid() &&
      Corner3X.IsValid() && Corner3Y.IsValid() && Corner4X.IsValid() && Corner4Y.IsValid())
  {
    image.DrawCircle((PosX1+PosX2) / 2, (PosY1+PosY2) / 2, 5, MEColor(255, 255, 0));
    image.DrawCircle((PosX3+PosX4) / 2, (PosY3+PosY4) / 2, 5, MEColor(255, 255, 0));
    image.DrawCircle((PosX1+PosX3) / 2, (PosY1+PosY3) / 2, 5, MEColor(255, 255, 0));
    image.DrawCircle((PosX2+PosX4) / 2, (PosY2+PosY4) / 2, 5, MEColor(255, 255, 0));

    image.DrawLine((PosX1+PosX2) / 2, (PosY1+PosY2) / 2,
                   (PosX3+PosX4) / 2, (PosY3+PosY4) / 2, MEColor(255, 255, 0));
    image.DrawLine((PosX1+PosX3) / 2, (PosY1+PosY3) / 2,
                   (PosX2+PosX4) / 2, (PosY2+PosY4) / 2, MEColor(255, 255, 0));
  }
}


MEPoint TableMarkers::FindCorner(MEImage& image, int x1, int y1, int x2, int y2)
{
  const int X1 = MCBound(0, x1, image.GetWidth()-1);
  const int Y1 = MCBound(0, y1, image.GetHeight()-1);
  const int X2 = MCBound(0, x2, image.GetWidth()-1);
  const int Y2 = MCBound(0, y2, image.GetHeight()-1);

  if (X2-X1 <= 0 || Y2-Y1 <= 0)
    return MEPoint(-1, -1);

  // Copy the search region
  ME::ImageSPtr RegionImage(new MEImage);

  image.CopyImagePart(X1, Y1, X2, Y2, *RegionImage);
  RegionImage->ConvertToGrayscale();
  RegionImage->AdaptiveThreshold();
  RegionImage->Binarize(1);

  unsigned char* ImageData = (unsigned char*)RegionImage->GetIplImage()->imageData;
  int RowStart = 0;
  MC::IntList PositionsX;
  MC::IntList PositionsY;

  // Search for vertical positions
  for (int y = 0; y < RegionImage->GetHeight(); ++y)
  {
    int WhiteCounter = 0;

    for (int x = RegionImage->GetWidth()-1; x >= 0; --x)
    {
      if (ImageData[RowStart+x] > 0)
        WhiteCounter++;
    }
    if (WhiteCounter > 10)
      PositionsY.push_back(y);

    RowStart += RegionImage->GetRowWidth();
  }
  // Search for horizontal positions
  for (int x = 0; x < RegionImage->GetWidth(); ++x)
  {
    int WhiteCounter = 0;

    RowStart = x;
    for (int y = 0; y < RegionImage->GetHeight(); ++y)
    {
      if (ImageData[RowStart] > 0)
        WhiteCounter++;

      RowStart += RegionImage->GetRowWidth();
    }
    if (WhiteCounter > 10)
      PositionsX.push_back(x);
  }
  if (PositionsX.empty() || PositionsY.empty())
    return MEPoint(-1, -1);

  if (PositionsX.size() == 1 || PositionsY.size() == 1)
    return MEPoint(PositionsX[0], PositionsY[0]);

  int PosX = x1+(int)MCCalculateVectorStatistic(PositionsX, *new MCArithmeticMean<int>);
  int PosY = y1+(int)MCCalculateVectorStatistic(PositionsY, *new MCArithmeticMean<int>);

  return MEPoint(PosX, PosY);
}
