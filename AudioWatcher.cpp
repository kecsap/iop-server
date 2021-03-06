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

#include "AudioWatcher.hpp"

#include <ml/MAModel.hpp>
#include <sound/MASoundData.hpp>

#include <MCContainers.hpp>
#include <MCDefs.hpp>
#include <MCSampleStatistics.hpp>

#include <qcoreapplication.h>
#include <qdatastream.h>
#include <qfile.h>
#include <qresource.h>
#include <qsound.h>
#include <QtConcurrentRun>

#include <boost/unordered_map.hpp>

namespace
{
const int SampleRate = 16000;
const int SlidingWindowSize = 2048;

QAudioDeviceInfo GetAudioDevice()
{
  QList<QAudioDeviceInfo> Devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  QAudioDeviceInfo FinalDevice;

  fprintf(stderr, "Input devices:\n");
  for (int i = 0; i < Devices.size(); ++i)
  {
    fprintf(stderr, "%d: %s\n", i, qPrintable(Devices[i].deviceName()));
    if (Devices[i].deviceName() == "alsa_input.usb-0d8c_C-Media_USB_Audio_Device-00-Device.analog-mono")
      return Devices[i];
  }
  return QAudioDeviceInfo::defaultInputDevice();
}


MAModel* GetClassifier(const QString& resource_str)
{
  MCBinaryData DataBuffer;

  DataBuffer.LoadFromQtResource(resource_str);
  return MAModel::Decode(DataBuffer);
}
}

AudioWatcher::AudioWatcher(const QString& audio_file) : AudioFile(audio_file), Device(NULL),
  AudioAnalyzer(SampleRate, 5), BufferPos(0)
{
  // Load the classifiers
  ClassifierTree.reset(GetClassifier(":/pingpongsound_dt.mdl"));
  ClassifierForest.reset(GetClassifier(":/pingpongsound_rt.mdl"));
  ClassifierSvm.reset(GetClassifier(":/pingpongsound_svmcdcd.mdl"));
  // Load wave data buffer
  if (!audio_file.isEmpty())
  {
    MC::DoubleList Temp;

    MASoundData::LoadFromFile(audio_file.toStdString(), WavBuffer, Temp, 0, 0);
    return;
  }
  // Set up the audio recording
  QAudioFormat Format;
  QAudioDeviceInfo SelectedDevice = GetAudioDevice();

  Format.setSampleRate(SampleRate);
  Format.setChannelCount(1);
  Format.setSampleSize(16);
  Format.setSampleType(QAudioFormat::SignedInt);
  Format.setByteOrder(QAudioFormat::LittleEndian);
  Format.setCodec("audio/pcm");
  AudioInput.reset(new QAudioInput(SelectedDevice, Format));
  fprintf(stderr, "Input device: %s\n", qPrintable(SelectedDevice.deviceName()));
  AudioInput->setBufferSize(256);
  Device = AudioInput->start();
  QTimer::singleShot(200, this, SLOT(AudioUpdate()));
}


AudioWatcher::~AudioWatcher()
{
}


void AudioWatcher::StartPlayback()
{
  if (!PlaybackClock.isValid())
    PlaybackClock.start();

  QSound::play(AudioFile);
  AudioUpdate();
}

void AudioWatcher::AudioUpdate()
{
  if (!AudioFile.isEmpty())
    Q_EMIT(Timestamp(PlaybackClock.elapsed()));
  if (WavBuffer.size() > 0)
  {
    int StartPosCorrection = (BufferPos > 0 ? SlidingWindowSize / 2 : 0);

    Buffer = MC::DoubleList(WavBuffer.begin()+BufferPos-StartPosCorrection, WavBuffer.begin()+BufferPos+SlidingWindowSize);
    BufferPos += SlidingWindowSize;
    StateMachine(DoRecognition(), (int)((float)BufferPos / 16000*1000));
    if ((int)WavBuffer.size()-BufferPos < SlidingWindowSize)
    {
      QCoreApplication::quit();
    }
    // Keep the buffer processing in sync with the sound playback
    int WaitTime = PlaybackClock.elapsed() < (int)((float)BufferPos / 16000*1000) ? 100 : 90;

//    printf("%d ? %d (diff: %d - wait: %d)\n", PlaybackClock.elapsed(), (int)((float)BufferPos / 16000*1000), 
//           PlaybackClock.elapsed()-(int)((float)BufferPos / 16000*1000), WaitTime);
    QTimer::singleShot(WaitTime, this, SLOT(AudioUpdate()));
    return;
  }

  if (!PlaybackClock.isValid())
    PlaybackClock.start();

  MCBinaryData RawDataBuffer(50000);
  MCBinaryData RawData;
  int Pos = 0;
  int ReadBytes = 10;

  while (ReadBytes > 0)
  {
    ReadBytes = Device->read((char*)&RawDataBuffer.GetData()[Pos], 2048);
//  if (ReadBytes > 0)
//    printf("Pos: %d - Bytes: %d\n", Pos, ReadBytes);
    Pos += ReadBytes;
  }
  if (Pos == 0)
  {
    QTimer::singleShot(30, this, SLOT(AudioUpdate()));
    return;
  }
  RawData.Allocate(Pos);
  memcpy(RawData.GetData(), RawDataBuffer.GetData(), Pos);
  MCMergeContainers(Buffer, MASoundData::ConvertToDouble(RawData));
  if ((int)Buffer.size() >= SlidingWindowSize)
  {
    StateMachine(DoRecognition(), PlaybackClock.elapsed());
    Buffer.erase(Buffer.begin(), Buffer.begin()+SlidingWindowSize);
    BufferPos += SlidingWindowSize;
  }
  QTimer::singleShot(30, this, SLOT(AudioUpdate()));
}


