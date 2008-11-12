#include "../include/debug.h"
#include "../include/Player.h"
#include "../include/GameObserver.h"



Player::Player(MTGPlayerCards * _deck, string file): Damageable(20){
  deckFile = file;
  game = _deck;
  game->setOwner(this);
  manaPool = NEW ManaCost();
  canPutLandsIntoPlay = 1;
  mAvatar = NULL;
  type_as_damageable = DAMAGEABLE_PLAYER;
}

Player::~Player(){
  if (manaPool) delete manaPool;
  if (mAvatarTex) delete mAvatarTex;
  if (mAvatar) delete mAvatar;
}

MTGInPlay * Player::inPlay(){
  return game->inPlay;
}

int Player::getId(){
  GameObserver * game = GameObserver::GetInstance();
  for (int i= 0; i < 2; i++){
    if (game->players[i] == this) return i;
  }
  return -1;
}

JQuad * Player::getIcon(){
  return mAvatar;
}

Player * Player::opponent(){
  GameObserver * game = GameObserver::GetInstance();
  for (int i= 0; i < 2; i++){
    if (game->players[i] != this) return game->players[i];
  }
  return NULL;
}

HumanPlayer::HumanPlayer(MTGPlayerCards * _deck, char * file):Player(_deck, file){
  mAvatarTex = JRenderer::GetInstance()->LoadTexture("player/avatar.jpg", TEX_TYPE_USE_VRAM);
  if (mAvatarTex)
    mAvatar = NEW JQuad(mAvatarTex, 0, 0, 35, 50);
}

ManaCost * Player::getManaPool(){
  return manaPool;
}

int Player::manaBurn(){
  int burn = manaPool->getConvertedCost();
  life -= burn;
  manaPool->init();
  return burn;
}


int Player::testLife(){
  if (life <=0){
#if defined (WIN32) || defined (LINUX)
    //char    buf[4096], *p = buf;
    //sprintf(buf, "--Diff color TEST %i : %i\n", i, cost[i]);
    OutputDebugString("GAME OVER\n");
#endif
    //return GameObserver::GetInstance()->endOfGame();
  }
  return life;
}

int Player::afterDamage(){
  return testLife();
}

//Cleanup phase at the end of a turn
void Player::cleanupPhase(){
  game->inPlay->cleanupPhase();
  game->graveyard->cleanupPhase();
}
