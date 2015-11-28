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
#include <qsound.h>
#include <qtimer.h>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>

#include <libxtract.h>
#include <WaveFile.h>

#include <boost/unordered_map.hpp>

const int SampleRate = 16000;
const int HannWindowSize = 512;
const int SlidingWindowSize = 2048;

template <typename T>
std::string ToStr(const T value, bool hex_manipulator = false)
{
  std::stringstream Tmp;

  if (hex_manipulator)
    Tmp << std::hex;
  Tmp << value;
  return Tmp.str();
}


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


CvStatModel* GetClassifier(const QString& resource_str)
{
  CvStatModel* Classifier = NULL;
  boost::scoped_ptr<cv::FileStorage> FileStorage;
  QResource XmlResource(resource_str);
  QFile ClassifierFile(XmlResource.absoluteFilePath());
  QDataStream InputStream(&ClassifierFile);
  std::vector<char> DataBuffer(ClassifierFile.size(), 0);
  cv::FileNode FileNode;

  ClassifierFile.open(QIODevice::ReadOnly);
  InputStream.readRawData(DataBuffer.data(), ClassifierFile.size());
  ClassifierFile.close();
  FileStorage.reset(new cv::FileStorage(DataBuffer.data(), cv::FileStorage::READ+cv::FileStorage::MEMORY));
  if (resource_str.contains("rt"))
  {
    Classifier = new CvRTrees;
  } else {
    Classifier = new CvDTree;
  }
  FileNode = FileStorage->getFirstTopLevelNode();
  Classifier->read(**FileStorage, *FileNode);
  return Classifier;
}

SoundRecorder::SoundRecorder(const QString& file_name, QObject* root_object) : Device(NULL), BufferPos(0),
  Page(NULL)
{
  if (root_object)
    Page = root_object->findChild<QObject*>("fuckYoo");
  // Load the classifiers
  ClassifierTree.reset(GetClassifier(":/dt.xml"));
  ClassifierForest.reset(GetClassifier(":/rt.xml"));
  // Load wave data buffer
  if (!file_name.isEmpty())
  {
    WaveFile AudioFile(file_name.toStdString());

    if (AudioFile.GetBitsPerSample() != 16 || (int)AudioFile.GetAudioFormat() != 1 ||
        AudioFile.GetNumChannels() != 1 || AudioFile.GetSampleRate() != 16000)
    {
      printf("Only 16 bit mono PCM Wave files with 16 kHz sample rate are supported (file: %s)!", qPrintable(file_name));
      exit(1);
    }
    WavBuffer.resize(AudioFile.GetDataSize());
    memcpy(WavBuffer.data(), AudioFile.GetData(), (int)AudioFile.GetDataSize());
    QSound::play(file_name);
    Update();
    connect(&ResultTextResetTimer, SIGNAL(timeout()), this, SLOT(ResetResultText()));
    ResultTextResetTimer.setSingleShot(true);
    return;
  }
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
//  AudioInput->setNotifyInterval(30);
  Device = AudioInput->start();
  connect(AudioInput.get(), SIGNAL(notify()), this, SLOT(Update()));
  QTimer::singleShot(200, this, SLOT(Update()));
}


void SoundRecorder::Update()
{
  if (WavBuffer.size() > 0)
  {
    memcpy(Buffer, &WavBuffer.data()[BufferPos], SlidingWindowSize*2);
    BufferPos += SlidingWindowSize;
    StateMachine(DoRecognition(), (int)((float)BufferPos / 32));
    if (WavBuffer.size()-BufferPos < SlidingWindowSize*2)
    {
      exit(0);
    }
    QTimer::singleShot(64, this, SLOT(Update()));
    return;
  }
  if (AudioInput->bytesReady() == 0 && BufferPos < SlidingWindowSize*2)
    return;

  if (!Timer.isValid())
    Timer.start();
  while (AudioInput->bytesReady() != 0)
  {
//    printf("%d\n", AudioInput->bytesReady());
    int ReadBytes = Device->read((char*)&Buffer[BufferPos], AudioInput->bytesReady());

    BufferPos += ReadBytes;
  }
  if (BufferPos >= SlidingWindowSize*2)
  {
    StateMachine(DoRecognition(), Timer.elapsed());

    memcpy(&Buffer[0], &Buffer[SlidingWindowSize], BufferPos-SlidingWindowSize);
    BufferPos -= SlidingWindowSize;
  }
  QTimer::singleShot(30, this, SLOT(Update()));
}


