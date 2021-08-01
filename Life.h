#ifndef LIFE_H
#define LIFE_H

#include <cstddef>
#include <bitset>
#include <string>

#include "olcPixelGameEngine.h"
#include "olcPGEX_TransformedView.h"

#include "Cells.h"

class Life : public olc::PixelGameEngine
{
  olc::vi2d gridDimensions;

  //olc::Renderable cursor;

  Cells cells;

  bool paused{ true };
  bool drawMode{ 0 }; // Drawing or erasing
  int lifeChance{ 40 }; // life chance for randomize

  float frameDuration{ .01f }; // how often cells update
  float frameTimer{ .0f }; // time towards next cells update

  int cR{ 255 }, cG{ 0 }, cB{ 255 }; // 255, 0, 255 is magenta
  int bgR{ 0 }, bgG{ 0 }, bgB{ 64 }; // 0, 0, 64 is very dark blue

  enum class CellDrawType
  {
    squares, dots
  };

  CellDrawType cdt{CellDrawType::dots};

  // Class that handles game panning, zooming and drawing
  class Camera
  {
    olc::TileTransformedView tv;

    float dZoom{ 0 };
    olc::vi2d prevMousePos{ 0, 0 };
    olc::vi2d panPos{ 0, 0 };
    olc::vf2d mouseVel{ 0, 0 };
    olc::vi2d zoomMousePos{ 0, 0 };

    void smoothDecrease(float& value, float fElapsedTime, float factor = 0.1f);
  public:
    void initialize(Life const * const life, const olc::vf2d& scale)
    {
      tv.Initialise({ life->ScreenWidth(), life->ScreenHeight() }, scale);
    }
    void center(Life const* const life)
    {
      const auto tl = tv.GetTopLeftTile().max({ 0, 0 });
      const auto br = tv.GetBottomRightTile().min(life->gridDimensions);
      tv.SetWorldOffset(olc::vf2d(life->gridDimensions - (br - tl)) / 2.0f);
    }
    const olc::TileTransformedView& getView() const
    {
      return tv;
    }
    void update(Life const* const life, float fElapsedTime);
    void draw(Life const* const life, float fElapsedTime);
  };

  Camera cam;

  // Class that handles menu interations and drawing
  class Menu
  {
    struct Rect
    {
      olc::vi2d pos;
      olc::vi2d size;
    };

    struct InputBox
    {
      int index;
      olc::vi2d bounds;
      int limit{ -1 };
      bool dragged{ false };
    };

    enum Indexes : int
    {
      title,
      instructions1 = title + 2,
      instructions2,
      instructions3,
      grid = instructions3 + 2,
      lifeChance = grid + 2,
      populaceControl = lifeChance + 2,
      speed = populaceControl + 2,
      colour = speed + 2,
      backgroundColour,
      shape,
      instructions4 = shape + 3,
      instructions9 = instructions4 + 2,
      instructions10,
      instructions11,
      instructions5,
      instructions6,
      instructions7,
      instructions8,
      end
    };

    bool dragginScrollbar{ false };
    bool hoverScroll{ false };
    float dragPos;

    int padScreenT{ 40 };
    int padScreenL{ 40 };
    int lineHeight{ 60 };
    float topOffset{ 0.0f };
    bool opened{ true };

    int newGridRows{ -1 };
    int newGridCols{ -1 };
    InputBox rowsInputBox{ Indexes::grid, {400, 102} };
    InputBox colsInputBox{ Indexes::grid, {552, 102} };
    InputBox newGridButton{ Indexes::grid, {800, 365} };
    olc::Pixel gridButtonSelection; // Will be green if inputs are valid and red if invalid

    InputBox lifeChanceInput{ Indexes::lifeChance, {870, 56} };

    InputBox randomizeButton{ Indexes::populaceControl, {200, 222} };
    InputBox clearButton{ Indexes::populaceControl, {600, 125} };
    olc::Pixel populaceButtonSelection; // Will be green if operations were successful and red otherwise

    InputBox cRInp{ Indexes::colour, {600, 80} };
    InputBox cGInp{ Indexes::colour, {680, 80} };
    InputBox cBInp{ Indexes::colour, {760, 80} };

    InputBox bgRInp{ Indexes::backgroundColour, {600, 80} };
    InputBox bgGInp{ Indexes::backgroundColour, {680, 80} };
    InputBox bgBInp{ Indexes::backgroundColour, {760, 80} };

    InputBox dotsButton{ Indexes::shape, {600, 100} };
    InputBox squaresButton{ Indexes::shape, {700, 175} };

    InputBox speedSlider{ Indexes::speed, {0, 20} };

    enum class Selection
    {
      none, rows, columns, gridButton, lifeChance,
      colR, colG, colB, bgR, bgG, bgB, randomButton, clearButton
    };

    Selection selected;

    const Rect getRect(int index)
    {
      int iOffset{ padScreenT + static_cast<int>(topOffset) + index * lineHeight };
      return { { padScreenL , iOffset }, {1000, lineHeight} };
    }

    const Rect getRect(InputBox inp)
    {
      auto rect = getRect(inp.index);
      rect.pos.x += inp.bounds.x;
      rect.pos.y -= 20;

      rect.size.x = inp.bounds.y;

      return rect;
    }

    void input(int keyInp, int& value, int limit = -1)
    {
      if (value < 0) value = 0;
      if (keyInp < 0)
      {
        value /= 10;
      }
      else
      {
        auto prevVal = value;
        value *= 10;
        value += keyInp;

        // Overflow
        if (value / 10 != prevVal) value = prevVal;

        if (limit > 0 && value > limit) value = limit;
      }
    }

    void drawInputBox(Life* const life, InputBox inp, int value, olc::Pixel bgc = olc::BLANK)
    {
      drawInputBox(life, inp, value >= 0 ? std::to_string(value) : "", bgc);
    }

    void drawInputBox(Life* const life, InputBox inp, const std::string& value, olc::Pixel bgc = olc::BLANK)
    {
      auto rect = getRect(inp);

      life->FillRect(rect.pos, rect.size, bgc);
      life->DrawRect(rect.pos, rect.size);

      rect.pos.x += 5;
      rect.pos.y += 20;
      life->DrawString(rect.pos, value, olc::WHITE, 3);
    }

    bool isInRect(Rect r, olc::vi2d p)
    {
      return p.x > r.pos.x && p.y > r.pos.y && p.x < r.pos.x + r.size.x && p.y < r.pos.y + r.size.y;
    }

  public:
    void open() { opened = true; }
    void close() {
      opened = false;
      selected = Selection::none;
      speedSlider.dragged = false;
      dragginScrollbar = false;
    }
    bool isOpen() { return opened; }
    void update(Life* const life, float fElapsedTime);
  };

  Menu menu;

  void newGrid(int i, int j);

  void randomize();

public:
  Life()
  {
    sAppName = "Conway's Game of Life";
  }

  bool OnUserCreate() override;

  bool OnUserUpdate(float fElapsedTime) override;
};

#endif