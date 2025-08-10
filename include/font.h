#pragma once

#include <nds.h>
#include <string>
#include <vector>
#include "button.h"
#include "text.h"

class FontMenu {
public:
    FontMenu(App* app);
    ~FontMenu();
    void draw();
    inline const std::vector<std::string>& getFiles() const { return files; }
    void handleInput();
    inline bool isDirty() const { return dirty; }
    inline void setDirty(bool d = true) { dirty = d; }
private:
    void findFiles();
    void handleButtonPress();
    void handleTouchInput();
    void nextPage();
    void previousPage();
    void selectNext();
    void selectPrevious();
    App* app;
    std::vector<Button*> buttons;
    std::string dir;
    bool dirty;
    std::vector<std::string> files;
    u8 page;
    u8 pagesize;  //! Items per page.
    u8 selected;
};