RecognitionResult SoundRecorder::DoRecognition()
{
  // Convert the data to double
  std::vector<double> NewData;
  double Power = 0;

  NewData.resize(SlidingWindowSize);
  for (int i = 0; i < SlidingWindowSize; ++i)
  {
    NewData[i] = (double)(((int)Buffer[i*2+1] << 8) | (int)Buffer[i*2]);
    Power += (NewData[i]*NewData[i]) / SlidingWindowSize / SlidingWindowSize;
  }
  if (Power / 1000 < 100)
    return RecognitionResult(1.0, 1.0);
//  printf("Power: %1.2f\n", Power / 1000);
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
    TestingLabel.at<float>(0) = dynamic_cast<CvRTrees*>(ClassifierForest.get())->predict(TestingSample, cv::Mat());
//    TestingLabel.at<float>(0) = dynamic_cast<CvDTree*>(ClassifierTree.get())->predict(TestingSample, cv::Mat())->value;
    Labels.push_back(TestingLabel.at<float>(0));
  }
  float Winner = GetMostFrequentItemFromContainer<float>(Labels);
  int Count = ItemCountInContainer(Labels, Winner);
  RecognitionResult Result(Winner, (float)Count / Labels.size());


  std::string Prefix;

  if (WavBuffer.size() > 0)
    Prefix = ToStr<int>(BufferPos / 32)+" ms: ";
  if (Winner == 2.0)
    printf("%sPing (%d out of %d) - Power %1.2f\n", Prefix.c_str(), Count, (int)Labels.size(), Power / 1000);
  if (Winner == 3.0)
    printf("%sPong (%d out of %d) - Power %1.2f\n", Prefix.c_str(), Count, (int)Labels.size(), Power / 1000);
  if (Winner == 4.0)
    printf("%sTalk (%d out of %d) - Power %1.2f\n", Prefix.c_str(), Count, (int)Labels.size(), Power / 1000);

  return Result;
}


