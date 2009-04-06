#include "../include/config.h"
#include "../include/GameStateDuel.h"
#include "../include/GameOptions.h"
#include "../include/utils.h"
#include "../include/AIPlayer.h"
#include "../include/AIMomirPlayer.h"
#include "../include/PlayerData.h"
#include "../include/DeckStats.h"
#include "../include/MTGRules.h"

#ifdef TESTSUITE
#include "../include/TestSuiteAI.h"
#endif

enum ENUM_DUEL_STATE
  {
    DUEL_STATE_START,
    DUEL_STATE_END,
    DUEL_STATE_CHOOSE_DECK1,
    DUEL_STATE_CHOOSE_DECK1_TO_2,
    DUEL_STATE_CHOOSE_DECK2,
    DUEL_STATE_CHOOSE_DECK2_TO_PLAY,
    DUEL_STATE_ERROR_NO_DECK,
    DUEL_STATE_CANCEL,
    DUEL_STATE_PLAY,
    DUEL_STATE_BACK_TO_MAIN_MENU,
    DUEL_STATE_MENU
  };

enum ENUM_DUEL_MENUS
  {
    DUEL_MENU_GAME_MENU,
    DUEL_MENU_CHOOSE_DECK,
    DUEL_MENU_CHOOSE_OPPONENT
  };


GameStateDuel::GameStateDuel(GameApp* parent): GameState(parent) {
  for (int i = 0; i<2; i ++){
    deck[i]=NULL;
    mPlayers[i]=NULL;
  }

  game = NULL;
  deckmenu = NULL;
  opponentMenu = NULL;
  opponentMenuFont = NULL;
  menu = NULL;
#ifdef TESTSUITE
  testSuite = NULL;
#endif
  showMsg = 0;
  unlockedTex = NULL;
  unlockedQuad = NULL;
}

GameStateDuel::~GameStateDuel() {
  End();
}

void GameStateDuel::Start()
{
  JRenderer * renderer = JRenderer::GetInstance();
  renderer->ResetPrivateVRAM();
  renderer->EnableVSync(true);


#ifdef TESTSUITE
  if (testSuite) delete testSuite;
  testSuite = NEW TestSuite(RESPATH"/test/_tests.txt",mParent->collection);
#endif


  mGamePhase = DUEL_STATE_CHOOSE_DECK1;

  mFont = GameApp::CommonRes->GetJLBFont(Constants::MENU_FONT);
  mFont->SetBase(0);	
  opponentMenuFont = mFont;

  unlocked = -1;


  menu = NEW SimpleMenu(DUEL_MENU_GAME_MENU, this, mFont, SCREEN_WIDTH/2-100, 25);
  menu->Add(12,"Back to main menu");
  menu->Add(13, "Cancel");

  int decksneeded = 0;


  for (int i = 0; i<2; i ++){
    if (mParent->players[i] ==  PLAYER_TYPE_HUMAN){
      if (!deckmenu){
	      decksneeded = 1;
	      deckmenu = NEW SimpleMenu(DUEL_MENU_CHOOSE_DECK, this, mFont, 35, 25, "Choose a Deck");
	      char buffer[100];
	      for (int j=1; j<6; j++){
	        sprintf(buffer, RESPATH"/player/deck%i.txt",j);
	        std::ifstream file(buffer);
	        if(file){
	          deckmenu->Add(j, GameState::menuTexts[j]);
	          file.close();
	          decksneeded = 0;
	        }
	      }
      }
    }
  }

  if (decksneeded)
    mGamePhase = DUEL_STATE_ERROR_NO_DECK;
}


void GameStateDuel::loadPlayerMomir(int playerId, int isAI){
  char * deckFile = RESPATH"/player/momir.txt";
  char * deckFileSmall = "momir";
  MTGDeck * tempDeck = NEW MTGDeck(deckFile, NULL, mParent->collection);
  deck[playerId] = NEW MTGPlayerCards(mParent->collection,tempDeck);
  if (!isAI){ //Human Player
      mPlayers[playerId] = NEW HumanPlayer(deck[playerId],deckFileSmall);
  }else{
      mPlayers[playerId] = NEW AIMomirPlayer(deck[playerId],deckFile,"");
  }
  delete tempDeck;
}

