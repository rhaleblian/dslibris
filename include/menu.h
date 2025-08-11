#pragma once
/*  Abstract base for Book, Pref and Font menus. */
#include <nds.h>
#include <vector>

class Menu {
public:
    Menu(class App *app);
    ~Menu();
    virtual void Draw() = 0; // Draw the menu on the screen
    u8 GetCurrentPage() const;
    u8 GetPageCount() const;
    virtual void HandleInput(u16 keys) = 0; // Handle input events
    void SelectItem(u8 index);

    class App *app;  //! Pointer to the application instance.
    std::vector<class Button*> buttons;
    bool dirty;
    u8 page;
    u8 pagesize;
    u8 selected;
};
