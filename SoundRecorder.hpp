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

#include <qaudioinput.h>
#include <qaudiorecorder.h>
#include <qobject.h>
#include <QTime>
#include <qtimer.h>

#include <opencv/ml.h>

#include <boost/scoped_ptr.hpp>

#include <utility>

class CvStatModel;

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

protected:
  boost::scoped_ptr<QAudioInput> AudioInput;
  QIODevice* Device;
  QAudioRecorder AudioRecorder;
  boost::scoped_ptr<CvStatModel> ClassifierTree;
  boost::scoped_ptr<CvStatModel> ClassifierForest;
  unsigned char Buffer[100000];
  int BufferPos;
  QTime Timer;
  std::vector<char> WavBuffer;
  QObject* Page;
  QTimer ResultTextResetTimer;
};
