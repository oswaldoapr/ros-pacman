#include "pacman/window.h"


Window::Window(QStringList args)	//GED Jul-27: Se recibe QStringList args con  argumentos
{
    char **argv;
    int argc = 0;
    ros::init(argc, argv, "pacman_world");
    QString mapName;
    QWidget *w = new QWidget();
    QWidget *wScores = new QWidget();
    wScores->setFixedSize(scoreWidth, scoreHeight);
    maps = new Maps();
    
    QFont fontBold;
    fontBold.setBold(true);
    
    scoreName = new QLabel("Score: ");
    scoreName->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    scoreName->setFont(fontBold);
    scoreLabel = new QLabel();
    scoreLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    livesName = new QLabel("Lives: ");
    livesName->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    livesName->setFont(fontBold);
    livesLabel = new QLabel();
    livesLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    performValName = new QLabel("Performance: ");
    performValName->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    performValName->setFont(fontBold);
    performValLabel = new QLabel();
    performValLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    
    glWidget = new GLWidget();
    refreshTimer = new QTimer();
    mainLayout = new QVBoxLayout();
    container = new QHBoxLayout();
    containerScores = new QHBoxLayout();
    listenMsg = new ListenMsgThread();
    mapsList = new QComboBox();
    counterTimer = new QTimer();				//GED Jul-27
    playBtn = new QPushButton(tr("Play"));
    counterBtn = new QPushButton(tr("Waiting for initialization request"));	//GED Jul-27: PushButton label for reverse counter
    EndGameBtn1 = new QPushButton(tr("Game Over"));		//GED Ag-01
    EndGameBtn2 = new QPushButton(tr("Finished Test"));		//GED Ag-01
    gameTimeRemainingLCD = new QLCDNumber(4);
    gameTime = new QTime(0, initialGameTimeMins, initialGameTimeSecs);
    initSound = new QSound(tr(":/resources/audio/start.wav"));

    node = new ros::NodeHandle();    
    
    w->setLayout(container);
    wScores->setLayout(containerScores);
    mainLayout->addWidget(wScores);
    mainLayout->addWidget(w);
    counterBtn->setEnabled(false);
    container->addWidget(glWidget);
    containerScores->addWidget(scoreName);  
    containerScores->addWidget(scoreLabel);
    containerScores->addWidget(livesName);
    containerScores->addWidget(livesLabel);
    containerScores->addWidget(performValName);
    containerScores->addWidget(performValLabel);   
    containerScores->addWidget(gameTimeRemainingLCD);
    ListArrayMap(QString::fromStdString(ros::package::getPath("pacman")) + "/resources/layouts/");
    mode = getArguments(args);						//GED Jul-28
    allowPlay = false;
    gameState = false;
    gameTimeRemainingLCD->setSegmentStyle(QLCDNumber::Filled);
    QString time = gameTime->toString();
    gameTimeRemainingLCD->display(time);
    
    if(mode == 1)  							//GED Jul-27: if Game mode
    {
      mainLayout->addWidget(playBtn);
      mainLayout->addWidget(mapsList);
      mapName = mapsList->currentText();
      connect(this, SIGNAL(ArrowKey(int)), glWidget, SLOT(receiveArrowKey(int)));
    }
    else if(mode == 2)
    {
      mainLayout->addWidget(counterBtn);
      mapName = verifyMapArgument(args, mapsList, mode); 		//GED Jul-30
    }
    else
    {
      cout << "Error: Pacman mode argument missing" << endl;
      cout << "Usage: rosrun pacman pacman_world [pacmanMode] {opt:mapName}" << endl;
      cout << "     pacmanMode : --c" << endl;
      cout << "                : --g" << endl;
      cout << "     mapName    : file name of the map" << endl;
      exit(0);
    }
    
    connect(playBtn, &QPushButton::clicked, this, &Window::PlaySlot);
    connect(mapsList, SIGNAL(currentIndexChanged(QString)), maps, SLOT(CreateMap(QString)));
    connect(maps, SIGNAL(SendMapData(int, int, QImage*, bool*, QVector<int>*, QVector<int>*, QVector<int>*, QVector<int>*, QVector<int>*, int, int)), glWidget, SLOT(ReceiveMapDataGL(int, int, QImage*, bool*, QVector<int>*, QVector<int>*, QVector<int>*, QVector<int>*, QVector<int>*, int, int)));
    connect(maps, SIGNAL(SendMapData(int, int, QImage*, bool*, QVector<int>*, QVector<int>*, QVector<int>*, QVector<int>*, QVector<int>*, int, int)), this, SLOT(UpdateSizeSlot()));
    connect(refreshTimer, SIGNAL(timeout()), glWidget, SLOT(UpdateSimulationSlot()));
    connect(listenMsg, SIGNAL(UpdatePacmanCommand(int)), glWidget, SLOT(SetPacmanCommand(int)));
    connect(glWidget, SIGNAL(UpdatePacmanPos(QVector<QPoint>*)), this, SLOT(UpdatePacmanPosSlot(QVector<QPoint>*)));
    connect(glWidget, SIGNAL(UpdateGhostsPos(QVector<QPoint>*, bool*)), this, SLOT(UpdateGhostsPosSlot(QVector<QPoint>*, bool*)));
    connect(glWidget, SIGNAL(UpdateCookiesPos(QVector<QPoint>*)), this, SLOT(UpdateCookiesPosSlot(QVector<QPoint>*)));
    connect(glWidget, SIGNAL(UpdateBonusPos(QVector<QPoint>*)), this, SLOT(UpdateBonusPosSlot(QVector<QPoint>*)));
    connect(glWidget, SIGNAL(UpdateObstaclesPos(QVector<QPoint>*, int, int, int, int)), this, SLOT(UpdateObstaclesPosSlot(QVector<QPoint>*, int, int, int, int)));
    connect(glWidget, SIGNAL(DeadPacmanSignal()), this, SLOT(DeadPacmanSlot()));
    connect(glWidget, SIGNAL(EndOfDeadPacmanSignal()), this, SLOT(EndOfDeadPacmanSlot()));
    connect(glWidget, SIGNAL(UpdateScores(int, int)), this, SLOT(UpdateScoresSlot(int, int)));
    connect(counterTimer, SIGNAL(timeout()), this, SLOT(InitializeCounterTimerSlot()));	//GED Jul-27
    connect(glWidget, SIGNAL(updateGameState()), this, SLOT(UpdateGameStateSlot()));
    connect(glWidget, SIGNAL(EndGameSignal()), this, SLOT(EndGame()));	//GED Ag-01
    connect(glWidget, SIGNAL(SendMaxScore(int, int)), this, SLOT(ReceiveMaxValues(int, int)));
    
    setLayout(mainLayout);
    this->setMaximumSize(QSize(maxWidth, maxHeight));
    maps->CreateMap(mapName);
    subscriber = node->subscribe("pacmanActions", 100, &ListenMsgThread::callback, listenMsg);
    pacmanPublisher = node->advertise<pacman::pacmanPos>("pacmanCoord", 100);
    ghostPublisher = node->advertise<pacman::ghostsPos>("ghostsCoord",100);
    cookiesPublisher = node->advertise<pacman::cookiesPos>("cookiesCoord",100);
    bonusPublisher = node->advertise<pacman::bonusPos>("bonusCoord",100); 
    gameStatePublisher = node->advertise<pacman::game>("gameState",100);
    mapResponseServer = node->advertiseService("pacman_world", &Window::obsService, this);
    listenMsg->start();
    refreshTimer->start(refreshTimeMs);
}

