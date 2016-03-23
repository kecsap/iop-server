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

TableMarkers::TableMarkers() : FrameLimit(120), CornerSampleCount(30),
  CornerRegionWidth(140), CornerRegionHeight(120), FrameCount(0),
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

  MEPoint Point1 = image.FindCorner(10, 10, CornerRegionWidth, CornerRegionHeight);
  MEPoint Point2 = image.FindCorner(image.GetWidth()-1-CornerRegionWidth, 10, image.GetWidth()-1-10, CornerRegionHeight);
  MEPoint Point3 = image.FindCorner(10, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-10);
  MEPoint Point4 = image.FindCorner(image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
                                    image.GetWidth()-1-10, image.GetHeight()-1-10);

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

  const int PosX1 = 10+(int)Corner1X.GetStatistic("Mean")->GetResult();
  const int PosY1 = 10+(int)Corner1Y.GetStatistic("Mean")->GetResult();
  const int PosX2 = image.GetWidth()-1-CornerRegionWidth+(int)Corner2X.GetStatistic("Mean")->GetResult();
  const int PosY2 = 10+(int)Corner2Y.GetStatistic("Mean")->GetResult();
  const int PosX3 = 10+(int)Corner3X.GetStatistic("Mean")->GetResult();
  const int PosY3 = image.GetHeight()-1-CornerRegionHeight+(int)Corner3Y.GetStatistic("Mean")->GetResult();
  const int PosX4 = image.GetWidth()-1-CornerRegionWidth+(int)Corner4X.GetStatistic("Mean")->GetResult();
  const int PosY4 = image.GetHeight()-1-CornerRegionHeight+(int)Corner4Y.GetStatistic("Mean")->GetResult();
  MEPoint Point1((PosX1+PosX2) / 2, (PosY1+PosY2) / 2);
  MEPoint Point2((PosX3+PosX4) / 2, (PosY3+PosY4) / 2);
  float Degree = MEGetInteriorAngle(Point2, Point1, MEPoint((PosX1+PosX2) / 2, image.GetHeight()));

  angle = (180-Degree) / 5;
  center.X = (PosX1+PosX2+PosX3+PosX4) / 4;
  center.Y = (PosY1+PosY2+PosY3+PosY4) / 4;
}


void TableMarkers::DrawMissingCorners(MEImage& image)
{
  if (FrameCount < FrameLimit)
    return;

  if (!Corner1X.IsValid() || !Corner1Y.IsValid())
  {
    image.DrawRectangle(10, 10, CornerRegionWidth, CornerRegionHeight, MEColor(255, 70, 70), false);
  }
  if (!Corner2X.IsValid() || !Corner2Y.IsValid())
  {
    image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, 10, image.GetWidth()-1-10, CornerRegionHeight,
                        MEColor(255, 70, 70), false);
  }
  if (!Corner3X.IsValid() || !Corner3Y.IsValid())
  {
    image.DrawRectangle(10, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-10,
                        MEColor(255, 70, 70), false);
  }
  if (!Corner4X.IsValid() || !Corner4Y.IsValid())
  {
    image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
                        image.GetWidth()-1-10, image.GetHeight()-1-10, MEColor(255, 70, 70), false);
  }
}


void TableMarkers::DrawDebugSigns(MEImage& image)
{
  image.DrawRectangle(10, 10, CornerRegionWidth, CornerRegionHeight, MEColor(0, 255, 0), false);
  image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, 10, image.GetWidth()-1-10, CornerRegionHeight,
                      MEColor(0, 255, 0), false);
  image.DrawRectangle(10, image.GetHeight()-1-CornerRegionHeight, CornerRegionWidth, image.GetHeight()-1-10,
                      MEColor(0, 255, 0), false);
  image.DrawRectangle(image.GetWidth()-1-CornerRegionWidth, image.GetHeight()-1-CornerRegionHeight,
                      image.GetWidth()-1-10, image.GetHeight()-1-10, MEColor(0, 255, 0), false);

  const int PosX1 = 10+(int)Corner1X.GetStatistic("Mean")->GetResult();
  const int PosY1 = 10+(int)Corner1Y.GetStatistic("Mean")->GetResult();
  const int PosX2 = image.GetWidth()-1-CornerRegionWidth+(int)Corner2X.GetStatistic("Mean")->GetResult();
  const int PosY2 = 10+(int)Corner2Y.GetStatistic("Mean")->GetResult();
  const int PosX3 = 10+(int)Corner3X.GetStatistic("Mean")->GetResult();
  const int PosY3 = image.GetHeight()-1-CornerRegionHeight+(int)Corner3Y.GetStatistic("Mean")->GetResult();
  const int PosX4 = image.GetWidth()-1-CornerRegionWidth+(int)Corner4X.GetStatistic("Mean")->GetResult();
  const int PosY4 = image.GetHeight()-1-CornerRegionHeight+(int)Corner4Y.GetStatistic("Mean")->GetResult();

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
