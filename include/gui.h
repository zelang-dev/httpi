#ifndef _GUI_H
#define _GUI_H

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc-runtime.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#define _DEFAULT_SOURCE 1
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/X.h>
#include <X11/Xresource.h>
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <X11/Core.h>
#include <X11/Object.h>
#include <X11/Shell.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Xmu.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/AsciiText.h>

#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

#include "Linux/TextField.h"
#include "Linux/FileSelect.h"
#include "Linux/Gridbox.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define RGB_COLOR(r, g, b)		(r),(g),(g)
#define RGB_BLACK				0,0,0
#define RGB_WHITE				255,255,255
#define RGB_RED					255,0,0
#define RGB_LIME				0,255,0
#define RGB_BLUE				0,0,255
#define RGB_YELLOW				255,255,0
#define RGB_CYAN				0,255,255
#define RGB_MAGENTA				255,0,255
#define RGB_SILVER				192,192,192
#define RGB_GRAY				128,128,128
#define RGB_MAROON				128,0,0
#define RGB_OLIVE				128,128,0
#define RGB_GREEN				0,128,0
#define RGB_PURPLE				128,0,128
#define RGB_TEAL				0,128,128
#define RGB_NAVY				0,0,128
#define RGB_MAROON				128,0,0
#define RGB_DARK_RED			139,0,0
#define RGB_BROWN				165,42,42
#define RGB_FIREBRICK			178,34,34
#define RGB_CRIMSON				220,20,60
#define RGB_RED					255,0,0
#define RGB_TOMATO				255,99,71
#define RGB_CORAL				255,127,80
#define RGB_INDIAN_RED			205,92,92
#define RGB_LIGHT_CORAL			240,128,128
#define RGB_DARK SALMON			233,150,122
#define RGB_SALMON				250,128,114
#define RGB_LIGHT_SALMON		255,160,122
#define RGB_ORANGE_RED			255,69,0
#define RGB_DARK_ORANGE			255,140,0
#define RGB_ORANGE				255,165,0
#define RGB_GOLD				255,215,0
#define RGB_DARK_GOLDEN_ROD		184,134,11
#define RGB_GOLDEN_ROD			218,165,32
#define RGB_PALE_GOLDEN_ROD		238,232,170
#define RGB_DARK_KHAKI			189,183,107
#define RGB_KHAK				240,230,140
#define RGB_OLIVE				128,128,0
#define RGB_YELLOW				255,255,0
#define RGB_YELLOW_GREEN		154,205,50
#define RGB_DARK_OLIVE_GREEN	85,107,47
#define RGB_OLIVE_DRAB			107,142,35
#define RGB_LAWN_GREEN			124,252,0
#define RGB_CHARTREUSE			127,255,0
#define RGB_GREEN_YELLOW		173,255,47
#define RGB_DARK_GREEN			0,100,0
#define RGB_GREEN				0,128,0
#define RGB_FOREST_GREEN		34,139,34
#define RGB_LIME				0,255,0
#define RGB_LIME_GREEN			50,205,50
#define RGB_LIGHT_GREEN			144,238,144
#define RGB_PALE_GREEN			152,251,152
#define RGB_DARK_SEA_GREEN		143,188,143
#define RGB_MEDIUM_SPRING_GREEN	0,250,154
#define RGB_SPRING_GREEN		0,255,127
#define RGB_SEA_GREEN			46,139,87
#define RGB_MEDIUM_AQUA_MARINE	102,205,170
#define RGB_MEDIUM_SEA_GREEN	60,179,113
#define RGB_LIGHT_SEA_GREEN		32,178,170
#define RGB_DARK_SLATE_GRAY		47,79,79
#define RGB_TEAL				0,128,128
#define RGB_DARK_CYAN			0,139,139
#define RGB_AQUA				0,255,255
#define RGB_CYAN				0,255,255
#define RGB_LIGHT_CYAN			224,255,255
#define RGB_DARK_TURQUOISE		0,206,209
#define RGB_TURQUOISE			64,224,208
#define RGB_MEDIUM_TURQUOISE	72,209,204
#define RGB_PALE_TURQUOISE		175,238,238
#define RGB_AQUA_MARINE			127,255,212
#define RGB_POWDER_BLUE			176,224,230
#define RGB_CADET_BLUE			95,158,160
#define RGB_STEEL_BLUE			70,130,180
#define RGB_CORN_FLOWER_BLUE	100,149,237
#define RGB_DEEP_SKY_BLUE		0,191,255
#define RGB_DODGER_BLUE			30,144,255
#define RGB_LIGHT_BLUE			173,216,230
#define RGB_SKY_BLUE			135,206,235
#define RGB_LIGHT_SKY_BLUE		135,206,250
#define RGB_MIDNIGHT_BLUE		25,25,112
#define RGB_NAVY				0,0,128
#define RGB_DARK_BLUE			0,0,139
#define RGB_MEDIUM_BLUE			0,0,205
#define RGB_BLUE				0,0,255
#define RGB_ROYAL_BLUE			65,105,225
#define RGB_BLUE_VIOLET			138,43,226
#define RGB_INDIGO				75,0,130
#define RGB_DARK_SLATE_BLUE		72,61,139
#define RGB_SLATE_BLUE			106,90,205
#define RGB_MEDIUM_SLATE_BLUE	123,104,238
#define RGB_MEDIUM_PURPLE		147,112,219
#define RGB_DARK_MAGENTA		139,0,139
#define RGB_DARK_VIOLET			148,0,211
#define RGB_DARK_ORCHID			153,50,204
#define RGB_MEDIUM_ORCHID		186,85,211
#define RGB_PURPLE				128,0,128
#define RGB_THISTLE				216,191,216
#define RGB_PLUM				221,160,221
#define RGB_VIOLET				238,130,238
#define RGB_FUCHSIA				255,0,255
#define RGB_ORCHID				218,112,214
#define RGB_MEDIUM_VIOLET_RED	199,21,133
#define RGB_PALE_VIOLET_RED		219,112,147
#define RGB_DEEP_PINK			255,20,147
#define RGB_HOT_PINK			255,105,180
#define RGB_LIGHT_PINK			255,182,193
#define RGB_PINK				255,192,203
#define RGB_ANTIQUE_WHITE		250,235,215
#define RGB_BEIGE				245,245,220
#define RGB_BISQUE				255,228,196
#define RGB_BLANCHED_ALMOND		255,235,205
#define RGB_WHEAT				245,222,179
#define RGB_CORN_SILK			255,248,220
#define RGB_LEMON_CHIFFON		255,250,205
#define RGB_LIGHT_GOLDEN_ROD_YELLOW	250,250,210
#define RGB_LIGHT_YELLOW		255,255,224
#define RGB_SADDLE_BROWN		139,69,19
#define RGB_SIENNA				160,82,45
#define RGB_CHOCOLATE			210,105,30
#define RGB_PERU				205,133,63
#define RGB_SANDY_BROWN			244,164,96
#define RGB_BURLY_WOOD			222,184,135
#define RGB_TAN					210,180,140
#define RGB_ROSY_BROWN			188,143,143
#define RGB_MOCCASIN			255,228,181
#define RGB_NAVAJO_WHITE		255,222,173
#define RGB_PEACH_PUFF			255,218,185
#define RGB_MISTY_ROSE			255,228,225
#define RGB_LAVENDER_BLUSH		255,240,245
#define RGB_LINEN				250,240,230
#define RGB_OLD_LACE			253,245,230
#define RGB_PAPAYA_WHIP			255,239,213
#define RGB_SEA_SHELL			255,245,238
#define RGB_MINT_CREAM			245,255,250
#define RGB_SLATE_GRAY			112,128,144
#define RGB_LIGHT_SLATE_GRAY	119,136,153
#define RGB_LIGHT_STEEL_BLUE	176,196,222
#define RGB_LAVENDER			230,230,250
#define RGB_FLORAL_WHITE		255,250,240
#define RGB_ALICE_BLUE			240,248,255
#define RGB_GHOST_WHITE			248,248,255
#define RGB_HONEYDEW			240,255,240
#define RGB_IVORY				255,255,240
#define RGB_AZURE				240,255,255
#define RGB_SNOW				255,250,250
#define RGB_BLACK				0,0,0
#define RGB_DIM_GRAY			105,105,105
#define RGB_GREY				128,128,128
#define RGB_DARK_GREY			169,169,169
#define RGB_SILVER				192,192,192
#define RGB_LIGHT_GREY			211,211,211
#define RGB_GAINSBORO			220,220,220
#define RGB_WHITE_SMOKE			245,245,245
#define RGB_WHITE				255,255,255