void SoundRecorder::StateMachine(RecognitionResult result, int timestamp)
{
  static int LastPingTimestamp = 0.0;
  static int LastPongTimestamp = 0.0;
  static int TalkCounter = 0;
  static int PingCount = 0;
  static std::vector<float> PingEventChances;
  static int PongCount = 0;
  static bool PongHappened = false;
  static int PongEventStart = 0;
  static std::vector<float> PongEventChances;
  static int LastPointTimestamp = -1;

  // Give 5 seconds automatic pause after a won point
  if (LastPointTimestamp > -1)
  {
    if (LastPointTimestamp+5000 > timestamp)
    {
      return;
    } else {
      LastPointTimestamp = -1;
    }
  }
  // Check the winner after a longer "silent" period
  if (LastPingTimestamp > 0 && LastPongTimestamp > 0 &&
      timestamp-LastPingTimestamp > 2000 && timestamp-LastPongTimestamp > 2000)
  {
    // Incorrect starting serve
    if (PingCount == 1 && PongCount != 2)
    {
      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    // Evaluate the match when the points are equal
    if (PingCount > 2 && PingCount % 2 == 1 && PingCount == PongCount)
    {
      ShowText("Point for Server", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 0 && PingCount == PongCount)
    {
      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    // Evaluate the match when the points are NOT equal
    if (PingCount > 2 && PingCount % 2 == 1 && PingCount != PongCount)
    {
      ShowText("Point for Server", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 0 && PingCount != PongCount)
    {
      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    printf("CAN'T DETERMINATE THE CAUSE\n");
  }
  // Check the talk periods
  if (result.first == 4.0)
  {
    TalkCounter++;
    if (TalkCounter > 3)
    {
      TalkCounter = 0;
      printf("%d ms: Bullshit\n", timestamp);
      ShowText("Bullshit");
    }
  } else {
    TalkCounter = 0;
  }
  // Check the ping periods to skip false alarms
  if (result.first == 2.0)
  {
    PingEventChances.push_back(result.second);
  }
  // Check the pong periods to skip false alarms
  PongHappened = false;
  if (result.first == 3.0)
  {
    if (PongEventChances.size() == 0)
    {
      PongEventStart = timestamp;
    }
    if (PongEventChances.size() > 2)
    {
      // Find a peak
      int PeakIndex = -1;

      for (unsigned int i = 1; i < PongEventChances.size()-1; ++i)
      {
        if (PongEventChances[i-1] < PongEventChances[i] && PongEventChances[i] > PongEventChances[i+1])
        {
          PeakIndex = i;
          break;
        }
      }
      if (PeakIndex > -1)
      {
        if (PongEventChances[PongEventChances.size()-1] < result.second)
        {
          // Peak has not been found, but the new chance is higher than the previous
          PongHappened = true;
          PongEventChances.push_back(result.second);
        } else {
          // Peak was found and the chances have not hit a minimum yet
          PongEventChances.push_back(result.second);
        }
      } else {
        if (PongEventChances[PongEventChances.size()-1] < result.second)
        {
          // Peak has not been found, but the new chance is higher than the previous
          PongHappened = true;
          PongEventChances.push_back(result.second);
        } else {
          // Peak has not been found and the chances have not hit a minimum yet
          PongEventChances.push_back(result.second);
        }
      }
    } else {
      PongEventChances.push_back(result.second);
    }
  } else {
    if (PongEventChances.size() > 0)
      PongHappened = true;
  }
  // PONG
  if (PongHappened && (PingCount < 2 || timestamp-LastPingTimestamp < 1500))
  {
    bool RealPong = false;

    for (unsigned int i = 0; i < PongEventChances.size(); ++i)
    {
      if (PongEventChances[i] > 0.5)
      {
        RealPong = true;
        break;
      }
    }
    if (RealPong)
    {
      // Count the missed "ping" event in the start
      if (PingCount == 0 && PongCount == 0)
      {
        PingCount = 1;
        LastPingTimestamp = PongEventStart;
        printf("%d ms: Ping (points - 1:0)!\n", timestamp);
        ShowText("Ping");
      }
      PongCount++;
      LastPongTimestamp = timestamp;
      printf("%d ms: Pong\n", PongEventStart);
      ShowText("Pong");
      PongEventChances.clear();
    } else {
      PongEventChances.clear();
    }
  }
  // PING
  if (result.first != 2.0 && PingEventChances.size() > 0)
  {
    bool RealPing = false;

    for (unsigned int i = 0; i < PingEventChances.size(); ++i)
    {
      if (PingEventChances[i] > 0.4)
      {
        RealPing = true;
        break;
      }
    }
    // Skip false alarms
    if (!RealPing || PingEventChances.size() > 4 || timestamp-LastPingTimestamp > 2000)
    {
      PingEventChances.clear();
      return;
    }
    PingEventChances.clear();
    printf("%d ms: Ping (points %d:%d)!\n", timestamp, PingCount+1, PongCount);
    ShowText("Ping");
    // The previous is the last valid ping in the case when the previous was less than 300 msec ago.
    if (timestamp-LastPingTimestamp < 300)
    {
      if (PingCount > 2 && PingCount % 2 == 1 && PingCount != PongCount)
      {
        ShowText("Point for Opponent", 2000);
        goto cleanup;
      }
      if (PingCount > 2 && PingCount % 2 == 0 && PingCount != PongCount)
      {
        ShowText("Point for Server", 2000);
        goto cleanup;
      }
    }
    PingCount++;
    LastPingTimestamp = timestamp;
    if (PingCount == 2 && PongCount != 2)
    {
      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 1 && PingCount != PongCount)
    {
      ShowText("Point for Server", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 0 && PingCount != PongCount)
    {
      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
  }
  return;
cleanup:
  LastPingTimestamp = 0.0;
  LastPongTimestamp = 0.0;
  TalkCounter = 0;
  PingCount = 0;
  PingEventChances.clear();
  PongCount = 0;
  PongHappened = false;
  PongEventStart = 0;
  PongEventChances.clear();
  LastPointTimestamp = timestamp;
}


void SoundRecorder::ShowText(const QString& text, int duration)
{
  printf("Show: %s\n", qPrintable(text));
  if (Page)
  {
    Page->setProperty("resultText", text);
    ResultTextResetTimer.start(duration);
  }
}


void SoundRecorder::ResetResultText()
{
  if (Page)
    Page->setProperty("resultText", "");
}
