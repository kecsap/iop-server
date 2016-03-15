/*
 *  This file is part of the iop_sound_prototype
 *
 *  Copyright (C) 2015-2016 Csaba Kertész (csaba.kertesz@gmail.com)
 *
 *  iop_sound_prototype is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  iop_sound_prototype is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include <qaudioinput.h>
#include <qaudiorecorder.h>
#include <qobject.h>
#include <QTime>
#include <qtimer.h>

#include <ml/MAModel.hpp>
#include <sound/MASoundEventAnalyzer.hpp>

#include <MCBinaryData.hpp>

#include <boost/scoped_ptr.hpp>

#include <utility>

typedef std::pair<float, float> RecognitionResult;

class SoundRecorder : public QObject
{
  Q_OBJECT

public:
  SoundRecorder(const QString& file_name = "", QObject* root_object = NULL);

public Q_SLOTS:

  void Update();
  RecognitionResult DoRecognition();
  void StateMachine(RecognitionResult result, int timestamp);
  void ShowText(const QString& text, int duration = 500);
  void ResetResultText();

Q_SIGNALS:
  void PingHappened();
  void PongHappened();
  void TalkHappened();

protected:
  boost::scoped_ptr<QAudioInput> AudioInput;
  QIODevice* Device;
  QAudioRecorder AudioRecorder;
  boost::scoped_ptr<MAModel> ClassifierTree;
  boost::scoped_ptr<MAModel> ClassifierForest;
  boost::scoped_ptr<MAModel> ClassifierSvm;
  MASoundEventAnalyzer AudioAnalyzer;
  MC::DoubleList Buffer;
  int BufferPos;
  QTime Timer;
  MC::DoubleList WavBuffer;
  QObject* Page;
  QTimer ResultTextResetTimer;
};
