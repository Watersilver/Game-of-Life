#include "Life.h"

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>

bool Life::OnUserCreate()
{
  std::srand(std::time(nullptr));

  //cursor.Load("./assets/gfx/note.png");

  cam.initialize(this, { 16, 16 });

  return true;
}

bool Life::OnUserUpdate(float fElapsedTime)
{
  //Clear(olc::BLANK);

  if (GetKey(olc::Key::ESCAPE).bPressed)
  {
    paused = true;

    if (!menu.isOpen()) menu.open();
    else if (cells.exist()) menu.close();
  }

  // Speed up/down
  if (GetKey(olc::Key::RIGHT).bHeld)
  {
    frameDuration -= fElapsedTime;
    if (frameDuration < 0.001f) frameDuration = 0.001f;
  }
  if (GetKey(olc::Key::LEFT).bHeld)
  {
    frameDuration += fElapsedTime;
    if (frameDuration > 1.0f) frameDuration = 1.0f;
  }

  // Change Cell draw type
  if (GetKey(olc::Key::D).bPressed)
    cdt = Life::CellDrawType::dots;
  else if (GetKey(olc::Key::S).bPressed)
    cdt = Life::CellDrawType::squares;

  if (menu.isOpen())
  {
    menu.update(this, fElapsedTime);

    return true;
  }

  // Game of life screen
  cam.update(this, fElapsedTime);

  // Pause
  if (GetKey(olc::Key::ENTER).bPressed || GetKey(olc::Key::SPACE).bPressed)
    paused = !paused;

  // Randomize
  if (GetKey(olc::Key::R).bPressed)
    randomize();

  // Clear
  if (GetKey(olc::Key::C).bPressed)
    cells.clear();

  // Add/Remove Tiles
  const auto& view = cam.getView();
  const auto mouseTile = view.GetTileUnderScreenPos(GetMousePos());

  const auto isMouseInGrid = mouseTile.x >= 0 && mouseTile.y >= 0 && mouseTile.x < gridDimensions.x && mouseTile.y < gridDimensions.y;

  if (GetMouse(0).bPressed && isMouseInGrid)
  {
    drawMode = !cells.isAlive(mouseTile.x, mouseTile.y);
    paused = true;
    frameTimer = 0.0f;
  }

  if (GetMouse(0).bHeld && isMouseInGrid)
  {
    if (drawMode) cells.setCell(mouseTile.x, mouseTile.y);
    else cells.unsetCell(mouseTile.x, mouseTile.y);
    paused = true;
    frameTimer = 0.0f;
  }

  if (!paused)
  {
    // Update frame
    if (frameTimer > frameDuration) {

      cells.nextGen();
      frameTimer = std::fmod(frameTimer, frameDuration);
    }
    frameTimer += fElapsedTime;
  }

  const olc::Pixel backgroundColour( bgR, bgG, bgB );
  Clear(backgroundColour);

  cam.draw(this, fElapsedTime);

  if (paused)
  {
    DrawString({ 10, 10 }, "Paused", olc::WHITE, 2U);
  }

  return true;
}

void Life::newGrid(int i, int j)
{
  gridDimensions = { i, j };

  cells.setDimensions(gridDimensions.x, gridDimensions.y);

  cells.clear();

  randomize();

  cam.center(this);
}

void Life::randomize()
{
  if (!cells.exist()) return;

  cells.clear();
  for (auto j = 0; j < gridDimensions.y; j++)
    for (auto i = 0; i < gridDimensions.x; i++)
    {
      if (rand() % 100 < lifeChance) cells.setCell(i, j);
      else cells.unsetCell(i, j);
    }
  frameTimer = .0f;
}


void Life::Camera::smoothDecrease(float& value, float fElapsedTime, float factor)
{
  if (value > 0)
  {
    value -= value * factor * 60 * fElapsedTime;
    if (value < 0) value = 0.0f;
  }
  else
  {
    value -= value * factor * 60 * fElapsedTime;
    if (value > 0) value = 0.0f;
  }
}

