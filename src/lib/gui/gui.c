#include <gui.h>
#if defined(__APPLE__)
static void gui_draw_rect(id v, SEL s, CGRect r) {
	(void)r, (void)s;
	gui_info *ui = (gui_info *)objc_getAssociatedObject(v, "guiInfo");
	CGContextRef context =
		osx(CGContextRef, osx(id, cocoa("NSGraphicsContext"), "currentContext"),
			"graphicsPort");
	CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(
		NULL, ui->buf, ui->width * ui->height * 4, NULL);
	CGImageRef img =
		CGImageCreate(ui->width, ui->height, 8, 32, ui->width * 4, space,
			kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little,
			provider, NULL, false, kCGRenderingIntentDefault);
	CGColorSpaceRelease(space);
	CGDataProviderRelease(provider);
	CGContextDrawImage(context, CGRectMake(0, 0, ui->width, ui->height), img);
	CGImageRelease(img);
}

static BOOL gui_should_close(id v, SEL s, id w) {
	(void)v, (void)s, (void)w;
	osx1(void, NSApp, "terminate:", id, NSApp);
	return YES;
}

int gui_window(gui_info *ui, const char *title, const int width, const int height, uint32_t *buffer) {
	ui->title = title;
	ui->width = (int)width;
	ui->height = (int)height;
	ui->buf = buffer;
	osx(id, cocoa("NSApplication"), "sharedApplication");
	osx1(void, NSApp, "setActivationPolicy:", NSInteger, 0);
	ui->wnd = osx4(id, osx(id, cocoa("NSWindow"), "alloc"),
		"initWithContentRect:styleMask:backing:defer:", CGRect,
		CGRectMake(0, 0, ui->width, ui->height), NSUInteger, 3,
		NSUInteger, 2, BOOL, NO);
	Class windelegate =
		objc_allocateClassPair((Class)cocoa("NSObject"), "GuiDelegate", 0);
	class_addMethod(windelegate, sel_getUid("windowShouldClose:"),
		(IMP)gui_should_close, "c@:@");
	objc_registerClassPair(windelegate);
	osx1(void, ui->wnd, "setDelegate:", id,
		osx(id, osx(id, (id)windelegate, "alloc"), "init"));
	Class c = objc_allocateClassPair((Class)cocoa("NSView"), "GuiView", 0);
	class_addMethod(c, sel_getUid("drawRect:"), (IMP)gui_draw_rect, "i@:@@");
	objc_registerClassPair(c);

	id v = osx(id, osx(id, (id)c, "alloc"), "init");
	osx1(void, ui->wnd, "setContentView:", id, v);
	objc_setAssociatedObject(v, "guiInfo", (id)f, OBJC_ASSOCIATION_ASSIGN);

	id title = osx1(id, cocoa("NSString"), "stringWithUTF8String:", const char *,
		ui->title);
	osx1(void, ui->wnd, "setTitle:", id, title);
	osx1(void, ui->wnd, "makeKeyAndOrderFront:", id, nil);
	osx(void, ui->wnd, "center");
	osx1(void, NSApp, "activateIgnoringOtherApps:", BOOL, YES);
	return 0;
}

void gui_close(gui_info *ui) {
	osx(void, ui->wnd, "close");
}