void GameStateDuel::loadPlayer(int playerId, int decknb, int isAI){
  if (decknb){
    if (!isAI){ //Human Player
      char deckFile[255];
      sprintf(deckFile, RESPATH"/player/deck%i.txt",decknb);
      char deckFileSmall[255];
      sprintf(deckFileSmall, "player_deck%i",decknb);
      //int deck_cards_ids[100];
      //int nb_elements = readfile_to_ints(deckFile, deck_cards_ids);
      MTGDeck * tempDeck = NEW MTGDeck(deckFile, NULL, mParent->collection);
      deck[playerId] = NEW MTGPlayerCards(mParent->collection,tempDeck);
      delete tempDeck;
      mPlayers[playerId] = NEW HumanPlayer(deck[playerId],deckFileSmall);
    }else{ //AI Player, chose deck
          AIPlayerFactory playerCreator;
          mPlayers[playerId] = playerCreator.createAIPlayer(mParent->collection,NULL,decknb);
          deck[playerId] = mPlayers[playerId]->game;
    }
  }else{
    AIPlayerFactory playerCreator;
    mPlayers[playerId] = playerCreator.createAIPlayer(mParent->collection,NULL);
    deck[playerId] = mPlayers[playerId]->game;
  }
}

#ifdef TESTSUITE
void GameStateDuel::loadTestSuitePlayers(){
  OutputDebugString ("loading suite 1\n");
  if (!testSuite) return;
  for (int i = 0; i < 2; i++){
    if (mPlayers[i]){
      delete mPlayers[i];
    }
    mPlayers[i] = NEW TestSuiteAI(testSuite, i);
    OutputDebugString ("loading suite 2\n");
    deck[i] = mPlayers[i]->game;
  }
  mParent->gameType = testSuite->gameType;
  if (game) delete game;
  game = NULL;
  if (!game){
    GameObserver::Init(mPlayers, 2);
    OutputDebugString ("loading suite 3\n");
    game = GameObserver::GetInstance();
    OutputDebugString ("loading suite 4\n");
    game->startGame(0,0);
    OutputDebugString ("loading suite 5\n");

    if (mParent->gameType == GAME_TYPE_MOMIR){
      game->addObserver(NEW MTGMomirRule(-1, mParent->collection));
      for (int i = 0; i < 2; i++){
        game->players[i]->life+=4;
      }
    }
  }
}
#endif

void GameStateDuel::End()
{
#if defined (WIN32) || defined (LINUX)
  OutputDebugString("Ending GamestateDuel\n");
#endif

  SAFE_DELETE(deckmenu);
  JRenderer::GetInstance()->EnableVSync(false);
  if (mPlayers[0] && mPlayers[1]) mPlayers[0]->End();
  GameObserver::EndInstance();
  game = NULL;

  for (int i = 0; i < 2; i++){
    SAFE_DELETE(mPlayers[i]);
    SAFE_DELETE(deck[i]);
  }

  SAFE_DELETE(unlockedQuad);
  SAFE_DELETE(unlockedTex);
  SAFE_DELETE(menu);
  SAFE_DELETE(opponentMenu);
#ifdef TESTSUITE
  SAFE_DELETE(testSuite);
#endif
}


