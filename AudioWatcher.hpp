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

#ifndef AudioWatcher_hpp
#define AudioWatcher_hpp

#include "Defines.hpp"

#include <sound/MASoundEventAnalyzer.hpp>

#include <qaudioinput.h>
#include <qaudiorecorder.h>
#include <qfuturewatcher.h>
#include <qobject.h>
#include <QTime>
#include <qtimer.h>

#include <boost/scoped_ptr.hpp>

class MAModel;

typedef std::pair<float, float> RecognitionResult;

class AudioWatcher : public QObject
{
  Q_OBJECT

public:
  AudioWatcher(const QString& audio_file);
  virtual ~AudioWatcher();

public Q_SLOTS:
  void StartPlayback();
  void AudioUpdate();
  RecognitionResult DoRecognition();
  void StateMachine(RecognitionResult result, int timestamp);

Q_SIGNALS:
  void AudioEvent(IOP::AudioEventType event);
  void Timestamp(int msec);

protected:
  QString AudioFile;
  QTime PlaybackClock;
  boost::scoped_ptr<QAudioInput> AudioInput;
  QIODevice* Device;
  QAudioRecorder AudioRecorder;
  boost::scoped_ptr<MAModel> ClassifierTree;
  boost::scoped_ptr<MAModel> ClassifierForest;
  boost::scoped_ptr<MAModel> ClassifierSvm;
  MASoundEventAnalyzer AudioAnalyzer;
  MC::DoubleList Buffer;
  int BufferPos;
  MC::DoubleList WavBuffer;
};

#endif