// clang-format off
static const uint8_t _GUI_KEYCODES[128] = {65,83,68,70,72,71,90,88,67,86,0,66,81,87,69,82,89,84,49,50,51,52,54,53,61,57,55,45,56,48,93,79,85,91,73,80,10,76,74,39,75,59,92,44,47,78,77,46,9,32,96,8,0,27,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,2,3,127,0,5,0,4,0,20,19,18,17,0};
// clang-format on
int gui_loop(gui_info *ui) {
	osx1(void, osx(id, ui->wnd, "contentView"), "setNeedsDisplay:", BOOL, YES);
	id ev = osx4(id, NSApp,
		"nextEventMatchingMask:untilDate:inMode:dequeue:", NSUInteger,
		NSUIntegerMax, id, NULL, id, NSDefaultRunLoopMode, BOOL, YES);
	if (!ev)
		return 0;
	NSUInteger evtype = osx(NSUInteger, ev, "type");
	switch (evtype) {
		case 1: /* NSEventTypeMouseDown */
			ui->mouse |= 1;
			break;
		case 2: /* NSEventTypeMouseUp*/
			ui->mouse &= ~1;
			break;
		case 5:
		case 6: { /* NSEventTypeMouseMoved */
				CGPoint xy = osx(CGPoint, ev, "locationInWindow");
				ui->x = (int)xy.x;
				ui->y = (int)(ui->height - xy.y);
				return 0;
			}
		case 10: /*NSEventTypeKeyDown*/
		case 11: /*NSEventTypeKeyUp:*/ {
				NSUInteger k = osx(NSUInteger, ev, "keyCode");
				ui->keys[k < 127 ? _GUI_KEYCODES[k] : 0] = evtype == 10;
				NSUInteger mod = osx(NSUInteger, ev, "modifierFlags") >> 17;
				ui->mod = (mod & 0xc) | ((mod & 1) << 1) | ((mod >> 1) & 1);
				return 0;
			}
	}
	osx1(void, NSApp, "sendEvent:", id, ev);
	return 0;
}
#elif defined(_WIN32)
// clang-format off
static const uint8_t _GUI_KEYCODES[] = {0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,81,87,69,82,84,89,85,73,79,80,91,93,10,0,65,83,68,70,71,72,74,75,76,59,39,96,0,92,90,88,67,86,66,78,77,44,46,47,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,17,3,0,20,0,19,0,5,18,4,26,127};
// clang-format on
typedef struct BINFO {
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[3];
}BINFO;
static LRESULT CALLBACK gui_wndproc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam) {
	gui_info *ui = (gui_info *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg) {
		case WM_PAINT: {
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				HDC memdc = CreateCompatibleDC(hdc);
				HBITMAP hbmp = CreateCompatibleBitmap(hdc, ui->width, ui->height);
				HBITMAP oldbmp = SelectObject(memdc, hbmp);
				BINFO bi = {{sizeof(bi), ui->width, -ui->height, 1, 32, BI_BITFIELDS}};
				bi.bmiColors[0].rgbRed = 0xff;
				bi.bmiColors[1].rgbGreen = 0xff;
				bi.bmiColors[2].rgbBlue = 0xff;
				SetDIBitsToDevice(memdc, 0, 0, ui->width, ui->height, 0, 0, 0, ui->height,
					ui->buf, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
				BitBlt(hdc, 0, 0, ui->width, ui->height, memdc, 0, 0, SRCCOPY);
				SelectObject(memdc, oldbmp);
				DeleteObject(hbmp);
				DeleteDC(memdc);
				EndPaint(hwnd, &ps);
			} break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			ui->mouse = (msg == WM_LBUTTONDOWN);
			break;
		case WM_MOUSEMOVE:
			ui->y = HIWORD(lParam), ui->x = LOWORD(lParam);
			break;
		case WM_KEYDOWN:
		case WM_KEYUP: {
				ui->mod = ((GetKeyState(VK_CONTROL) & 0x8000) >> 15) |
					((GetKeyState(VK_SHIFT) & 0x8000) >> 14) |
					((GetKeyState(VK_MENU) & 0x8000) >> 13) |
					(((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) >> 12);
				ui->keys[_GUI_KEYCODES[HIWORD(lParam) & 0x1ff]] = !((lParam >> 31) & 1);
			} break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int gui_window(gui_info *ui, const char *title, const int width, const int height, uint32_t *buffer) {
	ui->title = title;
	ui->width = (int)width;
	ui->height = (int)height;
	ui->buf = buffer;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = gui_wndproc;
	wc.hInstance = hInstance;
	wc.lpszClassName = ui->title;
	RegisterClassEx(&wc);
	ui->hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, ui->title, ui->title,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		ui->width, ui->height, NULL, NULL, hInstance, NULL);

	if (ui->hwnd == NULL)
		return -1;
	SetWindowLongPtr(ui->hwnd, GWLP_USERDATA, (LONG_PTR)f);
	ShowWindow(ui->hwnd, SW_NORMAL);
	UpdateWindow(ui->hwnd);
	return 0;
}

void gui_close(gui_info *ui) { (void)ui; }

int gui_loop(gui_info *ui) {
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT)
			return -1;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	InvalidateRect(ui->hwnd, NULL, TRUE);
	return 0;
}
#else

#define GetNumMenus(X) 		X->num_menus
#define GetNumFonts(X) 		X->num_fonts
#define LineHeight(X,Y,Z) 	((double)(X->size[2]+X->size[3])*(Y-Z))/(double)X->gwa.height
#define MenuName(X,Y) 		X->menus[Y].menu_name
#define MenuStart(X,Y) 		x_left+(X->menus[Y].x_start/(double)X->gwa.width)*(x_right-x_left)
#define MenuWidth(X,Y) 		((double)X->menus[Y].width/(double)window_info->gwa.width)*(x_right-x_left)
#define GetState(X) 		(X->state-1)

#define x_left -1
#define x_right 8.2
#define y_bot -1
#define y_top 17
#define left_click Button1
#define right_click Button3
#define open_menu 1
#define close_menu 3
#define run_command 4
#define left_menu_padding 10
#define top_menu_padding 0.1

struct Dimensions {
	//window
	unsigned int winMinWidth;
	unsigned int winMinHeight;
	//vertical space between lines
	unsigned int lineSpacing;
	unsigned int barHeight;
	//padding
	unsigned int pad_up;
	unsigned int pad_down;
	unsigned int pad_left;
	unsigned int pad_right;
	//button
	unsigned int btSpacing;
	unsigned int btMinWidth;
	unsigned int btMinHeight;
	unsigned int btLateralPad;
};

typedef struct ButtonData {
	const Button *button;
	GC *gc;
	XRectangle rect;
} ButtonData;

//these values can be changed to whatever you prefer
struct Dimensions dim = {400, 150, 5, 40, 25, 10, 30, 30, 20, 75, 25, 8};

static int load_font(XFontStruct *font_info, char *font, Display *dpy, GLuint font_base, int *size) {
	font_info = XLoadQueryFont(dpy, font);
	size[0] = font_info->ascent;
	size[1] = font_info->descent;
	if (!font_info) {
		return 1;
	}

	glXUseXFont(font_info->fid, font_info->min_char_or_byte2, font_info->max_char_or_byte2 - font_info->min_char_or_byte2 + 1, font_base + font_info->min_char_or_byte2);
	return 0;
}

static int print_string(GLuint font_base, char *s) {
	if (!glIsList(font_base)) {
		return 1;
	}
	glPushAttrib(GL_LIST_BIT);
	glListBase(font_base);
	glCallLists(strlen(s), GL_UNSIGNED_BYTE, (GLubyte *)s);
	glPopAttrib();
	return 0;
}

static int drawMenubar(menu_bar_t *window_info) {
	int index = 0, error = 0;
	double x, y;
	y = y_top - ((double)window_info->size[2] / (double)window_info->gwa.height) * (y_top - y_bot);
	while (index < (window_info->num_menus)) {
		glColor3f(1, 1, 1);
		x = MenuStart(window_info, index);
		glRasterPos2f(x, y);
		error += print_string(window_info->font_lists[1], MenuName(window_info, index));
		++index;
	}

	y = y_top - LineHeight(window_info, y_top, y_bot);
	glColor3f(0.6, 0.6, 0.6);
	glBegin(GL_POLYGON);
	glVertex3f(x_left, y_top, 0);  glVertex3f(x_right, y_top, 0);
	glVertex3f(x_right, y, 0);   glVertex3f(x_left, y, 0);
	glEnd();
	return error;
}

static int drawMenu(menu_bar_t *window_info) {
	int error = 0, index = 0;
	double y, line_height, x;
	line_height = LineHeight(window_info, y_top, y_bot);
	y = (double)(2 * window_info->size[2] + window_info->size[3]);
	y = y * (y_top - y_bot);
	y = y / (double)window_info->gwa.height;
	y = y_top - y;
	x = MenuStart(window_info, GetState(window_info));
	while (index < (window_info->menus[GetState(window_info)].num_items)) {
		if (index == window_info->menus[GetState(window_info)].active) {
			glColor3f(0, 0, 0);
		} else {
			glColor3f(1, 1, 1);
		}
		glRasterPos2f(x, y);
		error += print_string(window_info->font_lists[1], window_info->menus[GetState(window_info)].items[index].item_name);
		y -= (line_height);
		++index;
	}

	glColor3f(0.6, 0.6, 0.6);
	y = y_top - (window_info->menus[GetState(window_info)].num_items + 1) * line_height;
	glBegin(GL_POLYGON);
	glVertex3f(x, y, 0);
	glVertex3f(x + MenuWidth(window_info, GetState(window_info)) + 0.1, y, 0);
	glVertex3f(x + MenuWidth(window_info, GetState(window_info)) + 0.1, y_top - line_height, 0);
	glVertex3f(x, y_top - line_height, 0);
	glEnd();
	return error;
}

static int draw(menu_bar_t *window_info) {
	int error = 0;
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(x_left, x_right, y_bot, y_top, 1., 20.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

	error = drawMenubar(window_info);

	if (window_info->state != 0) {
		error += drawMenu(window_info);
	}

	return error;
}

static int whichMenu(menu_bar_t *window_info, double x) {
	int index = window_info->num_menus - 1;
	while (index > 0 && (MenuStart(window_info, index) > x)) {
		--index;
	}
	return index + 1;
}

static int whichActive(menu_bar_t *window_info, double x, double y) {
	int index = 0;
	double y_cat, line_height, x_min, x_max;
	line_height = LineHeight(window_info, y_top, y_bot);
	y_cat = y_top - line_height;
	x_min = MenuStart(window_info, GetState(window_info));
	x_max = x_min + MenuWidth(window_info, GetState(window_info));
	if (y > y_cat || x < x_min || x > x_max) {
		return -1;
	}
	y_cat -= line_height;
	while (x > x_min && x < x_max && y < y_cat && index < window_info->menus[GetState(window_info)].num_items) {
		y_cat -= (line_height);
		++index;
	}
	return (index != window_info->menus[GetState(window_info)].num_items) ? index : -2;
}

static void WMProtocols(Widget w, XEvent *ev, String *params, Cardinal *nparams) {
	if (ev->type == ClientMessage
		&& !strcmp(XGetAtomName(XtDisplay(w), ev->xclient.message_type), "WM_PROTOCOLS")) {
	}
}

static Atom active_wm = 0;

static void ui_open_cb(Widget cmd, XtPointer client, XtPointer call_data) {
	Widget topLevel = XtParent(cmd);
	if (call_data == NULL) {
		gui_cancel(cmd);
		return;
	}

	//XtResizeWidget(cmd, 600, 600, 0);
	Widget fileText = XtVaCreateManagedWidget("fileText", asciiTextWidgetClass,
		topLevel,
		XtNheight, 600,
		XtNwidth, 600,
		XtNtype, XawAsciiFile,
		XtNstring, call_data,
		XtNeditType, XawtextEdit,
		XtNresize, XawtextResizeBoth,
		XtNscrollHorizontal, XawtextScrollWhenNeeded,
		XtNscrollVertical, XawtextScrollAlways,
		NULL);

	Display *ldpy = XtDisplayOfObject(fileText);
	Window win = XtWindow(topLevel);
	XStoreName(ldpy, win, call_data);
	XMapWindow(ldpy, win);
}

void gui_open_dialog(actionbar_t *filedialog, void *data) {
	char *filter = "*";
	char *dir = "./";
	char *initial = "";

	int argc = 0;
	char **argv = NULL;
	XtAppContext app_ctx;

	static char *fallback_resources[] = {
	  "*variablewidth*font: -adobe-helvetica-medium-r-normal--*-120-*",
	  "*monospaced*font: -*-courier-medium-r-*-*-14-*-*-*-*-*-*",
	  "<Message>WM_PROTOCOLS: WMProtocols()\n",
	  NULL
	};

	Widget topLevel = XtAppInitialize(&app_ctx, "FilePrompt", NULL, 0,
		&argc, argv, fallback_resources, NULL, 0);
	XtResizeWidget(topLevel, 400, 400, 0);

	Widget fileSelect = XtVaCreateManagedWidget("fileSelector",
		fileSelectWidgetClass, topLevel, NULL, 0);
	if (data)
		XtAddCallback(fileSelect, XtNcallback, (XtCallbackProc)data, 0);
	else
		XtAddCallback(fileSelect, XtNcallback, ui_open_cb, 0);

	FileSelectSet(fileSelect, dir, filter, initial);
	filedialog->app_data = (void *)fileSelect;
	filedialog->name = "Select File";
	filedialog->wnd = topLevel;
	gui_active(filedialog);
	XtDestroyApplicationContext(app_ctx);
}

void gui_cancel(wnd_t window) {
	Display *disp = XtDisplayOfObject(window);
	Window win = XtWindow(window);
	XEvent event;
	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.message_type = active_wm;
	event.xclient.window = win;
	event.xclient.format = 32;
	event.xclient.data.l[0] = active_wm;
	XSendEvent(disp, win, False, 0, &event);
	XSync(disp, False);
}

void gui_active(actionbar_t *window) {
	Display *ldpy = XtDisplayOfObject((Widget)window->app_data);
	XtAppContext context = XtWidgetToApplicationContext((Widget)window->app_data);
	XtRealizeWidget(window->wnd);

	XtActionsRec fileprompt_actions[] = {
		{"WMProtocols", WMProtocols},
	};

	XtAppAddActions(context,
		fileprompt_actions, XtNumber(fileprompt_actions));

	/* set up to handle quits */
	XInternAtom(ldpy, "WM_PROTOCOLS", False);
	window->code = XInternAtom(ldpy, "WM_DELETE_WINDOW", False);
	XtOverrideTranslations(window->wnd,
		XtParseTranslationTable("<Message>WM_PROTOCOLS: WMProtocols()"));

	Window win = XtWindow(window->wnd);
	XStoreName(ldpy, win, window->name);
	XMapWindow(ldpy, win);

	(void)XSetWMProtocols(ldpy, win, (Atom *)&window->code, 1);
	active_wm = window->code;
	XEvent ev;
	for (;;) {
		XtAppNextEvent(context, &ev);
		XtDispatchEvent(&ev);
		if (ev.xclient.type == ClientMessage && ev.xclient.data.l[0] == window->code)
			break;
	}
	XtUnrealizeWidget(window->wnd);
}

int gui_menu(gui_info *ui, int num_menu, menuitem_t *items, int number_items, int id, char *name) {
	int r;
	menu_t *menu = &(ui->bar_info->menus[num_menu]);
	menu->num_items = number_items;
	menu->selected = none_selected;
	menu->menu_id = id;
	menu->items = items;
	//menu->items = (menuitem_t *)calloc(1, menu->num_items * sizeof(menuitem_t));
	//if (memcpy(menu->items, items, menu->num_items * sizeof(menuitem_t)) == NULL) {
	//	return 0;
	//}

	if (!(r = snprintf(menu->menu_name, sizeof(menu->menu_name) - 1, "%s", name)))
		XtAppError(ui->app_con, "\tMenu failed\n");

	return r;
}

void gui_querymenu(gui_info *ui) {
	int error = 0, index = 0, item_index = 0;
	double x = 0, width = 0, temp;
	XFontStruct *menu_font = XLoadQueryFont(ui->dpy, ui->bar_info->font_names[1]);

	while (index < (ui->bar_info->num_menus)) {
		ui->bar_info->menus[index].x_start = x;
		x += XTextWidth(menu_font, ui->bar_info->menus[index].menu_name, strlen(ui->bar_info->menus[index].menu_name));
		ui->bar_info->menus[index].x_end = x;
		x += left_menu_padding;
		while (item_index < ui->bar_info->menus[index].num_items) {
			temp = XTextWidth(menu_font, ui->bar_info->menus[index].items[item_index].item_name, strlen(ui->bar_info->menus[index].items[item_index].item_name));
			width = (temp > width) ? temp : width;
			++item_index;
		}item_index = 0;
		ui->bar_info->menus[index].width = width;
		++index;
	}
}

int gui_font(gui_info *ui, int font_num, const char *font) {
	memcpy(ui->bar_info->font_names[font_num], font, strlen(font));
	ui->bar_info->font_names[font_num][strlen(font)] = '\0';
}

int gui_queryfont(gui_info *ui) {
	ui->bar_info->font_lists[0] = glGenLists(256);
	if (!glIsList(ui->bar_info->font_lists[0])) {
		fprintf(stdout, "\tfont list failure\n");
		return 0;
	}

	ui->bar_info->font_lists[1] = glGenLists(256);
	if (!glIsList(ui->bar_info->font_lists[1])) {
		fprintf(stdout, "\tfont list failure\n");
		return 0;
	}

	if (load_font(ui->bar_info->font_info, ui->bar_info->font_names[0],
		ui->dpy, ui->bar_info->font_lists[0], ui->bar_info->size) != 0) {
		fprintf(stdout, "\tfont load failure\n");
		return 0;
	}

	if (load_font(&(ui->bar_info->font_info[1]), ui->bar_info->font_names[1],
		ui->dpy, ui->bar_info->font_lists[1], &(ui->bar_info->size[2])) != 0) {
		fprintf(stdout, "\tfont load failure\n");
		return 0;
	}

	return 1;
}

void gui_free(gui_info *ui) {
	int i;
	if (ui) {
		if (!ui->buf) {
			if (ui->bar_info) {
				for (i = 0; i < ui->bar_info->num_fonts; i++)
					free(ui->bar_info->font_names[i]);
				free(ui->bar_info->font_names);
				free(ui->bar_info->menus);
				free(ui->bar_info->font_lists);
				free(ui->bar_info->font_info);
				free(ui->bar_info->size);
				free(ui->bar_info);
				ui->bar_info = NULL;
			}

			if (ui->glc) {
				glXMakeCurrent(ui->dpy, None, NULL);
				glXDestroyContext(ui->dpy, ui->glc);
			}

			XDestroyWindow(ui->dpy, ui->win);
		}

		XCloseDisplay(ui->dpy);
		ui = NULL;
	}
}

int gui_create_menu(gui_info *ui, int num_menu, int numFonts) {
	int index = 0;
	if ((ui->bar_info = (menu_bar_t *)calloc(1, sizeof(menu_bar_t)))) {
		ui->bar_info->num_menus = num_menu;
		ui->bar_info->menus = (menu_t *)calloc(1, GetNumMenus(ui->bar_info) * sizeof(menu_t));
		ui->bar_info->num_fonts = numFonts;
		ui->bar_info->font_lists = (GLuint *)malloc(GetNumFonts(ui->bar_info) * sizeof(GLuint));
		ui->bar_info->font_info = (XFontStruct *)malloc(GetNumFonts(ui->bar_info) * sizeof(XFontStruct));
		ui->bar_info->size = (int *)malloc(GetNumFonts(ui->bar_info) * 2 * sizeof(int));
		ui->bar_info->state = 0;
		ui->bar_info->font_names = (char **)malloc(GetNumFonts(ui->bar_info) * sizeof(char *));
		while (index < ui->bar_info->num_fonts) {
			ui->bar_info->font_names[index] = (char *)malloc(max_font_name_length * sizeof(char));
			++index;
		}

		return 1;
	}

	return 0;
}

int gui_handler(gui_info *ui) {
	int buttoncase, click_x, click_y, error, check;
	double x_limit, y_limit, x, y;
	actionbar_t menu[1];
	glEnable(GL_DEPTH_TEST);
	while (1) {
		XNextEvent(ui->dpy, &ui->xev);
		switch (ui->xev.type) {
			case MotionNotify:
				if (ui->bar_info->state != 0) {
					x = x_left + ((double)ui->xev.xmotion.x / (double)ui->bar_info->gwa.width) * (x_right - x_left);
					y = y_top - ((double)ui->xev.xmotion.y / (double)ui->bar_info->gwa.height) * (y_top - y_bot);
					check = ui->bar_info->menus[GetState(ui->bar_info)].active;
					ui->bar_info->menus[GetState(ui->bar_info)].active = whichActive(ui->bar_info, x, y);
					if (check != ui->bar_info->menus[GetState(ui->bar_info)].active) {
						error = draw(ui->bar_info);
						glXSwapBuffers(ui->dpy, ui->win);
					}
				}
				break;
			case ButtonPress:
				x_limit = x_left + (ui->bar_info->menus[ui->bar_info->num_menus - 1].x_end / (double)ui->bar_info->gwa.width) * (x_right - x_left);
				y_limit = y_top - LineHeight(ui->bar_info, y_top, y_bot);
				x = x_left + ((double)ui->xev.xbutton.x / (double)ui->bar_info->gwa.width) * (x_right - x_left);
				click_x = (x < 0) ? -1 : (int)x;
				y = y_bot + ((double)(ui->bar_info->gwa.height) - (double)ui->xev.xbutton.y) * ((double)y_top - (double)y_bot) / ((double)ui->bar_info->gwa.height);
				click_y = (y < 0) ? -1 : (int)y;
				buttoncase = open_menu * (ui->xev.xbutton.button == left_click) * (ui->bar_info->state == 0) * (x < x_limit) * (y > y_limit);
				buttoncase += close_menu * (ui->bar_info->state != 0) * (whichActive(ui->bar_info, x, y) < 0);
				buttoncase += run_command * (ui->bar_info->state != 0) * (whichActive(ui->bar_info, x, y) >= 0);
				switch (buttoncase) {
					case open_menu:
						ui->bar_info->state = whichMenu(ui->bar_info, x);
						break;
					case close_menu:
						ui->bar_info->state = 0;
						break;
					case run_command:
						memset(menu, 0, sizeof(actionbar_t));
						menu->wnd = ui->topLevel;

						menuitem_t menu_active = ui->bar_info->menus[GetState(ui->bar_info)]
							.items[ui->bar_info->menus[GetState(ui->bar_info)].active];
						menu_active.action(menu, menu_active.data);
						break;
					default:
						break;
				}
			case Expose:
				XGetWindowAttributes(ui->dpy, ui->win, &(ui->bar_info->gwa));
				glViewport(0, 0, ui->bar_info->gwa.width, ui->bar_info->gwa.height);
				error = draw(ui->bar_info);
				if (error != 0) {
					fprintf(stdout, "\tfont failure: %d\n", error);
					return 0;
				}
				glXSwapBuffers(ui->dpy, ui->win);
				break;
			case ClientMessage:
				if (ui->xev.xclient.data.l[0] == ui->wmDeleteMessage) {
					return 0;
				}
				break;
			default:
				break;
		}
	}
	return 0;
}

static void setWindowTitle(const char *title, const Window *win, Display *dpy) {
	Atom wm_Name = XInternAtom(dpy, "_NET_WM_NAME", False);
	Atom utf8Str = XInternAtom(dpy, "UTF8_STRING", False);

	Atom winType = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	Atom typeDialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);

	XChangeProperty(dpy, *win, wm_Name, utf8Str, 8, PropModeReplace, (const unsigned char *)title, (int)strlen(title));
	XChangeProperty(dpy, *win, winType, XA_ATOM,
		32, PropModeReplace, (unsigned char *)&typeDialog,
		1);
}

static void split(const char *text, const char *seps, char ***str, int *count) {
	char *last, *tok, *data;
	int i;
	*count = 0;
	data = strdup(text);

	for (tok = strtok(data, seps); tok != NULL; tok = strtok(NULL, seps))
		(*count)++;

	free(data);
	fflush(stdout);
	data = strdup(text);
	*str = (char **)malloc((size_t)(*count) * sizeof(char *));

	for (i = 0, tok = strtok(data, seps); tok != NULL; tok = strtok(NULL, seps), i++)
		(*str)[i] = strdup(tok);
	free(data);
}

static void computeTextSize(XFontSet *fs, char **texts, int size, unsigned int spaceBetweenLines,
	unsigned int *w, unsigned  int *h) {
	int i;
	XRectangle rect = {0,0,0,0};
	*h = 0;
	*w = 0;
	for (i = 0; i < size; i++) {
		Xutf8TextExtents(*fs, texts[i], (int)strlen(texts[i]), &rect, NULL);
		*w = (rect.width > *w) ? (rect.width) : *w;
		*h += rect.height + spaceBetweenLines;
		fflush(stdin);
	}
}

static void createGC(GC *gc, const Colormap *cmap, Display *dpy, const  Window *win,
	unsigned char red, unsigned char green, unsigned char blue) {
	float coloratio = (float)65535 / 255;
	XColor color;
	*gc = XCreateGC(dpy, *win, 0, 0);
	memset(&color, 0, sizeof(color));
	color.red = (unsigned short)(coloratio * red);
	color.green = (unsigned short)(coloratio * green);
	color.blue = (unsigned short)(coloratio * blue);
	color.flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, *cmap, &color);
	XSetForeground(dpy, *gc, color.pixel);
}

static bool isInside(int x, int y, XRectangle rect) {
	if (x < rect.x || x >(rect.x + rect.width) || y < rect.y || y >(rect.y + rect.height))
		return false;
	return true;
}

int gui_message_box(const char *title, const char *text, const Button *buttons, int numButtons) {
	// convert the text in list (to draw in multiply lines)
	char **text_splitted = NULL;
	int textLines = 0;
	split(text, "\n", &text_splitted, &textLines);

	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Error opening display display.");
	}

	int ds = DefaultScreen(dpy);
	Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, ds), 0, 10, 400, 120, 0,
		BlackPixel(dpy, ds), WhitePixel(dpy, ds));

	XSelectInput(dpy, win, ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
	XMapWindow(dpy, win);

	//allow windows to be closed by pressing cross button (but it wont close - see ClientMessage on switch)
	Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &WM_DELETE_WINDOW, 1);

	// create the gc for drawing text
	XGCValues gcValues;
	gcValues.font = XLoadFont(dpy, "7x13");
	gcValues.foreground = BlackPixel(dpy, ds);
	GC textGC = XCreateGC(dpy, win, GCFont + GCForeground, &gcValues);
	XUnmapWindow(dpy, win);

	// create fontset
	char **missingCharset_list = NULL;
	int i, missingCharset_count = 0;
	XFontSet fs;
	fs = XCreateFontSet(dpy,
		"-*-*-medium-r-*-*-*-140-75-75-*-*-*-*",
		&missingCharset_list, &missingCharset_count, NULL);

	if (missingCharset_count) {
		fprintf(stderr, "Missing charsets :\n");
		for (i = 0; i < missingCharset_count; i++) {
			fprintf(stderr, "%s\n", missingCharset_list[i]);
		}
		XFreeStringList(missingCharset_list);
		missingCharset_list = NULL;
	}

	Colormap cmap = DefaultColormap(dpy, ds);

	//resize the window according to the text size
	unsigned int winW, winH;
	unsigned int textW, textH;

	//calculate the ideal window's size
	computeTextSize(&fs, text_splitted, textLines, dim.lineSpacing, &textW, &textH);
	unsigned int newWidth = textW + dim.pad_left + dim.pad_right;
	unsigned int newHeight = textH + dim.pad_up + dim.pad_down + dim.barHeight;
	winW = (newWidth > dim.winMinWidth) ? newWidth : dim.winMinWidth;
	winH = (newHeight > dim.winMinHeight) ? newHeight : dim.winMinHeight;

	//set windows hints
	XSizeHints hints;
	hints.flags = PSize | PMinSize | PMaxSize;
	hints.min_width = hints.max_width = hints.base_width = winW;
	hints.min_height = hints.max_height = hints.base_height = winH;

	XSetWMNormalHints(dpy, win, &hints);
	XMapRaised(dpy, win);

	GC barGC;
	GC buttonGC;
	GC buttonGC_underPointer;
	GC buttonGC_onClick;                               // GC colors
	createGC(&barGC, &cmap, dpy, &win, RGB_WHITE);
	createGC(&buttonGC, &cmap, dpy, &win, RGB_GOLDEN_ROD);
	createGC(&buttonGC_underPointer, &cmap, dpy, &win, RGB_SILVER);
	createGC(&buttonGC_onClick, &cmap, dpy, &win, RGB_DIM_GRAY);

	/* setup the buttons data */
	ButtonData *btsData;
	btsData = (ButtonData *)malloc((size_t)numButtons * sizeof(ButtonData));

	int pass = 0;
	for (i = 0; i < numButtons; i++) {
		btsData[i].button = &buttons[i];
		btsData[i].gc = &buttonGC;
		XRectangle btTextDim;
		Xutf8TextExtents(fs, btsData[i].button->label, (int)strlen(btsData[i].button->label),
			&btTextDim, NULL);
		btsData[i].rect.width = (btTextDim.width < dim.btMinWidth) ? dim.btMinWidth :
			(btTextDim.width + 2 * dim.btLateralPad);
		btsData[i].rect.height = dim.btMinHeight;
		btsData[i].rect.x = winW - dim.pad_left - btsData[i].rect.width - pass;
		btsData[i].rect.y = textH + dim.pad_up + dim.pad_down + ((dim.barHeight - dim.btMinHeight) / 2);
		pass += btsData[i].rect.width + dim.btSpacing;
	}

	setWindowTitle(title, &win, dpy);
	XFlush(dpy);

	bool quit = false;
	int res = -1;

	while (!quit) {
		XEvent e;
		XNextEvent(dpy, &e);
		switch (e.type) {
			case MotionNotify:
			case ButtonPress:
			case ButtonRelease:
				for (i = 0; i < numButtons; i++) {
					btsData[i].gc = &buttonGC;
					if (isInside(e.xmotion.x, e.xmotion.y, btsData[i].rect)) {
						btsData[i].gc = &buttonGC_underPointer;
						if (e.type == ButtonPress && e.xbutton.button == Button1) {
							btsData[i].gc = &buttonGC_onClick;
							res = btsData[i].button->result;
							quit = true;
						}
					}
				}
			case Expose:
				// draw the text in multiply lines
				for (i = 0; i < textLines; i++) {
					Xutf8DrawString(dpy, win, fs, textGC, dim.pad_left, dim.pad_up + i * (dim.lineSpacing + 18),
						text_splitted[i], (int)strlen(text_splitted[i]));
				}

				XFillRectangle(dpy, win, barGC, 0, textH + dim.pad_up + dim.pad_down, winW, dim.barHeight);
				for (i = 0; i < numButtons; i++) {
					XFillRectangle(dpy, win, *btsData[i].gc, btsData[i].rect.x, btsData[i].rect.y,
						btsData[i].rect.width, btsData[i].rect.height);

					XRectangle btTextDim;
					Xutf8TextExtents(fs, btsData[i].button->label, (int)strlen(btsData[i].button->label),
						&btTextDim, NULL);
					Xutf8DrawString(dpy, win, fs, textGC,
						btsData[i].rect.x + (btsData[i].rect.width - btTextDim.width) / 2,
						btsData[i].rect.y + (btsData[i].rect.height + btTextDim.height) / 2,
						btsData[i].button->label, (int)strlen(btsData[i].button->label));
				}
				XFlush(dpy);
				break;
			case ClientMessage:
				break;
			default:
				break;
		}
	}

	for (i = 0; i < textLines; i++) {
		free(text_splitted[i]);
	}
	free(text_splitted);
	free(btsData);
	if (missingCharset_list)
		XFreeStringList(missingCharset_list);
	XDestroyWindow(dpy, win);
	XFreeFontSet(dpy, fs);
	XFreeGC(dpy, textGC);
	XFreeGC(dpy, barGC);
	XFreeGC(dpy, buttonGC);
	XFreeGC(dpy, buttonGC_underPointer);
	XFreeGC(dpy, buttonGC_onClick);
	XFreeColormap(dpy, cmap);
	XCloseDisplay(dpy);

	return res;
}