QSize Window::sizeHint() const
{
    return QSize(maps->getWidth()+scoreWidth, maps->getHeight()+scoreHeight);
}

QSize Window::minimumSizeHint() const
{
    return QSize(150, 150);
}

int Window::getArguments(QStringList args)				//GED Jul-28
{
    int pacmanMode = 0;
    int pacmanGameMode = 1, pacmanChallengeMode = -1;
    
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it)//GED Jul-28
    {
      QString current = *it;
      QString pacmanModeG="--g";
      QString pacmanModeC="--c";
      pacmanGameMode = QString::compare(pacmanModeG, current, Qt::CaseInsensitive)*pacmanGameMode;
      pacmanChallengeMode = QString::compare(pacmanModeC, current, Qt::CaseInsensitive)*pacmanChallengeMode;
    }
    
    if(pacmanGameMode == 0)
      pacmanMode = 1; //GED Jul-28: Game mode
    else if(pacmanChallengeMode == 0)
      pacmanMode = 2; //GED Jul-28: Challenge mode
    else
      pacmanMode = 0;
    
    return pacmanMode;
}

void Window::InitializeCounterTimerSlot() 				//GED Jul-27
{
  QTime zeroTime;
  if (mode == 1)
  {
      *gameTime = gameTime->addMSecs(-oneSecondTimeMilisecs);
      QString time = gameTime->toString();
      gameTimeRemainingLCD->display(time);
  }
  else if (mode == 2)
  {
      counterBtn->setStyleSheet("background-color: black;"
                            "font: bold 28px;"
			    "color: yellow;"
			   );      
      if(counterBtn->text() == "Waiting for initialization request")
	  counterBtn->setText("Playing in... 3");
      else if(counterBtn->text() == "Playing in... 3")
	  counterBtn->setText("Playing in... 2");    
      else if(counterBtn->text() == "Playing in... 2")
	  counterBtn->setText("Playing in... 1");
      else if(counterBtn->text() == "Playing in... 1")
      {
	  counterBtn->setText("Play!");
	  listenMsg->setWorkingThread(true);
	  glWidget->TogglePlaying();
	  gameState = true;
      }
      else if (counterBtn->text() == "Play!")
      {
	  *gameTime = gameTime->addMSecs(-oneSecondTimeMilisecs);
	  QString time = gameTime->toString();
	  gameTimeRemainingLCD->display(time);
      }
  }
  
  if (*gameTime == QTime(0,0,0))
      EndGame();
}