void GameStateDuel::Update(float dt)
{
  switch (mGamePhase)
    {
    case DUEL_STATE_ERROR_NO_DECK:
      if (PSP_CTRL_CIRCLE == mEngine->ReadButton())
	mParent->SetNextState(GAME_STATE_DECK_VIEWER);
      break;
    case DUEL_STATE_CHOOSE_DECK1:
      if (mParent->gameType == GAME_TYPE_MOMIR){
        for (int i = 0; i < 2; i++){
          int isAI = 1;
          if (mParent->players[i] ==  PLAYER_TYPE_HUMAN) isAI = 0;
          loadPlayerMomir(i, isAI);
        }
        mGamePhase = DUEL_STATE_PLAY;
      }else if (mParent->players[0] ==  PLAYER_TYPE_HUMAN)
	      deckmenu->Update(dt);
#ifdef TESTSUITE
      else if (mParent->players[1] ==  PLAYER_TYPE_TESTSUITE){
	      if (testSuite && testSuite->loadNext()){

	        loadTestSuitePlayers();

	        mGamePhase = DUEL_STATE_PLAY;
	        testSuite->initGame();
	        char buf[4096];
	        sprintf(buf, "nb cards in player2's graveyard : %i\n",mPlayers[1]->game->graveyard->nb_cards);
	        LOG(buf);
	      }else{
	        mGamePhase = DUEL_STATE_END;
	      }
      }
#endif
      else{
	      loadPlayer(0);
	      mGamePhase = DUEL_STATE_CHOOSE_DECK2;
      }
      break;
    case DUEL_STATE_CHOOSE_DECK1_TO_2:
      if (deckmenu->closed) mGamePhase = DUEL_STATE_CHOOSE_DECK2;
      else deckmenu->Update(dt);
      break;
    case DUEL_STATE_CHOOSE_DECK2:
      if (mParent->players[1] ==  PLAYER_TYPE_HUMAN){
	deckmenu->Update(dt);
      }
      else{
	if (mParent->players[0] ==  PLAYER_TYPE_HUMAN){
	  if (!opponentMenu){
	    opponentMenu = NEW SimpleMenu(DUEL_MENU_CHOOSE_OPPONENT, this, opponentMenuFont, 35, 25, "Choose Opponent");
	    opponentMenu->Add(0,"Random");
	    nbAIDecks = 0;
	    int found = 1;
	    while (found){
	      found = 0;
	      char buffer[512];
	      char aiSmallDeckName[512];
	      char deckDesc[512];
	      sprintf(buffer, RESPATH"/ai/baka/deck%i.txt",nbAIDecks+1);
	      if(fileExists(buffer)){
		found = 1;
		nbAIDecks++;
		sprintf(aiSmallDeckName, "ai_baka_deck%i",nbAIDecks);
		DeckStats * stats = DeckStats::GetInstance();
		stats->load(mPlayers[0]);
		int percentVictories = stats->percentVictories(string(aiSmallDeckName));
		string difficulty;
		if (percentVictories < 34){
		  difficulty = "(hard)";
		}else if (percentVictories < 67){
		  difficulty = "";
		}else{
		  difficulty = "(easy)";
		}
		sprintf(deckDesc, "Deck %i %s",nbAIDecks, difficulty.c_str());
		opponentMenu->Add(nbAIDecks,deckDesc);
	      }
	    }
	  }
	  opponentMenu->Update(dt);
	}else{
	  loadPlayer(1);
	  mGamePhase = DUEL_STATE_PLAY;
	}
      }
      break;
    case DUEL_STATE_CHOOSE_DECK2_TO_PLAY:
      if (mParent->players[1] ==  PLAYER_TYPE_HUMAN){
	if (deckmenu->closed) mGamePhase = DUEL_STATE_PLAY;
	else deckmenu->Update(dt);
      }
      else
	{
	  if (opponentMenu->closed) mGamePhase = DUEL_STATE_PLAY;
	  else opponentMenu->Update(dt);
	}
      break;
    case DUEL_STATE_PLAY:
    //Stop the music before starting the game
      if (GameApp::music){
	      JSoundSystem::GetInstance()->StopMusic(GameApp::music);
	      SAFE_DELETE(GameApp::music);
      }
      if (!game){
	      GameObserver::Init(mPlayers, 2);
	      game = GameObserver::GetInstance();
	      game->startGame();
        if (mParent->gameType == GAME_TYPE_MOMIR){
          game->addObserver(NEW MTGMomirRule(-1, mParent->collection));
          for (int i = 0; i < 2; i++){
            game->players[i]->life+=4;
          }
        }
      }
      game->Update(dt);
      if (game->gameOver){
      showMsg = (rand() % 5);
	if (!mPlayers[0]->isAI() && mPlayers[1]->isAI() && mPlayers[0]!= game->gameOver){
	  PlayerData * playerdata = NEW PlayerData(mParent->collection);
	  playerdata->credits+= 500;
	  playerdata->save();
	  delete playerdata;
    if (unlocked == -1){
      unlocked = isDifficultyUnlocked();
      if (unlocked){
        unlockedTex = JRenderer::GetInstance()->LoadTexture("graphics/unlocked.png", TEX_TYPE_USE_VRAM); 
        unlockedQuad = NEW JQuad(unlockedTex, 2, 2, 396, 96);
        GameOptions::GetInstance()->values[OPTIONS_DIFFICULTY_MODE_UNLOCKED] = GameOption(1);
        GameOptions::GetInstance()->save();
      }else{
        unlocked = isMomirUnlocked();
        if (unlocked){
          unlockedTex = JRenderer::GetInstance()->LoadTexture("graphics/momir_unlocked.png", TEX_TYPE_USE_VRAM); 
          unlockedQuad = NEW JQuad(unlockedTex, 2, 2, 396, 96);
          GameOptions::GetInstance()->values[OPTIONS_MOMIR_MODE_UNLOCKED] = GameOption(1);
          GameOptions::GetInstance()->save();
        }
      }
      if (unlocked){
        JSample * sample = SampleCache::GetInstance()->getSample("sound/sfx/bonus.wav");
        if (sample) JSoundSystem::GetInstance()->PlaySample(sample); 
      }
    }
  }else{
      unlocked = 0;
  }
	mGamePhase = DUEL_STATE_END;
#ifdef TESTSUITE
	if (mParent->players[1] == PLAYER_TYPE_TESTSUITE){
	  if (testSuite->loadNext()){
	    loadTestSuitePlayers();
	    mGamePhase = DUEL_STATE_PLAY;
	    testSuite->initGame();
	  }else{
	    mGamePhase = DUEL_STATE_END;
	  }
	}else
#endif
	  if (mParent->players[0] == PLAYER_TYPE_CPU && mParent->players[1] == PLAYER_TYPE_CPU){
	    End();
	    Start();
	  }
	mFont->SetColor(ARGB(255,255,255,255));
      }
      if (mEngine->GetButtonClick(PSP_CTRL_START)){
	mGamePhase = DUEL_STATE_MENU;
      }
      break;
    case DUEL_STATE_MENU:
      menu->Update(dt);
      break;
    case DUEL_STATE_CANCEL:
      menu->Update(dt);
      if (menu->closed)
	mGamePhase = DUEL_STATE_PLAY;
      break;
    case DUEL_STATE_BACK_TO_MAIN_MENU:
      menu->Update(dt);
      if (menu->closed)
	mParent->SetNextState(GAME_STATE_MENU);
      break;
    default:
      if (PSP_CTRL_CIRCLE == mEngine->ReadButton()){
	mParent->SetNextState(GAME_STATE_MENU);
      }
    }
}