void Life::Camera::update(Life const* const life, float fElapsedTime)
{
  const auto& gridDimensions = life->gridDimensions;

  // Smooth Pan
  if (life->GetMouse(1).bPressed) tv.StartPan(life->GetMousePos());
  if (life->GetMouse(1).bHeld)
  {
    panPos = life->GetMousePos();
    mouseVel = (olc::vf2d(panPos) - olc::vf2d(prevMousePos)) / fElapsedTime;
  }
  else
  {
    panPos += mouseVel * fElapsedTime;
  }

  smoothDecrease(mouseVel.x, fElapsedTime);
  smoothDecrease(mouseVel.y, fElapsedTime);

  tv.UpdatePan(panPos);

  // Clamp camera
  const auto tl = tv.GetWorldTL();
  const auto br = tv.GetWorldBR();
  const auto brTile = tv.GetBottomRightTile();

  olc::vf2d clamped{ tl };

  if ((tl.x <= 0.0f) && (brTile.x >= gridDimensions.x))
  {
    clamped.x = (gridDimensions.x - (br.x - tl.x)) / 2.0f;
  }
  else if (tl.x <= 0.0f)
  {
    clamped.x = 0.0f;
  }
  else if (brTile.x >= gridDimensions.x)
  {
    clamped.x = gridDimensions.x - (br.x - tl.x);
  }

  if ((tl.y <= 0.0f) && (brTile.y >= gridDimensions.y))
  {
    clamped.y = (gridDimensions.y - (br.y - tl.y)) / 2.0f;
  }
  else if (tl.y <= 0.0f)
  {
    clamped.y = 0.0f;
  }
  else if (brTile.y >= gridDimensions.y)
  {
    clamped.y = gridDimensions.y - (br.y - tl.y);
  }

  if (clamped != tl)
    tv.SetWorldOffset(clamped);

  // Smooth Zoom
  if (life->GetMouseWheel() > 0)
  {
    zoomMousePos = life->GetMousePos();
    dZoom += fElapsedTime;
  }
  if (life->GetMouseWheel() < 0)
  {
    zoomMousePos = life->GetMousePos();
    dZoom -= fElapsedTime;
  }

  // Clamp Zoom
  const auto ws = tv.GetWorldScale();

  if (ws.x > 100.0f)
    dZoom = -.01f;
  else if (ws.x < 1.0f)
    dZoom = .01f;

  if (dZoom > 0.06f) dZoom = 0.06f;
  if (dZoom < -0.06f) dZoom = -0.06f;

  tv.ZoomAtScreenPos(1.0f + dZoom, zoomMousePos);

  smoothDecrease(dZoom, fElapsedTime);

  prevMousePos = life->GetMousePos();
}

void Life::Camera::draw(Life const* const life, float fElapsedTime)
{
  const auto& gridDimensions = life->gridDimensions;
  const auto& cdt = life->cdt;
  const auto& cells = life->cells;
  const olc::Pixel colour( life->cR, life->cG, life->cB );

  const auto tl = tv.GetTopLeftTile().max({ 0, 0 });
  const auto br = tv.GetBottomRightTile().min(gridDimensions);

  olc::vi2d tile;

  if (cdt == Life::CellDrawType::dots)
    for (tile.y = tl.y; tile.y < br.y; tile.y++)
      for (tile.x = tl.x; tile.x < br.x; tile.x++)
      {
        if (cells.isAlive(tile.x, tile.y))
        {
          tv.FillCircle(olc::vf2d(tile) + olc::vf2d{ .5f, .5f }, .3f, colour);
        }
      }
  else if (cdt == Life::CellDrawType::squares)
    for (tile.y = tl.y; tile.y < br.y; tile.y++)
      for (tile.x = tl.x; tile.x < br.x; tile.x++)
      {
        if (cells.isAlive(tile.x, tile.y))
        {
          tv.FillRect(olc::vf2d(tile) + olc::vf2d{ .1f, .1f }, { .8f, .8f }, colour);
          tv.Draw(olc::vf2d(tile) + olc::vf2d{ .5f, .5f }, colour);
        }
      }
}


