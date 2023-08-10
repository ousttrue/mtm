#include "child_process.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <vterm.h>

struct Utf8 {
  char8_t b0 = 0;
  char8_t b1 = 0;
  char8_t b2 = 0;
  char8_t b3 = 0;

  const char8_t *begin() const { return &b0; }

  const char8_t *end() const {
    if (b0 == 0) {
      return &b0;
    } else if (b1 == 0) {
      return &b1;
    } else if (b2 == 0) {
      return &b2;
    } else if (b3 == 0) {
      return &b3;
    } else {
      return (&b3) + 1;
    }
  }

  std::u8string_view view() const { return {begin(), end()}; }
};

inline Utf8 to_utf8(char32_t cp) {
  if (cp < 0x80) {
    return {
        (char8_t)cp,
    };
  } else if (cp < 0x800) {
    return {(char8_t)(cp >> 6 | 0x1C0), (char8_t)((cp & 0x3F) | 0x80)};
  } else if (cp < 0x10000) {
    return {
        (char8_t)(cp >> 12 | 0xE0),
        (char8_t)((cp >> 6 & 0x3F) | 0x80),
        (char8_t)((cp & 0x3F) | 0x80),
    };
  } else if (cp < 0x110000) {
    return {
        (char8_t)(cp >> 18 | 0xF0),
        (char8_t)((cp >> 12 & 0x3F) | 0x80),
        (char8_t)((cp >> 6 & 0x3F) | 0x80),
        (char8_t)((cp & 0x3F) | 0x80),
    };
  } else {
    return {0xEF, 0xBF, 0xBD};
  }
}

struct VtNode : ftxui::Node {
  VTerm *m_vt;
  VTermScreen *m_screen;
  VtNode(VTerm *vt) : m_vt(vt) {
    vterm_set_utf8(m_vt, true);

    m_screen = vterm_obtain_screen(vt);
    vterm_screen_reset(m_screen, true);

    int row, col;
    vterm_get_size(m_vt, &row, &col);
    requirement_.min_x = col;
    requirement_.min_y = row;
  }

  void Render(ftxui::Screen &screen) override {
    int x = box_.x_min;
    const int y = box_.y_min;
    if (y > box_.y_max) {
      return;
    }

    // for (const auto &cell : Utf8ToGlyphs(text_)) {
    //   if (x > box_.x_max) {
    //     return;
    //   }
    //   screen.PixelAt(x, y).character = cell;
    //   ++x;
    // }
    //
    vterm_screen_flush_damage(m_screen);
    int rows, cols;
    vterm_get_size(m_vt, &rows, &cols);
    for (int y = 0; y < rows; ++y) {
      for (int x = 0; x < cols; ++x) {
        VTermScreenCell cell;
        vterm_screen_get_cell(m_screen, {y, x}, &cell);

        wchar_t ch = cell.chars[0];
        if (ch > 0 && ch < 128) {
          if (ch != ' ') {
            auto a = 0;
          }
        }

        auto &dst = screen.PixelAt(x, y).character;
        dst.clear();
        for (int i = 0; i < cell.width; ++i) {
          if (cell.chars[i] == 0) {
            dst = " ";
            break;
          }
          auto utf8 = to_utf8(cell.chars[i]);
          dst.append(utf8.begin(), utf8.end());
        }
      }
    }
  }
};

int main(void) {

  term_screen::SIZE size{.Rows = 30, .Cols = 60};

  auto vt = vterm_new(size.Rows, size.Cols);
  ftxui::Element document = std::make_shared<VtNode>(vt);

  auto child = term_screen::Process::Fork(size, "lsd.exe");
  char buf[8192];
  // while (child->Handle()) {
  if (auto read = child->ReadSync(buf, std::size(buf))) {
    vterm_input_write(vt, buf, read);
  } else {
    // break;
  }
  // }

  // Define the document
  // auto document = ftxui::hbox({
  //     ftxui::text("left") | ftxui::border,
  //     ftxui::text("middle") | ftxui::border | ftxui::flex,
  //     ftxui::text("right") | ftxui::border,
  // });

  auto screen = ftxui::Screen::Create(ftxui::Dimension::Full(),       // Width
                                      ftxui::Dimension::Fit(document) // Height
  );
  Render(screen, document);
  screen.Print();

  return EXIT_SUCCESS;
}
