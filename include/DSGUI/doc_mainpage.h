/*!
  \mainpage

  This is the documentation for the DSGUI library, a library for
  constructing graphical user interfaces on the Nintendo DS.

  DSGUI is written by Bj&ouml;rn Giesler <bjoern AT giesler.de>. I
  haven't decided on a license yet. Until I do, the following holds:
  You can use DSGUI freely in your code, without any royalties
  whatsoever. You can also modify it and use the modified version. You
  may NOT redistribute modified or unmodified versions yourself. The
  right for redistribution remains with me.

  There are some known and lots of unknown bugs in the code. If you
  find one, please fix it and send me a patch, I'll be happy to
  integrate it.
  
  \section screensandlayout Screens, Images, and Screen Layout
  
  The most basic user interface component in DSGUI is a widget (an
  instance of the BWidget class or one of its subclasses). Widgets can
  act as containers (\em parents) for other widgets (\em children),
  constructing a widget hierarchy or tree. At the root of that tree,
  there is a \em toplevel widget; one or several of such trees are
  managed by a screen (of the BScreen class). A screen is responsible
  for managing its widgets, delegating events to them (see \ref
  eventmanagement), and drawing them onto an \em image
  associated with the screen.

  In DSGUI, everything that is drawn gets drawn onto an image (an
  instance of the BImage class). When a widget draws itself, it draws
  itself onto an image at a certain point in that image. Images can
  manage their own memory, or they can act as proxies for existing
  memory blocks, such as VRAM blocks.

  A screen, therefore, manages an image and a collection of widgets to
  be drawn onto that image if necessary. Typically you would,
  for example, have one image and one screen per extended rotation
  background on the DS, so when widgets get drawn, they end up on the
  according background automatically. There is nothing that forces you
  to do that, though; you can have as many screens and images as you
  like and combine them at will if necessary.

  Here is some example code to set up a very simple DSGUI application
  that shows off the concepts of screens, images, and widgets. Let's
  look at it and then examine it in detail.

  \code
  // Set up the top screen buffer
  videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
  vramSetBankA(VRAM_A_MAIN_BG_0x6000000);
  BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(0);

  // Set up the top image and screen
  BImage* topImg = new BImage(256, 192, (u8*)BG_BMP_RAM(0));
  BScreen* topScreen = new BScreen(topImg);
  topScreen->clear();

  // Add the screen to the GUI
  BGUI::get()->addScreen(topScreen);

  // Label widget (with no parent, i.e. toplevel widget)
  BLabel* lbl = new BLabel(NULL, BRect(100, 80, 56, 32));
  lbl->setText("Hello World");
  topScreen->addWidget(lbl);

  // Enter GUI main loop
  BGUI::get()->run();
  \endcode

  The first three lines should be familiar if you've ever done
  anything with Nintendo DS development: They set up the video mode
  for the main screen to show one extended background (background
  no. 2), map the first main VRAM bank into memory and tell the
  background to use that VRAM bank as a 256x256 16-bpp image.

  The following three lines construct a proxy image that points to the
  VRAM bank we just mapped (we're using just 256x192 of it, since the
  DS won't display more than that anyway), and a screen that uses the
  new image. We also clear the top screen initially, since we don't
  know what was on there before.

  The next line makes the top screen known to the main GUI manager
  (see \ref guimanager).

  The three following lines construct a label widget with no parent
  (NULL as the first constructor argument), positioned at x=100, y=80
  and 56x32 pixels in size, set the label's text to be "Hello World"
  and add the label to the top screen.

  The last line enters the main GUI loop, drawing the widget if needed
  and distributing events. In our case, since we don't have anything
  that reacts to events, this amounts to an infinite loop.
  
  \section widgetcoordinates Widgets and Widget Coordinates

  Every widget, upon creation, gets a <i>widget frame
  rectangle</i>. This rectangle is interpreted relative to the widget's
  parent, if it has one. For toplevel widgets, the frame rectangle
  gives the widget's position in image coordinates; i.e., a toplevel
  widget with a frame rectangle of (10, 10, 100, 100) would occupy the
  rectangular screen space from x=10 to x=110 and from y=10 to y=110
  in the image that belongs to the screen that the image is on.

  The widget defines its own coordinate system, relative to the frame
  rectangle. Since all drawing is eventually done in the screen's
  image, there are some convenience methods to transform points from
  the widget coordinate system to the image coordinate system. These
  are BWidget::transformUp() (transforms from widget coordinates to
  parent coordinates) and BWidget::transformDown() (the other way
  'round). Example: Let's say you have a toplevel widget with a frame
  rectangle of (10, 10, 100, 100), and you want to draw a point at
  position x=5, y=5 relative to the widget. If you call
  transformUp(BPoint(5, 5)), the result is the point (15, 15), now in
  image coordinates.
  
  A widget can also be \em rotated in 90 degree steps, relative to its
  parent. This does not change the space occupied by the widget's
  frame (since that is given in the widget's parent's coordinates),
  but it does change the way transformUp() and transformDown() work.

  So to finish, if you write your own widget's draw() method, you
  should think in coordinates relative to your widget's top left
  corner (seen from the interior of the widget!), and only run them
  through transformUp() right before you need them for drawing. 

  \note While there is a BWidget::setFrame() method, widget resizing
  is not supported by any special mechanism; the rationale being that
  without a mouse, and with such a small screen, resizable windows are
  a very inefficient paradigm. If you store any frame-related stuff
  that needs to be adapted upon resize, you must therefore overwrite
  the setFrame() method yourself.
  
  \section eventmanagement Event Management and Delegates

  Cooperating classes: BRunLoop, BEvent, BResponder subclasses

  DSGUI spends most of its time running in a loop. This loop is
  represented by the BRunLoop class. Your widgets receive events from
  within this loop whenever a button is pressed or released (or while
  it is held), the touch screen is used or a VBlank interrupt
  occurs. The loop is entered by calling the BRunLoop's run() method
  and it can be exited by calling its stop() method.

  There can only be one active run loop at any time. This loop can be
  accessed using the static BRunLoop::currentRunLoop()
  method. Normally, you don't have to worry about run loops, as DSGUI
  does this automatically (when you call DSGUI::get()->run(), a global
  run loop is started, and all regular widgets will be serviced by
  this loop).

  Normally, you want your own code to be notified when something
  happens (e.g. a button is pressed). The mechanism to do this is
  called a \b callback, and in DSGUI this is achieved by using \b
  delegate classes. In short, this means that you must subclass a
  widget's delegate class and use the widget's setDelegate() method to
  set yourself as its delegate. Confusing? Let's look at BButton's
  delegate class:

  \code
  class BButton {
    class Delegate {
    public:
      virtual void onButtonClick(BButton* button) {}
    };
    // ...
    void setDelegate(Delegate* deleg);
    //...
  }
  \endcode

  This should clear things up. To use this mechanism, you:

  -# subclass BButton::Delegate
  -# implement onButtonClick()
  -# use setDelegate() on the button with your class instance as an argument

  Now, when the user clicks the button, your onButtonClick() method
  will be called. Easy, no?
  
  The drawback of this mechanism is that if your class is the delegate
  for several buttons, it needs to decide for itself which button
  called the onButtonClick() method. Oh well, there's a drawback to
  everything.

  Almost every widget has its own delegate class. You should check
  BWidget and its subclasses to see what is available. There is one
  non-BWidget class that uses the delegate mechanism, and that is
  BRunLoop. That class uses delegates for raw event handling (called
  every time an event is generated; widgets jack into this) and
  delegates for whenever a VBlank is reached (for timers, animations
  etc.)

  \note For experts: Events generated by a run loop go two separate
  ways: First, they are passed to all screens in the GUI manager (see
  \ref guimanager), and the screens will hand them down their widget
  hierarchy. Second, they go to all responders plugged into the run
  loop directly using the BRunLoop::addResponder() method.
  
  \section memorymanagement Memory Management

  Cooperating classes: BWidget subclasses

  Widgets are created dynamically; you always say

  \code
  BButton* b = new BButton();
  \endcode

  and \b never

  \code
  BButton b;
  \endcode

  You are responsible to delete toplevel widgets (i.e. widgets without
  a parent) when they are no longer needed. Widgets that are part of a
  hierarchy (i.e. that have a parent) <b> are deleted by its parent
  </b> if not explicitly specified otherwise by the
  BWidget::setDeletesChildren() method!

  \section Widget Geometry and Themes

  Cooperating classes: BTheme subclasses, BWidget subclasses

  FIXME: Needs work

  \section managers Built-in Managers

  FIXME: Needs work
  
  \subsection guimanager The GUI Manager

  Cooperating classes: BGUI, BScreen

  The GUI manager serves several purposes. First, it is the class that
  contains the main runloop. Second, it implements a communication
  mechanism with the ARM7 that lets you (if the ARM7 will react to it)
  switch the backlight off or on, set the backlight brighness, and
  power off the DS. Third, it works as a receptacle for screens. 

  \subsection fontmanager Fonts and the Font Manager

  Cooperating classes: BFont, BFontManager, BVirtualFile

  DSGUI can load fonts from flash cart or from memory. The
  BFontManager singleton class and the BFont class work together on
  this. You use the BFontManager::scanDirectory() method to obtain a
  list of all fonts that can be loaded, then you can query what fonts
  are available using BFontManager::fontNames() and
  BFontManager::sizesForFont().

  The scanDirectory() method does not actually load font bitmaps into
  memory, it just catalogs information about what fonts are available.
  You can actually obtain a font from the font manager by calling the
  BFontManager::font() method. The font is cached by the font manager,
  i.e. subsequent calls to font() will return the same font object,
  until that object is deleted. On deletion, the font is removed from
  memory; the next call to font() with the same arguments will re-load
  the font.

  Initially, the font manager knows no fonts at all and cannot return
  any fonts until either scanDirectory() has been called on a font
  directory or a font has been added by hand using the addFont()
  method. If you want a default font in the font manager, do the
  following:

  -# add the .bfont file as a resource to your project as you do with
     any binary resource
  -# create a BMemFile instance pointing to the data block
  -# create a BFont instance from the BVirtualFile
  -# (optionally) add it to the font manager using addFont().

  \subsection filemanager Files and the File Manager

  Cooperating classes: BFileManager, BVirtualFile, BMemFile, BFATFile

  The file manager abstracts away the DS's file system. It can list
  directory contents, handle path names, change directories
  etc. Depending on how you compiled DSGUI, it uses the gba_nds_fat or
  libfat libraries.

  Files in DSGUI are accessed using the BVirtualFile interface. There
  are two subclasses of BVirtualFile: BFATFile (which represents a
  true file on flash cart and takes the file name as the constructor
  argument) and BMemFile (which provides a file abstraction to a
  memory block and takes the address and length as constructor
  arguments).
  
  \subsection wifimanager The Wifi Manager
  ...doesn't exist yet.
*/

