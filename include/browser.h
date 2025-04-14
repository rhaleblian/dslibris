#pragma once

#include "button.h"

class App;
class Book;

class Browser {
  public:
    Browser(App* app);
    ~Browser();

    void AdvancePage();
    void Draw(void);
    Book* GetBookSelected() { return bookselected; }
    void Init(void);
    void HandleEvent();
    void Redraw(void);
    void RetreatPage();
    void SetBookSelected(Book *book) { bookselected = book; }

  private:
    App *app;
    u16 *screen;
    std::vector<Book*>* books;
    std::vector<Button*> buttons;
    u8 offset, maxperpage;
    Button menubutton[3];
    Book *bookselected;
};