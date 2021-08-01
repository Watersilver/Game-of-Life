#ifndef CELLS_H
#define CELLS_H

#include <cstddef>
#include <cstring>
#include <stdexcept>

#include <iostream>

struct Cells
{
  Cells() :exists{ false }, w{ 0 }, h{ 0 }, bda{ nullptr }, bda2{ nullptr } {}
  Cells(std::size_t i)
  {
    setDimensions(i, i);
  }
  Cells(std::size_t i, std::size_t j)
  {
    setDimensions(i, j);
  }

  ~Cells()
  {
    destroy();
  }

  void setCell(const std::size_t& i, const std::size_t& j)
  {
    setCell(bda + i + 1 + (j + 1) * (w + 2));
  }

  void setCell(unsigned char* const cell_ptr)
  {
    // If already alive do nothing
    if (*cell_ptr & 0x01) return;

    *cell_ptr |= 0x01;

    // Inform neighbours
    *(cell_ptr + 1) += 0x02; // Cell to my right
    *(cell_ptr - 1) += 0x02; // Cell to my left
    *(cell_ptr - w - 2) += 0x02; // Cell to my top
    *(cell_ptr + w + 2) += 0x02; // Cell to my bottom
    *(cell_ptr + 1 - w - 2) += 0x02; // Cell to my tr
    *(cell_ptr - 1 - w - 2) += 0x02; // Cell to my tl
    *(cell_ptr + 1 + w + 2) += 0x02; // Cell to my br
    *(cell_ptr - 1 + w + 2) += 0x02; // Cell to my bl
  }

  void unsetCell(const std::size_t& i, const std::size_t& j)
  {
    unsetCell(bda + i + 1 + (j + 1) * (w + 2));
  }

  void unsetCell(unsigned char* const cell_ptr)
  {
    // If already dead do nothing
    if (!(*cell_ptr & 0x01)) return;

    *cell_ptr &= ~0x01;

    // Inform neighbours
    *(cell_ptr + 1) -= 0x02; // Cell to my right
    *(cell_ptr - 1) -= 0x02; // Cell to my left
    *(cell_ptr - w - 2) -= 0x02; // Cell to my top
    *(cell_ptr + w + 2) -= 0x02; // Cell to my bottom
    *(cell_ptr + 1 - w - 2) -= 0x02; // Cell to my tr
    *(cell_ptr - 1 - w - 2) -= 0x02; // Cell to my tl
    *(cell_ptr + 1 + w + 2) -= 0x02; // Cell to my br
    *(cell_ptr - 1 + w + 2) -= 0x02; // Cell to my bl
  }

  bool isAlive(const std::size_t& i, const std::size_t& j) const
  {
    return bda[i + 1 + (j + 1) * (w + 2)] & 0x01;
  }

  void nextGen()
  {
    auto current = bda + 1 + w + 2; // skip buffer
    auto next = bda2 + 1 + w + 2; // skip buffer

    // Loo terminates here
    //auto end = bda + (w + 2) * (h + 2) - 1 - w - 2;
    unsigned char const* const end = bda + w * h + w + 2 * h + 1;

    std::size_t x{ 0 };
    do {
      // Count living neighbours
      switch (*current >> 1)
      {
      case 2:
        // If alive
        if ((*current & 0x01))
        {
          // stay alive
          setCell(next);
        }
        else {
          // stay dead
          unsetCell(next);
        }
        break;
      case 3:
        setCell(next);
        break;
      default:
        unsetCell(next);
      }

      if (++x >= w)
      {
        x = 0;
        current += 2;
        next += 2;
      }

      ++next;
    } while (++current < end);

    // Swap arrays because bda2 now contains next gen
    auto temp = bda;
    bda = bda2;
    bda2 = temp;
  }

  void setDimensions(std::size_t i, std::size_t j)
  {
    destroy();
    exists = true;
    w = i;
    h = j;

    // +2 buffers so that set and unset won't need conditionals
    bda = new unsigned char[(i + 2) * (j + 2)];
    bda2 = new unsigned char[(i + 2) * (j + 2)];
    clear();
  }

  void setDimensions(std::size_t i)
  {
    setDimensions(i, i);
  }

  std::size_t getWidth()
  {
    return w;
  }

  std::size_t getHeight()
  {
    return h;
  }

  void destroy()
  {
    if (!exists) return;

    delete[] bda;
    delete[] bda2;
    exists = false;
  }

  void clear()
  {
    if (!exists) return;

    std::memset(bda, 0, (w + 2) * (h + 2));
    std::memset(bda2, 0, (w + 2) * (h + 2));
  }

  bool exist()
  {
    return exists;
  }

private:
  // The big dumb arrays that store the data
  // first bit is if I'm alive or not
  // next four are my neighbours
  // The second array is used to find next gen
  unsigned char* bda;
  unsigned char* bda2;
  bool exists;
  std::size_t w;
  std::size_t h;
};

#endif