// clang-format off
static int _GUI_KEYCODES[124] = {XK_BackSpace,8,XK_Delete,127,XK_Down,18,XK_End,5,XK_Escape,27,XK_Home,2,XK_Insert,26,XK_Left,20,XK_Page_Down,4,XK_Page_Up,3,XK_Return,10,XK_Right,19,XK_Tab,9,XK_Up,17,XK_apostrophe,39,XK_backslash,92,XK_bracketleft,91,XK_bracketright,93,XK_comma,44,XK_equal,61,XK_grave,96,XK_minus,45,XK_period,46,XK_semicolon,59,XK_slash,47,XK_space,32,XK_a,65,XK_b,66,XK_c,67,XK_d,68,XK_e,69,XK_f,70,XK_g,71,XK_h,72,XK_i,73,XK_j,74,XK_k,75,XK_l,76,XK_m,77,XK_n,78,XK_o,79,XK_p,80,XK_q,81,XK_r,82,XK_s,83,XK_t,84,XK_u,85,XK_v,86,XK_w,87,XK_x,88,XK_y,89,XK_z,90,XK_0,48,XK_1,49,XK_2,50,XK_3,51,XK_4,52,XK_5,53,XK_6,54,XK_7,55,XK_8,56,XK_9,57};

static char *fallback[] = {
  "*variablewidth*font: -adobe-helvetica-medium-r-normal--*-120-*",
  "*monospaced*font: -*-courier-medium-r-*-*-14-*-*-*-*-*-*",
  "<Message>WM_PROTOCOLS: WMProtocols()\n",
  NULL
};