void GameStateDuel::Render()
{
  //Erase
  LOG("Start Render\n");
  JRenderer::GetInstance()->ClearScreen(ARGB(0,0,0,0));

  if (game)
    game->Render();
  switch (mGamePhase)
    {
    case DUEL_STATE_END:
      {
        JRenderer * r = JRenderer::GetInstance();
	      r->ClearScreen(ARGB(200,0,0,0));
	      char buffer[50];
	      int p0life = mPlayers[0]->life;
	      if (!mPlayers[0]->isAI() && mPlayers[1]->isAI() ){
	        if (game->gameOver != mPlayers[0]){
	          sprintf (buffer, "Victory! Congratulations, You earn 500 credits");
            
          }else{
	          sprintf (buffer, "You have been defeated");
	        }
	      }else{
	        int winner = 2;
	        if (game->gameOver !=mPlayers[0]){
	          winner = 1;
	        }
	        sprintf(buffer, "Player %i wins (%i)", winner, p0life );
	      }
	      mFont->DrawString(buffer, 10, 150);
        if (unlockedQuad){
          r->RenderQuad(unlockedQuad, 20, 20);
        }
        if (showMsg == 1){
            JLBFont * f = GameApp::CommonRes->GetJLBFont(Constants::MAIN_FONT);
            mFont->DrawString("Please support this project !" ,10,180);
            f->DrawString("Wagic is free, open source, and developed on the little free time I have" ,10,196);
            f->DrawString("If you enjoy this game, please consider donating a few bucks" ,10,208);
            f->DrawString("I'll drink a beer in your name!" ,10,220);
            f->DrawString("Thanks in advance for your support." ,10,232);
            mFont->DrawString("-> http://wololo.net/wagic" ,10,244);
        }
	      break;
      }
    case DUEL_STATE_CHOOSE_DECK1:
    case DUEL_STATE_CHOOSE_DECK1_TO_2:
    case DUEL_STATE_CHOOSE_DECK2:
    case DUEL_STATE_CHOOSE_DECK2_TO_PLAY:
      if (opponentMenu){
	      opponentMenu->Render();
      }else if (deckmenu){
	      deckmenu->Render();
      }
      break;
    case DUEL_STATE_ERROR_NO_DECK:
      mFont->DrawString("NO DECK AVAILABLE,",0,SCREEN_HEIGHT/2);
      mFont->DrawString("PRESS CIRCLE TO GO TO THE DECK EDITOR!",0,SCREEN_HEIGHT/2 + 20);
      break;
    case DUEL_STATE_MENU:
    case DUEL_STATE_CANCEL:
    case DUEL_STATE_BACK_TO_MAIN_MENU:
      menu->Render();
  }
  LOG("End Render\n");
}