#define item_name_limit 	50
#define none_selected 		-1
#define no_menu 			-1
#define max_font_name_length 60

typedef struct Button {
	char *label;
	int result;
} Button;

#if defined(__APPLE__)
	/* Platform Window type */
typedef id wnd_t;
#elif defined(_WIN32)
	/* Platform Window type */
typedef HWND wnd_t;
#else
	/* Platform Window type */
typedef Widget wnd_t;
#endif

typedef struct {
	/* `Application` main Window handle */
	wnd_t wnd;
	/* App's per `Window` handle */
	void *app_data;
	/* App's per `Window` title */
	const char *name;
	/* App's per `Window` close code */
	unsigned long code;
} actionbar_t;

typedef void (*_menu_cb)(actionbar_t *, void *data);
typedef struct {
	short menu_id;
	char *item_name;
	_menu_cb action;
	char alphaKey;
	void *data;
} menuitem_t;

typedef struct menu_s {
	menuitem_t *items;	// menu items
	int num_items;	// # of menu items
	int active;
	int selected;
	int menu_id;
	char menu_name[32];
	double x_start;
	double x_end;
	double width;
} menu_t;

typedef struct {
#if defined(__APPLE__)
#elif defined(_WIN32)
#else
	XWindowAttributes gwa;
	int state;
	menu_t *menus;
	int num_menus;
	char **font_names;
	int num_fonts;
	int *size;
	GLuint *font_lists;
	XFontStruct *font_info;
#endif
} menu_bar_t;