RecognitionResult AudioWatcher::DoRecognition()
{
  // Convert the data to double
  double Power = MCCalculateVectorStatistic(Buffer, *new MCPower<double>)*100000;

  if (Power < 50)
    return RecognitionResult(1.0, 1.0);
//  printf("Power: %1.12f\n", Power);

  AudioAnalyzer.AddSoundData(Buffer);
  MC::FloatTable FeatureVectors = AudioAnalyzer.GetFeatureVectors();
  MC::FloatList Labels, Confidences;

  FeatureVectors = MAAnalyzer::CompactFeatureVectors(FeatureVectors, 5);
//  Labels = ClassifierTree->Predict(FeatureVectors, Confidences);
//  Labels = ClassifierForest->Predict(FeatureVectors, Confidences);
  Labels = ClassifierSvm->Predict(FeatureVectors, Confidences);

  float Winner = MCGetMostFrequentItemFromContainer<float>(Labels);
  int Count = MCItemCountInContainer(Labels, Winner);
  RecognitionResult Result(Winner, (float)Count / Labels.size());

//  if (Result.second < 0.3)
//    return RecognitionResult(1.0, 1.0);

  std::string Prefix;

  if (WavBuffer.size() > 0)
    Prefix = MCToStr<int>(BufferPos / 16)+" ms: ";
  if (Winner == 2.0)
  {
    printf("%sPing (%d out of %d) - Power %1.2f\n", Prefix.c_str(), Count, (int)Labels.size(), Power);
    Q_EMIT(AudioEvent(IOP::PingEvent));
  }
  if (Winner == 3.0)
  {
    printf("%sPong (%d out of %d) - Power %1.2f\n", Prefix.c_str(), Count, (int)Labels.size(), Power);
    Q_EMIT(AudioEvent(IOP::PongEvent));
  }
  if (Winner == 4.0)
  {
    printf("%sTalk (%d out of %d) - Power %1.2f\n", Prefix.c_str(), Count, (int)Labels.size(), Power);
    Q_EMIT(AudioEvent(IOP::TalkEvent));
  }

  return Result;
}


void AudioWatcher::StateMachine(RecognitionResult result, int timestamp)
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
//      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    // Evaluate the match when the points are equal
    if (PingCount > 2 && PingCount % 2 == 1 && PingCount == PongCount)
    {
//      ShowText("Point for Server", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 0 && PingCount == PongCount)
    {
//      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    // Evaluate the match when the points are NOT equal
    if (PingCount > 2 && PingCount % 2 == 1 && PingCount != PongCount)
    {
//      ShowText("Point for Server", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 0 && PingCount != PongCount)
    {
//      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
//    printf("CAN'T DETERMINATE THE CAUSE\n");
  }
  // Check the talk periods
  if (result.first == 4.0)
  {
    TalkCounter++;
    if (TalkCounter > 3)
    {
      TalkCounter = 0;
      printf("%d ms: Bullshit\n", timestamp);
//      ShowText("Bullshit");
    }
  } else {
    TalkCounter = 0;
  }
  // Check the ping periods to skip false alarms
  if (result.first == 2.0 && timestamp-LastPingTimestamp > 100)
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
//        ShowText("Ping");
      }
      PongCount++;
      LastPongTimestamp = timestamp;
      printf("%d ms: Pong\n", PongEventStart);
//      ShowText("Pong");
      PongEventChances.clear();
    } else {
      PongEventChances.clear();
    }
  }
  // PING
  if (result.first != 2.0 && result.second > 0.4 && PingEventChances.size() > 0)
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
//    ShowText("Ping");
    // The previous is the last valid ping in the case when the previous was less than 300 msec ago.
    if (timestamp-LastPingTimestamp < 300)
    {
      if (PingCount > 2 && PingCount % 2 == 1 && PingCount != PongCount)
      {
//        ShowText("Point for Opponent", 2000);
        goto cleanup;
      }
      if (PingCount > 2 && PingCount % 2 == 0 && PingCount != PongCount)
      {
//        ShowText("Point for Server", 2000);
        goto cleanup;
      }
    }
    PingCount++;
    LastPingTimestamp = timestamp;
    if (PingCount == 2 && PongCount != 2)
    {
//      ShowText("Point for Opponent", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 1 && PingCount != PongCount)
    {
//      ShowText("Point for Server", 2000);
      goto cleanup;
    }
    if (PingCount > 2 && PingCount % 2 == 0 && PingCount != PongCount)
    {
//      ShowText("Point for Opponent", 2000);
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