void Window::keyPressEvent(QKeyEvent *e)
{
    if (allowPlay)
      emit ArrowKey(e->key());
}

void Window::ListArrayMap(QString path)
{
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QStringList list = dir.entryList();
    for(int i = 0; i < list.size(); i++)
      mapsList->addItem(list.at(i).split(".")[0]);
}

void Window::PlaySlot()
{
    if(playBtn->text() == "Play")
    {
      playBtn->setText("Stop");
      mapsList->setEnabled(false);
      listenMsg->setWorkingThread(true);
      gameState = true;
      allowPlay = true;
      initSound->play();
      counterTimer->start(oneSecondTimeMilisecs);
    }
    else
    {
      listenMsg->setWorkingThread(false);
      playBtn->setText("Play");
      maps->CreateMap(mapsList->currentText());
      mapsList->setEnabled(true);
      allowPlay = false;
      counterTimer->stop();
      gameTime->setHMS(0, initialGameTimeMins, initialGameTimeSecs);
      QString time = gameTime->toString();
      gameTimeRemainingLCD->display(time);
    }
    glWidget->TogglePlaying();
}
void Window::UpdatePacmanPosSlot(QVector<QPoint>* pos)
{
  msgPacman.pacmanPos.resize(pos->size());
  for(int i = 0; i < pos->size(); i++)
  {
    msgPacman.pacmanPos[i].x = pos->at(i).x(); 
    msgPacman.pacmanPos[i].y = pos->at(i).y();
  }
  msgPacman.nPacman = pos->size();
  pacmanPublisher.publish(msgPacman);
}
void Window::UpdateGhostsPosSlot(QVector<QPoint>* pos, bool* ghostsMode)
{
  msgGhosts.ghostsPos.resize(pos->size());
  msgGhosts.mode.resize(pos->size());
  for(int i = 0; i < pos->size(); i++)
  {
    msgGhosts.ghostsPos[i].x = pos->at(i).x(); 
    msgGhosts.ghostsPos[i].y = pos->at(i).y();
    msgGhosts.mode[i] = (int)ghostsMode[i];
  }
  msgGhosts.nGhosts = pos->size();
  ghostPublisher.publish(msgGhosts);
}
void Window::UpdateCookiesPosSlot(QVector<QPoint>* pos)
{
  msgCookies.cookiesPos.resize(pos->size());
  for(int i = 0; i < pos->size(); i++)
  {
    msgCookies.cookiesPos[i].x = pos->at(i).x();
    msgCookies.cookiesPos[i].y = pos->at(i).y();
  }
  msgCookies.nCookies = pos->size();
  cookiesPublisher.publish(msgCookies);
}
void Window::UpdateBonusPosSlot(QVector<QPoint>* pos)
{
  msgBonus.bonusPos.resize(pos->size());
  for(int i = 0; i < pos->size(); i++)
  {
    msgBonus.bonusPos[i].x = pos->at(i).x();
    msgBonus.bonusPos[i].y = pos->at(i).y();
  }
  msgBonus.nBonus = pos->size();
  bonusPublisher.publish(msgBonus);
}

