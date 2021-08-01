#include "Life.h"

int main()
{
  Life game;
  if (game.Construct(1280, 720, 1, 1))
    game.Start();
  return 0;
}