int gui_window(gui_info *ui, const char *title, const int width, const int height, uint32_t *buffer) {
	GLint att[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE,     24,
		GLX_STENCIL_SIZE,   8,
		GLX_RED_SIZE,       8,
		GLX_GREEN_SIZE,     8,
		GLX_BLUE_SIZE,      8,
		GLX_SAMPLE_BUFFERS, 0,
		GLX_SAMPLES,        0,
		None
	};
	int argc = 0;
	char **argv = NULL;

	ui->title = title;
	ui->width = (int)width;
	ui->height = (int)height;
	ui->buf = buffer;
	if (!ui->buf) {
		ui->wnd = XtOpenApplication(&ui->app_con, ui->title, NULL, 0, &argc, argv,
			fallback, sessionShellWidgetClass, NULL, 0);
		ui->dpy = XtDisplayOfObject(ui->wnd);
	} else {
		ui->dpy = XOpenDisplay(NULL);
	}

	if (ui->dpy == NULL) {
		printf("\n\tcannot connect to X server\n\n");
		return 0;
	}

	int screen = DefaultScreen(ui->dpy);
	if (!ui->buf) {
		ui->vi = glXChooseVisual(ui->dpy, screen, att);
		if (ui->vi == NULL) {
			printf("\n\tno appropriate visual found\n\n");
			return 0;
		}
	}

	ui->root = RootWindow(ui->dpy, screen);
	ui->win = XCreateSimpleWindow(ui->dpy, ui->root, 0, 0, ui->width, ui->height, 0,
		BlackPixel(ui->dpy, screen), WhitePixel(ui->dpy, screen));

	if (ui->buf)
		ui->gc = XCreateGC(ui->dpy, ui->win, 0, 0);

	XSelectInput(ui->dpy, ui->win,
		ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask);
	ui->wmDeleteMessage = XInternAtom(ui->dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(ui->dpy, ui->win, &ui->wmDeleteMessage, 1);
	XStoreName(ui->dpy, ui->win, ui->title);
	XMapWindow(ui->dpy, ui->win);

	if (!ui->buf) {
		XSync(ui->dpy, 0);
		ui->glc = glXCreateContext(ui->dpy, ui->vi, NULL, GL_TRUE);
		glXMakeCurrent(ui->dpy, ui->win, ui->glc);
		ui->topLevel = XtAppCreateShell("main", NULL, applicationShellWidgetClass, ui->dpy, NULL, 0);
		XtResizeWidget(ui->topLevel, 310, 110, 1);
	} else {
		XSync(ui->dpy, ui->win);
		ui->img = XCreateImage(ui->dpy, DefaultVisual(ui->dpy, 0), 24, ZPixmap, 0,
			(char *)ui->buf, ui->width, ui->height, 32, 0);
	}

	return 1;
}

void gui_close(gui_info *ui) {
	gui_free(ui);
}

int gui_loop(gui_info *ui) {
	XEvent ev;
	unsigned int i;
	XPutImage(ui->dpy, ui->win, ui->gc, ui->img, 0, 0, 0, 0, ui->width, ui->height);
	XFlush(ui->dpy);
	while (XPending(ui->dpy)) {
		XNextEvent(ui->dpy, &ev);
		switch (ev.type) {
			case ClientMessage:
				if (ev.xclient.data.l[0] == ui->wmDeleteMessage)
					return -(ClientMessage);
				break;
			case ButtonPress:
			case ButtonRelease:
				ui->mouse = (ev.type == ButtonPress);
				break;
			case MotionNotify:
				ui->x = ev.xmotion.x, ui->y = ev.xmotion.y;
				break;
			case KeyPress:
			case KeyRelease: {
				int m = ev.xkey.state;
				int k = XkbKeycodeToKeysym(ui->dpy, ev.xkey.keycode, 0, 0);
				for (i = 0; i < 124; i += 2) {
					if (_GUI_KEYCODES[i] == k) {
						ui->keys[_GUI_KEYCODES[i + 1]] = (ev.type == KeyPress);
						break;
					}
				}
				ui->mod = (!!(m & ControlMask)) | (!!(m & ShiftMask) << 1)
					| (!!(m & Mod1Mask) << 2) | (!!(m & Mod4Mask) << 3);
			} break;
		}
	}
	return 0;
}
#endif

#ifdef _WIN32
void gui_sleep(int64_t ms) { Sleep(ms); }
int64_t gui_time() {
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return (int64_t)(count.QuadPart * 1000.0 / freq.QuadPart);
}
#else
void gui_sleep(int64_t ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}
int64_t gui_time(void) {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return time.tv_sec * 1000 + (time.tv_nsec / 1000000);
}
#endif