typedef struct gui_info_s {
	menu_bar_t *bar_info;
	int width;
	int height;
	int x;
	int y;
	int mouse;
	int error;
	int mod;       /* mod is 4 bits mask, ctrl=1, shift=2, alt=4, meta=8 */
	int keys[256]; /* keys are mostly ASCII, but arrows are 17..20 */
	const char *title;
	uint32_t *buf;
	wnd_t wnd;
#if !defined(__APPLE__) && !defined(_WIN32)
	Colormap cmap;
	Window win, root;
	Atom wmDeleteMessage;
	XEvent xev;
	Widget topLevel;
	Display *dpy;
	XVisualInfo *vi;
	XtAppContext app_con;
	GLXContext glc;
	GC gc;
	XImage *img;
#endif
} gui_info;

#ifndef C_API
#	define C_API extern
#endif

C_API void gui_open_dialog(actionbar_t *filedialog, void *data);
C_API int gui_message_box(const char *title, const char *text, const Button *buttons, int numButtons);
C_API int gui_create_menu(gui_info *, int num_menu, int numFonts);
C_API void gui_free(gui_info *);
C_API int gui_font(gui_info *, int font_num, const char *font);
C_API int gui_queryfont(gui_info *);
C_API int gui_menu(gui_info *, int num_menu, menuitem_t *items, int number_items, int id, char *name);
C_API void gui_querymenu(gui_info *);
C_API int gui_handler(gui_info *);
C_API void gui_active(actionbar_t *window);
C_API void gui_cancel(wnd_t window);

C_API int gui_window(gui_info *ui, const char *title, const int width, const int height, uint32_t *buffer);
C_API int gui_loop(gui_info *);
C_API void gui_close(gui_info *);
C_API void gui_sleep(int64_t ms);
C_API int64_t gui_time(void);
#define gui_pixel(w, x, y) ((w)->buf[((y) * (w)->width) + (x)])
#if defined(__APPLE__)
#define osx(r, o, s) ((r(*)(id, SEL))objc_msgSend)(o, sel_getUid(s))
#define osx1(r, o, s, A, a)                                                    \
  ((r(*)(id, SEL, A))objc_msgSend)(o, sel_getUid(s), a)
#define osx2(r, o, s, A, a, B, b)                                              \
  ((r(*)(id, SEL, A, B))objc_msgSend)(o, sel_getUid(s), a, b)
#define osx3(r, o, s, A, a, B, b, C, c)                                        \
  ((r(*)(id, SEL, A, B, C))objc_msgSend)(o, sel_getUid(s), a, b, c)
#define osx4(r, o, s, A, a, B, b, C, c, D, d)                                  \
  ((r(*)(id, SEL, A, B, C, D))objc_msgSend)(o, sel_getUid(s), a, b, c, d)

#define cocoa(x) ((id)objc_getClass(x))

C_API id const NSDefaultRunLoopMode;
C_API id const NSApp;
#endif

#endif /* _GUI_H */