void Life::Menu::update(Life* const life, float fElapsedTime)
{
  const auto mousePos = life->GetMousePos();
  const auto mouse = life->GetMouse(0);
  const auto contentHeight = (Indexes::end + 1) * lineHeight;

  Rect scrollbar{ { life->ScreenWidth() - 20, static_cast<int>(life->ScreenHeight() * static_cast<float>(-topOffset) / static_cast<float>(contentHeight)) },
    { 20, static_cast<int>(life->ScreenHeight() * (static_cast<float>(life->ScreenHeight()) / static_cast<float>(contentHeight))) } };

  // Interaction
  // Scroll
  if (life->GetMouseWheel() > 0)
    topOffset += 1000 * fElapsedTime;
  if (life->GetMouseWheel() < 0)
    topOffset -= 1000 * fElapsedTime;

  if (life->GetKey(olc::Key::UP).bHeld)
    topOffset += 1000 * fElapsedTime;
  if (life->GetKey(olc::Key::DOWN).bHeld)
    topOffset -= 1000 * fElapsedTime;

  if (isInRect(scrollbar, mousePos))
  {
    hoverScroll = true;
    if (mouse.bPressed)
    {
      dragginScrollbar = true;
      dragPos = static_cast<float>(mousePos.y - scrollbar.pos.y) / static_cast<float>(scrollbar.size.y);
    }
  }
  else
  {
    hoverScroll = false;

    if (mouse.bPressed && mousePos.x > life->ScreenWidth() - 20)
    {
      if (mousePos.y < scrollbar.pos.y)
      {
        topOffset = -(contentHeight * (static_cast<float>(mousePos.y) / static_cast<float>(life->ScreenHeight())));
      }
      else
      {
        topOffset = -(contentHeight * (static_cast<float>(mousePos.y - scrollbar.size.y) / static_cast<float>(life->ScreenHeight())));
      }
    }
  }
  if (mouse.bReleased) dragginScrollbar = false;

  if (dragginScrollbar)
    topOffset = - (contentHeight * (static_cast<float>(mousePos.y - dragPos * scrollbar.size.y) / static_cast<float>(life->ScreenHeight())));

  if (topOffset > 0.0f) topOffset = 0.0f;
  if (topOffset < -contentHeight + life->ScreenHeight()) topOffset = -contentHeight + life->ScreenHeight();

  // Mouse selection
  if (isInRect(getRect(rowsInputBox), life->GetMousePos()) && mouse.bPressed)
    selected = Selection::rows;
  else if (isInRect(getRect(colsInputBox), mousePos) && mouse.bPressed)
    selected = Selection::columns;
  else if (isInRect(getRect(newGridButton), mousePos) && mouse.bPressed)
  {
    selected = Selection::gridButton;

    if (newGridRows > 0 && newGridCols > 0) {
      gridButtonSelection = olc::DARK_GREEN;

      life->newGrid(newGridRows, newGridCols);
      newGridRows = newGridCols  = -1;
    }
    else gridButtonSelection = olc::DARK_RED;
  }
  else if (isInRect(getRect(lifeChanceInput), mousePos) && mouse.bPressed)
  {
    life->lifeChance = 0;
    selected = Selection::lifeChance;
  }
  else if (isInRect(getRect(randomizeButton), mousePos) && mouse.bPressed || life->GetKey(olc::Key::R).bPressed)
  {
    if (life->cells.exist()) populaceButtonSelection = olc::DARK_GREEN;
    else populaceButtonSelection = olc::DARK_RED;
    selected = Selection::randomButton;
    life->randomize();
  }
  else if (isInRect(getRect(clearButton), mousePos) && mouse.bPressed || life->GetKey(olc::Key::C).bPressed)
  {
    if (life->cells.exist()) populaceButtonSelection = olc::DARK_GREEN;
    else populaceButtonSelection = olc::DARK_RED;
    selected = Selection::clearButton;

    life->cells.clear();
  }
  else if (isInRect(getRect(cRInp), mousePos) && mouse.bPressed)
    selected = Selection::colR;
  else if (isInRect(getRect(cGInp), mousePos) && mouse.bPressed)
    selected = Selection::colG;
  else if (isInRect(getRect(cBInp), mousePos) && mouse.bPressed)
    selected = Selection::colB;
  else if (isInRect(getRect(bgRInp), mousePos) && mouse.bPressed)
    selected = Selection::bgR;
  else if (isInRect(getRect(bgGInp), mousePos) && mouse.bPressed)
    selected = Selection::bgG;
  else if (isInRect(getRect(bgBInp), mousePos) && mouse.bPressed)
    selected = Selection::bgB;
  else if (isInRect(getRect(dotsButton), mousePos) && mouse.bPressed)
    life->cdt = CellDrawType::dots;
  else if (isInRect(getRect(squaresButton), mousePos) && mouse.bPressed)
    life->cdt = CellDrawType::squares;
  else if (mouse.bPressed)
    selected = Selection::none;

  // Keyboard input
  int keyInp = -2;
  keyInp = life->GetKey(olc::Key::K0).bPressed ? 0 : keyInp;
  keyInp = life->GetKey(olc::Key::NP0).bPressed ? 0 : keyInp;
  keyInp = life->GetKey(olc::Key::K1).bPressed ? 1 : keyInp;
  keyInp = life->GetKey(olc::Key::NP1).bPressed ? 1 : keyInp;
  keyInp = life->GetKey(olc::Key::K2).bPressed ? 2 : keyInp;
  keyInp = life->GetKey(olc::Key::NP2).bPressed ? 2 : keyInp;
  keyInp = life->GetKey(olc::Key::K3).bPressed ? 3 : keyInp;
  keyInp = life->GetKey(olc::Key::NP3).bPressed ? 3 : keyInp;
  keyInp = life->GetKey(olc::Key::K4).bPressed ? 4 : keyInp;
  keyInp = life->GetKey(olc::Key::NP4).bPressed ? 4 : keyInp;
  keyInp = life->GetKey(olc::Key::K5).bPressed ? 5 : keyInp;
  keyInp = life->GetKey(olc::Key::NP5).bPressed ? 5 : keyInp;
  keyInp = life->GetKey(olc::Key::K6).bPressed ? 6 : keyInp;
  keyInp = life->GetKey(olc::Key::NP6).bPressed ? 6 : keyInp;
  keyInp = life->GetKey(olc::Key::K7).bPressed ? 7 : keyInp;
  keyInp = life->GetKey(olc::Key::NP7).bPressed ? 7 : keyInp;
  keyInp = life->GetKey(olc::Key::K8).bPressed ? 8 : keyInp;
  keyInp = life->GetKey(olc::Key::NP8).bPressed ? 8 : keyInp;
  keyInp = life->GetKey(olc::Key::K9).bPressed ? 9 : keyInp;
  keyInp = life->GetKey(olc::Key::NP9).bPressed ? 9 : keyInp;
  keyInp = life->GetKey(olc::Key::BACK).bPressed ? -1 : keyInp;

  if (keyInp != -2)
  {
    if (selected == Selection::rows)
      input(keyInp, newGridRows, 9999);
    else if (selected == Selection::columns)
      input(keyInp, newGridCols, 9999);
    else if (selected == Selection::lifeChance)
      input(keyInp, life->lifeChance, 99);
    else if (selected == Selection::colR)
      input(keyInp, life->cR, 255);
    else if (selected == Selection::colG)
      input(keyInp, life->cG, 255);
    else if (selected == Selection::colB)
      input(keyInp, life->cB, 255);
    else if (selected == Selection::bgR)
      input(keyInp, life->bgR, 255);
    else if (selected == Selection::bgG)
      input(keyInp, life->bgG, 255);
    else if (selected == Selection::bgB)
      input(keyInp, life->bgB, 255);
  }

  // Draw
  life->Clear(olc::BLANK);

  life->FillRect(scrollbar.pos, scrollbar.size,
    dragginScrollbar ? olc::WHITE :
    hoverScroll ? olc::GREY : olc::DARK_GREY
  );

  life->DrawString(getRect(Indexes::title).pos, "Conway's Game of Life", olc::WHITE, 5);

  life->DrawString(getRect(Indexes::instructions1).pos, "Insert grid rows and columns,", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions2).pos, "click \"create new grid\"", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions3).pos, "press Escape to enter/exit this menu screen.", olc::WHITE, 3);

  auto gridPos = getRect(Indexes::grid).pos;
  life->DrawString(gridPos, "Input grid size: ", olc::WHITE, 3);

  if (selected == Selection::rows) drawInputBox(life, rowsInputBox, newGridRows, olc::VERY_DARK_GREY);
  else drawInputBox(life, rowsInputBox, newGridRows);
  gridPos.x += 12 + rowsInputBox.bounds.x + rowsInputBox.bounds.y;
  life->DrawString(gridPos, "x", olc::WHITE, 3);

  if (selected == Selection::columns) drawInputBox(life, colsInputBox, newGridCols, olc::VERY_DARK_GREY);
  else drawInputBox(life, colsInputBox, newGridCols);

  drawInputBox(life, newGridButton, "Create New Grid",
    selected == Selection::gridButton ? gridButtonSelection :
    isInRect(getRect(newGridButton), mousePos) ? olc::VERY_DARK_GREY : olc::BLANK
  );

  life->DrawString(getRect(Indexes::lifeChance).pos, "Chance of life when randomizing (%): ", olc::WHITE, 3);
  drawInputBox(life, lifeChanceInput, life->lifeChance, selected == Selection::lifeChance ? olc::VERY_DARK_GREY : olc::BLANK);

  drawInputBox(life, randomizeButton, "Randomize",
    selected == Selection::randomButton ? populaceButtonSelection :
    isInRect(getRect(randomizeButton), mousePos) ? olc::VERY_DARK_GREY : olc::BLANK
  );

  drawInputBox(life, clearButton, "Clear",
    selected == Selection::clearButton ? populaceButtonSelection :
    isInRect(getRect(clearButton), mousePos) ? olc::VERY_DARK_GREY : olc::BLANK
  );

  life->DrawString(getRect(Indexes::speed).pos, "Simulation Speed: ", olc::WHITE, 3);

  const auto sliderStart = 410;
  const auto sliderEnd = 910;
  speedSlider.bounds.x = sliderStart + static_cast<int>((sliderEnd - sliderStart) * (1.0f - life->frameDuration));
  if (isInRect(getRect(speedSlider), mousePos) && mouse.bPressed)
  {
    speedSlider.dragged = true;
  }
  if (mouse.bReleased) speedSlider.dragged = false;
  if (speedSlider.dragged)
  {
    speedSlider.bounds.x = mousePos.x - padScreenL - 10;
    if (speedSlider.bounds.x < sliderStart)
      speedSlider.bounds.x = sliderStart;
    if (speedSlider.bounds.x > sliderEnd)
      speedSlider.bounds.x = sliderEnd;

    life->frameDuration = static_cast<float>(sliderEnd - speedSlider.bounds.x) / static_cast<float>(sliderEnd - sliderStart);
    if (life->frameDuration > 1.0f) life->frameDuration = 1.0f;
    if (life->frameDuration <= 0.0f) life->frameDuration = .001f;
  }
  life->FillRect(getRect(Indexes::speed).pos + olc::vi2d{ sliderStart, 5 }, { sliderEnd - sliderStart + 20, 10}, olc::VERY_DARK_GREY);
  drawInputBox(life, speedSlider, "", (speedSlider.dragged || isInRect(getRect(speedSlider), mousePos)) ? olc::WHITE : olc::GREY);

  life->DrawString(getRect(Indexes::colour).pos, "Colour (RGB): ", olc::WHITE, 3);
  drawInputBox(life, cRInp, life->cR, selected == Selection::colR ? olc::VERY_DARK_GREY : olc::BLANK);
  drawInputBox(life, cGInp, life->cG, selected == Selection::colG ? olc::VERY_DARK_GREY : olc::BLANK);
  drawInputBox(life, cBInp, life->cB, selected == Selection::colB ? olc::VERY_DARK_GREY : olc::BLANK);

  life->DrawString(getRect(Indexes::backgroundColour).pos, "Background colour (RGB): ", olc::WHITE, 3);
  drawInputBox(life, bgRInp, life->bgR, selected == Selection::bgR ? olc::VERY_DARK_GREY : olc::BLANK);
  drawInputBox(life, bgGInp, life->bgG, selected == Selection::bgG ? olc::VERY_DARK_GREY : olc::BLANK);
  drawInputBox(life, bgBInp, life->bgB, selected == Selection::bgB ? olc::VERY_DARK_GREY : olc::BLANK);


  drawInputBox(life, dotsButton, "Dots",
    life->cdt == CellDrawType::dots ? olc::VERY_DARK_CYAN :
    isInRect(getRect(dotsButton), mousePos) ? olc::VERY_DARK_GREY : olc::BLANK
  );
  drawInputBox(life, squaresButton, "Squares",
    life->cdt == CellDrawType::squares ? olc::VERY_DARK_CYAN :
    isInRect(getRect(squaresButton), mousePos) ? olc::VERY_DARK_GREY : olc::BLANK
  );

  // Display preview
  auto previewRect = getRect(Indexes::colour);
  previewRect.size.y *= 3;
  previewRect.size.x = previewRect.size.y;
  previewRect.pos.x = squaresButton.bounds.x + squaresButton.bounds.y + 150;
  previewRect.pos.y -= 20;
  life->FillRect(previewRect.pos, previewRect.size, olc::Pixel(life->bgR, life->bgG, life->bgB));

  if (life->cdt == CellDrawType::dots)
    life->FillCircle(previewRect.pos + previewRect.size / 2, 25, olc::Pixel(life->cR, life->cG, life->cB));
  else if (life->cdt == CellDrawType::squares)
    life->FillRect(previewRect.pos + previewRect.size / 2 - olc::vi2d{30, 30}, { 60, 60 }, olc::Pixel(life->cR, life->cG, life->cB));

  life->DrawString(getRect(Indexes::instructions4).pos, "Instructions: ", olc::WHITE, 4);
  life->DrawString(getRect(Indexes::instructions9).pos, "Left mouse button to draw cells", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions10).pos, "Right mouse button to pan screen", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions11).pos, "Mouse wheel to zoom in/out", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions5).pos, "Space and Enter to pause simulation", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions6).pos, "R to randomize and C to clear", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions7).pos, "Left and Right Arrows to change simulation speed", olc::WHITE, 3);
  life->DrawString(getRect(Indexes::instructions8).pos, "S and D to switch between dots and squares", olc::WHITE, 3);
}