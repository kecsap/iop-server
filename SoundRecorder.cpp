/*
 *  This file is part of the iop_sound_prototype
 *
 *  Copyright (C) 2015 Csaba Kertész (csaba.kertesz@gmail.com)
 *
 *  AiBO+ is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  AiBO+ is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include "SoundRecorder.hpp"

#include <qdatastream.h>
#include <qfile.h>
#include <qresource.h>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>

#include <libxtract.h>

#include <boost/unordered_map.hpp>

const int SampleRate = 16000;
const int HannWindowSize = 512;
const int SlidingWindowSize = 2048;

QString GetAudioDevice()
{
  QAudioRecorder RecorderInfo;
  QStringList InputDevices = RecorderInfo.audioInputs();

  fprintf(stderr, "Input devices:\n");
  for (int i = 0; i < InputDevices.size(); ++i)
  {
    fprintf(stderr, "%s\n", qPrintable(InputDevices[i]));
  }
  return "hw:CARD=Device,DEV=0";
}

template <typename T, typename U>
T GetMostFrequentItemFromContainer(const U& container)
{
  if (container.empty())
    return (T)0;

  boost::unordered_map<float, int> Counts;

  for (unsigned int i = 0; i < container.size(); ++i)
  {
    Counts[container[i]]++;
  }
  int MaxCount = 0;
  T Item = (T)0;

  for (boost::unordered_map<float, int>::const_iterator Iter = Counts.begin();
       Iter != Counts.end(); ++Iter)
  {
    if (MaxCount < Iter->second)
    {
      MaxCount = Iter->second;
      Item = Iter->first;
    }
  }
  return Item;
}

template <typename T, typename U>
int ItemCountInContainer(const U& container, const T item)
{
  if (container.size() == 0)
    return 0;

  unsigned int ContainerSize = container.size();
  int Hits = 0;

  for (unsigned int i = 0; i < ContainerSize; ++i)
  {
    if (container[i] == item)
      Hits++;
  }
  return Hits;
}

template <typename U>
void PrintContainerAsFloats(U& container, const std::string& message = "", unsigned int digits = 2)
{
  if (container.empty())
    return;

  std::stringstream FormatString;

  if (!message.empty())
    FormatString << message << ": " << std::fixed;

  FormatString.precision(digits);
  for (unsigned int i = 0; i < container.size(); ++i)
  {
    FormatString << (float)container[i];
    if (i < container.size()-1)
      FormatString << ", ";
  }
  printf("%s\n", FormatString.str().c_str());
}


SoundRecorder::SoundRecorder() : Device(NULL), BufferPos(0)
{
  // Set up the classifier
  boost::scoped_ptr<cv::FileStorage> FileStorage;
  QResource XmlResource(":/rt.xml");
  QFile ClassifierFile(XmlResource.absoluteFilePath());
  QDataStream InputStream(&ClassifierFile);
  std::vector<char> DataBuffer(ClassifierFile.size(), 0);
  cv::FileNode FileNode;

  ClassifierFile.open(QIODevice::ReadOnly);
  InputStream.readRawData(DataBuffer.data(), ClassifierFile.size());
  ClassifierFile.close();
  FileStorage.reset(new cv::FileStorage(DataBuffer.data(), cv::FileStorage::READ+cv::FileStorage::MEMORY));
  Classifier.reset(new CvRTrees);
  FileNode = FileStorage->getFirstTopLevelNode();
  Classifier->read(**FileStorage, *FileNode);
  // Set up the audio recording
  QAudioFormat Format;

  Format.setSampleRate(SampleRate);
  Format.setChannelCount(1);
  Format.setSampleSize(16);
  Format.setSampleType(QAudioFormat::SignedInt);
  Format.setByteOrder(QAudioFormat::LittleEndian);
  Format.setCodec("audio/pcm");
  AudioInput.reset(new QAudioInput(QAudioDeviceInfo::defaultInputDevice(), Format));
  fprintf(stderr, "Input device: %s\n", qPrintable(QAudioDeviceInfo::defaultInputDevice().deviceName()));
  AudioInput->setBufferSize(256);
  AudioInput->setNotifyInterval(30);
  Device = AudioInput->start();
  connect(AudioInput.get(), SIGNAL(notify()), this, SLOT(Update()));
  QTimer::singleShot(200, this, SLOT(Update()));
}


void SoundRecorder::Update()
{
  if (AudioInput->bytesReady() == 0 && BufferPos < SlidingWindowSize*2)
    return;

  int ReadBytes = Device->read((char*)&Buffer[BufferPos], AudioInput->bytesReady());

  BufferPos += ReadBytes;
  if (BufferPos >= SlidingWindowSize*2)
  {
    DoRecognition();
    memcpy(&Buffer[0], &Buffer[SlidingWindowSize], BufferPos-SlidingWindowSize);
    BufferPos -= SlidingWindowSize;
  }
}


void SoundRecorder::DoRecognition()
{
  // Convert the data to double
  std::vector<double> NewData;

  NewData.resize(SlidingWindowSize);
  for (int i = 0; i < SlidingWindowSize; ++i)
  {
    NewData[i] = (double)(((int)Buffer[i*2+1] << 8) | (int)Buffer[i*2]);
  }
  // Create Hann windows
  std::vector<std::vector<double> > Windows;
  double* HannWindow = NULL;

  HannWindow = xtract_init_window(HannWindowSize, XTRACT_HANN);
  // Create the Hann windows
  for (int i = 0; i <= (int)SlidingWindowSize-HannWindowSize; i += HannWindowSize / 3)
  {
    std::vector<double> Window;

    Window.resize(HannWindowSize);
    xtract_windowed((double*)&NewData[i], HannWindowSize, HannWindow, (double*)&Window[0]);
    Windows.push_back(Window);
  }
  xtract_free_window(HannWindow);
  // Generate the MFCC feature frames
  const int MfccCount = 12;
  double UserData[4] = { (double)SampleRate / HannWindowSize, XTRACT_MAGNITUDE_SPECTRUM, 0.0f, 0.0f};
  std::vector<std::vector<float> > FeatureTable;
  xtract_mel_filter* Pmfcc = (xtract_mel_filter*)malloc(sizeof(xtract_mel_filter));
  std::vector<std::vector<double> > MfccFrames;

  Pmfcc->n_filters = 25;
  Pmfcc->filters = (double**)malloc(Pmfcc->n_filters*sizeof(double*));
  for (int i = 0; i < Pmfcc->n_filters; ++i)
  {
    Pmfcc->filters[i] = (double*)malloc(HannWindowSize*sizeof(double));
  }
  xtract_init_fft(HannWindowSize, XTRACT_SPECTRUM);
  xtract_init_mfcc(HannWindowSize, SampleRate, XTRACT_EQUAL_AREA, 1, SampleRate,
                   Pmfcc->n_filters, Pmfcc->filters);

  // Generate the mfcc frames
  for (unsigned int i = 0; i < Windows.size(); ++i)
  {
    std::vector<double> Temp;
    std::vector<double> MfccCoefficients;
    std::vector<double> Spectrum(HannWindowSize, 0);

    Temp.resize(Pmfcc->n_filters);
    MfccCoefficients.resize(MfccCount);
    xtract_spectrum((double*)&(Windows[i][0]), HannWindowSize, UserData, (double*)&(Spectrum[0]));
    xtract_mfcc((double*)&(Spectrum[0]), HannWindowSize, Pmfcc, (double*)&Temp[0]);
    memcpy(&MfccCoefficients[0], &Temp[0], sizeof(double)*MfccCount);
    MfccFrames.push_back(MfccCoefficients);
  }
  for (unsigned int i = 0; i < Windows.size(); ++i)
  {
    std::vector<float> FinalVector;

    // Add MFCC components except the first to the feature vector
    for (unsigned int i1 = 1; i1 < MfccFrames[i].size(); ++i1)
    {
      FinalVector.push_back((float)MfccFrames[i][i1]);
    }
    FeatureTable.push_back(FinalVector);
  }
  free(Pmfcc);
  xtract_free_fft();
  // Do the recognition with majority voting
  std::vector<float> Labels;

  for (unsigned int i = 0; i < FeatureTable.size(); ++i)
  {
    // Convert the feature vector to OpenCV's version
    cv::Mat TestingSample(1, FeatureTable[0].size(), cv::DataType<float>::type);
    cv::Mat TestingLabel(1, 1, cv::DataType<float>::type);

    for (unsigned int i1 = 0; i1 < FeatureTable[0].size(); ++i1)
    {
      TestingSample.at<float>(0, i1) = FeatureTable[i][i1];
    }
//    PrintContainerAsFloats(FeatureTable[i]);
    TestingLabel.at<float>(0) = Classifier->predict(TestingSample, cv::Mat());
    Labels.push_back(TestingLabel.at<float>(0));
  }
  float Winner = GetMostFrequentItemFromContainer<float>(Labels);
  int Count = ItemCountInContainer(Labels, Winner);

  if (Winner == 1.0)
    printf("Ping (%d out of %d)\n", Count, Labels.size());
  if (Winner == 2.0)
    printf("Pong (%d out of %d)\n", Count, Labels.size());
  if (Winner == 3.0)
    printf("Talk (%d out of %d)\n", Count, Labels.size());
}