void GameStateDuel::ButtonPressed(int controllerId, int controlId)
{
  switch (controllerId){
    case DUEL_MENU_CHOOSE_OPPONENT:
      {
        switch(controlId){
          case 0:
            loadPlayer(1);
	    opponentMenu->Close();
	    mGamePhase = DUEL_STATE_CHOOSE_DECK2_TO_PLAY;
            break;
          default:
            loadPlayer(1,controlId,1);
	    opponentMenu->Close();
	    mGamePhase = DUEL_STATE_CHOOSE_DECK2_TO_PLAY;
            break;

        }
        break;
      }
    default:
      {
      switch (controlId)
        {
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
            {
	            if (mGamePhase == DUEL_STATE_CHOOSE_DECK1){
	              loadPlayer(0,controlId);
		      deckmenu->Close();
	              mGamePhase = DUEL_STATE_CHOOSE_DECK1_TO_2;
	            }else{
	              loadPlayer(1,controlId);
		      deckmenu->Close();
	              mGamePhase = DUEL_STATE_CHOOSE_DECK2_TO_PLAY;
	            }
	            break;
            }
          case 12:
	    menu->Close();
	    mGamePhase = DUEL_STATE_BACK_TO_MAIN_MENU;
            break;
          case 13:
	    menu->Close();
            mGamePhase = DUEL_STATE_CANCEL;
            break;
        }
      }
  }
}

int GameStateDuel::isDifficultyUnlocked(){
  if (GameOptions::GetInstance()->values[OPTIONS_DIFFICULTY_MODE_UNLOCKED].getIntValue()) return 0;
  nbAIDecks = 0;
  int found = 1;
  int wins = 0;
  DeckStats * stats = DeckStats::GetInstance();
  stats->load(mPlayers[0]);
  while (found){
    found = 0;
    char buffer[512];
    char aiSmallDeckName[512];
    sprintf(buffer, RESPATH"/ai/baka/deck%i.txt",nbAIDecks+1);
    if(fileExists(buffer)){
      found = 1;
      nbAIDecks++;
      sprintf(aiSmallDeckName, "ai_baka_deck%i",nbAIDecks); 
      int percentVictories = stats->percentVictories(string(aiSmallDeckName));
      if (percentVictories >=67) wins++;
      if (wins >= 10) return 1;
    }
  }
  return 0;
}

int GameStateDuel::isMomirUnlocked(){
  if (GameOptions::GetInstance()->values[OPTIONS_MOMIR_MODE_UNLOCKED].getIntValue()) return 0;
  Player *p = mPlayers[0];
  if (p->game->inPlay->countByType("land") == 8) return 1;
  return 0;
}