void Window::UpdateObstaclesPosSlot(QVector< QPoint >* pos, int xMin, int xMax, int yMin, int yMax)
{
  posObstacles = pos;
  minX = xMin;
  maxX = xMax;
  minY = yMin;
  maxY = yMax;
}

void Window::UpdateSizeSlot()
{
    this->resize(sizeHint());
}

void Window::DeadPacmanSlot()
{
    refreshTimer->stop();
    gameState = false;
}

void Window::EndOfDeadPacmanSlot()
{
    refreshTimer->start(refreshTimeMs);
}

void Window::EndGame()	//GED Ag-01
{
    EndGameBtn1->setStyleSheet("background-color: black;"
                               "font: bold 28px;"
			       "color: yellow;"
			      );
    EndGameBtn2->setStyleSheet("background-color: black;"
                               "font: bold 28px;"
			       "color: yellow;"
			      );
    if(mode == 1)
    {
      mapsList->setVisible(false);
      playBtn->setVisible(false);
      mainLayout->addWidget(EndGameBtn1);
    }
    else if(mode == 2)
    {
      counterBtn->setVisible(false);
      mainLayout->addWidget(EndGameBtn2);
    }
    refreshTimer->stop();
    counterTimer->stop();
    listenMsg->setWorkingThread(false);
    glWidget->TogglePlaying();
    allowPlay = false;
}

QString Window::verifyMapArgument(QStringList args, QComboBox *mapsList, int pacmanMode)	//GED Jul-30
{
  QString mapArgument = "", Listmaps;
  bool mapFound = false;
  if(pacmanMode == 2)	//If test mode
  {
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) //GED Jul-30
    {
      QString current = *it;
      if(mapsList->findText(current) == -1 && !mapFound)
      {
      }
      else
      {
	mapArgument = mapsList->itemText(mapsList->findText(current));
	mapFound = true;
      }
    }      
    if(mapFound == false)
    {
      
      QDir dir(QString::fromStdString(ros::package::getPath("pacman")) + "/resources/layouts/");
      dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
      QStringList list = dir.entryList();
      cout<<"No valid argument for map"<<endl;
      cout << "Maps List:" << endl;
      for(int i = 0; i < list.size(); i++)
	cout << list.at(i).toLocal8Bit().constData() << " -- ";
      exit(0);
    }
  }
  return QString(mapArgument);
}

void Window::UpdateGameStateSlot()
{
  msgState.state = (int)gameState; 
  gameStatePublisher.publish(msgState);
}

void Window::UpdateScoresSlot(int score, int lives)
{
  scoreLabel->setText(QString::number(score));
  livesLabel->setText(QString::number(lives));
  
  int timeSec = QTime(0, 0, 0).secsTo(*gameTime);
  performEval =  (((double)score / (double)MAX_SCORE ) * wScore) + (((double)lives / (double)MAX_LIVES ) * wLives) + (((double)timeSec / (double)MAX_TIME_SEC ) * wTime);
  performEval *= 100;
  performValLabel->setText(QString::number(performEval, 'g', 4));
}

bool Window::obsService(pacman::mapService::Request& req, pacman::mapService::Response& res)
{
  res.minX = minX;
  res.maxX = maxX;
  res.minY = minY;
  res.maxY = maxY;
  res.nObs = posObstacles->size();
  res.obs.resize(posObstacles->size());
  for(int i = 0; i < posObstacles->size(); i++)
  {
    res.obs[i].x = posObstacles->at(i).x();
    res.obs[i].y = posObstacles->at(i).y();
  }
  return true;
}

void Window::ReceiveMaxValues(int maxScore, int maxLives)
{
  MAX_SCORE = maxScore;
  MAX_LIVES = maxLives;
  MAX_TIME_SEC = (initialGameTimeMins * 60) + initialGameTimeSecs;
}

/*bool Window::InitializeService(pacman::initializeService::Request& req, pacman::initializeService::Response &res)
{
    counterTimer->start(oneSecondTimeMilisecs);
    //Devolver alguna confirmación de que empieza el conteo?
    return true;
}*/