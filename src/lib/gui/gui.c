#include <gui.h>
static volatile gui_info *main_gui_info = NULL;
static volatile bool main_gui_shutdown = false;
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
#define ID_WINDOW_ICON	900

// clang-format off
static const uint8_t _GUI_KEYCODES[] = {0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,81,87,69,82,84,89,85,73,79,80,91,93,10,0,65,83,68,70,71,72,74,75,76,59,39,96,0,92,90,88,67,86,66,78,77,44,46,47,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,17,3,0,20,0,19,0,5,18,4,26,127};
// clang-format on
typedef struct BINFO {
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[3];
}BINFO;

static LRESULT CALLBACK gui_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	gui_info *ui = (gui_info *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	static HMENU hmenu;             // handle to main menu
	static COLORREF crSelText;  // text color of selected item
	static COLORREF crSelBkgnd = RGB(173, 216, 230); // background color of selected item
	COLORREF crText;            // text color of unselected item
	COLORREF crBkgnd;           // background color unselected item
	LPMEASUREITEMSTRUCT lpmis;  // pointer to item of data
	LPDRAWITEMSTRUCT lpdis;     // pointer to item drawing data
	HDC hdc;                    // handle to screen DC
	SIZE size;                  // menu-item text extents
	WORD wCheckX;               // check-mark width
	int nTextX;                 // width of menu item
	int nTextY;                 // height of menu item
	int i;                      // loop counter
	HFONT hfontOld;             // handle to old font
	BOOL fSelected = FALSE;     // menu-item selection flag
	size_t length = 0;
	size_t *pcch = &length;
	HRESULT hResult;
	HMENU hCharacterMenu;
	menuitem_t *pmyitem;
	switch (msg) {
		case WM_MEASUREITEM:
			// Retrieve a device context for the main window.
			hdc = GetDC(hwnd);

			// Retrieve pointers to the menu item's
			// MEASUREITEMSTRUCT structure and MYITEM structure.
			lpmis = (LPMEASUREITEMSTRUCT)lParam;
			pmyitem = (menuitem_t *)lpmis->itemData;

			// Select the font associated with the item into
			// the main window's device context.
			hfontOld = (HFONT)SelectObject(hdc, pmyitem->hfont);

			// Retrieve the width and height of the item's string,
			// and then copy the width and height into the
			// MEASUREITEMSTRUCT structure's itemWidth and
			// itemHeight members.
			hResult = StringCchLength(pmyitem->item_name, STRSAFE_MAX_CCH, pcch);
			if (FAILED(hResult)) {
				// Add code to fail as securely as possible.
				return (LRESULT)0;
			}

			GetTextExtentPoint32(hdc, pmyitem->item_name, *pcch, &size);
			lpmis->itemWidth = size.cx;
			lpmis->itemHeight = size.cy;

			// Select the old font back into the device context,
			// and then release the device context.
			SelectObject(hdc, hfontOld);
			ReleaseDC(hwnd, hdc);
			return TRUE;
		case WM_DRAWITEM:
			// Get pointers to the menu item's DRAWITEMSTRUCT
			// structure and MYITEM structure.
			lpdis = (LPDRAWITEMSTRUCT)lParam;
			pmyitem = (menuitem_t *)lpdis->itemData;

			// If the user has selected the item, use the selected
			// text and background colors to display the item.
			if (lpdis->itemState & ODS_SELECTED) {
				crText = SetTextColor(lpdis->hDC, crSelText);
				crBkgnd = SetBkColor(lpdis->hDC, crSelBkgnd);
				fSelected = TRUE;
			}

			// Remember to leave space in the menu item for the
			// check-mark bitmap. Retrieve the width of the bitmap
			// and add it to the width of the menu item.
			wCheckX = GetSystemMetrics(SM_CXMENUCHECK);
			nTextX = wCheckX + lpdis->rcItem.left;
			nTextY = lpdis->rcItem.top;

			// Select the font associated with the item into the
			// item's device context, and then draw the string.
			hfontOld = (HFONT)SelectObject(lpdis->hDC, pmyitem->hfont);
			hResult = StringCchLength(pmyitem->item_name, STRSAFE_MAX_CCH, pcch);
			if (FAILED(hResult)) {
				// Add code to fail as securely as possible.
				return (LRESULT)0;
			}

			ExtTextOut(lpdis->hDC, nTextX, nTextY, ETO_OPAQUE,&lpdis->rcItem, pmyitem->item_name,*pcch, NULL);

			// Select the previous font back into the device
			// context.
			SelectObject(lpdis->hDC, hfontOld);

			// Return the text and background colors to their
			// normal state (not selected).
			if (fSelected) {
				SetTextColor(lpdis->hDC, crText);
				SetBkColor(lpdis->hDC, crBkgnd);
			}

			return TRUE;
		case WM_CLOSE:
			if (ui == main_gui_info)
				main_gui_shutdown = true;
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_COMMAND:
			ui_t menu[1];
			menuitem_t *menus_active;
			int i, x, count, which = LOWORD(wParam);
			for (i = 0; i < ui->bar_info->num_menus; i++) {
				count = ui->bar_info->menus[i].num_items;
				menus_active = ui->bar_info->menus[i].items;
				for (x = 0; x < count; x++) {
					if (which == menus_active[x].menu_id) {
						memset(menu, 0, sizeof(ui_t));
						menu->wnd = ui->wnd;
						menu->app_data = ui->hinst;
						menus_active[x].action(menu, menus_active[x].data);
						return 0;
					}
				}
			}
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static LRESULT CALLBACK gui_wndproc_arcade(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

static BOOL ui_open_cb(ui_t *edit, LPCTSTR pszFileName) {
	HANDLE hFile;
	BOOL bSuccess = FALSE;

	hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwFileSize;

		dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize != 0xFFFFFFFF) {
			LPSTR pszFileText;

			pszFileText = GlobalAlloc(GPTR, dwFileSize + 1);
			if (pszFileText != NULL) {
				DWORD dwRead;
				if (ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL)) {
					pszFileText[dwFileSize] = 0; // Add null terminator
					HWND hEdit = CreateWindowEx(WS_EX_RIGHTSCROLLBAR, TEXT("edit"), pszFileName,
						WS_VISIBLE | WS_POPUPWINDOW | WS_CHILD | WS_SIZEBOX |
						WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE,
						150, 150, 565, 320, edit->wnd, NULL, edit->app_data, NULL);
					SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
					if (SetWindowText(hEdit, pszFileText))
						bSuccess = TRUE;
				}
				GlobalFree(pszFileText);
			}
		}
		CloseHandle(hFile);
	}
	return bSuccess;
}

void gui_open_dialog(ui_t *filedialog, void *data) {
	int ok;
	OPENFILENAME ofn;
	static char result_buf[2048];
	result_buf[0] = '\0';

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.hwndOwner = filedialog->wnd ? filedialog->wnd : NULL;
	ofn.hInstance = filedialog->app_data ? filedialog->app_data : NULL;
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = TEXT("All files(*.*)\0*.*\0");//(opt);
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = result_buf;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.nMaxFile = sizeof(result_buf) - 1;
	ofn.lpstrInitialDir = "./";
	ofn.lpstrTitle = "Select File";
	ofn.lpstrDefExt = "*.*";
	ok = GetOpenFileName(&ofn);
	if (ok && !data) {
		ui_open_cb(filedialog, ofn.lpstrFile);
	} else if (ok && data) {
		((ui_file_cb)data)(filedialog, ofn.lpstrFile);
	}
}

static LRESULT CALLBACK gui_wndproc_form(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	gui_info *ui = (gui_info *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (ui == NULL)
		return DefWindowProc(hwnd, msg, wParam, lParam);

	ui_form_cb verify = ui->user_data;
	const ui_t *app = ui->app;
	Form *form = app->app_data;
	const unsigned long numFields = app->code;
	int i, action, which;
	HWND hStatus, hEdit, hReady;
	HDC hdc;

	switch (msg) {
		case WM_CTLCOLOREDIT:
			if (ui == main_gui_info)
				return 0;

			if (lParam == (LRESULT)GetFocus()) {
				hdc = (HDC)wParam;
				SetTextColor(hdc, RGB(0, 0, 0));
				SetBkColor(hdc, RGB(255, 255, 255));
				return (LRESULT)GetStockObject(WHITE_BRUSH);
			} else {
				hdc = (HDC)wParam;
				SetTextColor(hdc, RGB(255, 255, 255)); 	// RGB_WHITE
				SetBkColor(hdc, RGB(178, 34, 34)); 		// RGB_FIREBRICK
				return (LRESULT)GetStockObject(WHITE_BRUSH);
			}
		case WM_DRAWITEM:
			LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
			PTSTR ptStr = (PTSTR)lpDIS->itemData;
			SetTextColor(lpDIS->hDC, RGB(0xFF, 00, 00));
			ExtTextOut(lpDIS->hDC, 0, 0, 0, &lpDIS->rcItem, ptStr, _tcslen(ptStr), NULL);
			return (LRESULT)0;
		case WM_COMMAND:
			which = LOWORD(wParam);
			action = HIWORD(wParam);
			if (action == BN_CLICKED && which == ID_GUI_CANCEL) {
				DestroyWindow(hwnd);
				return (LRESULT)0;
			} else if (action == BN_CLICKED && which == ID_GUI_CONFIRM) {
				hStatus = GetDlgItem(hwnd, ID_GUI_STATUS);
				for (i = 0; i < numFields; i++) {
					which = form[i].ID;
					hEdit = GetDlgItem(hwnd, which);
					hReady = GetDlgItem(hwnd, (ID_GUI_ERROR + which));
					int len = GetWindowTextLength(hEdit);\
					SendMessage(hReady, BM_SETCHECK, BST_CHECKED, 0);
					if (len > form[i].max) {
						SendMessage(hReady, BM_SETCHECK, BST_UNCHECKED, 0);
						SendMessage(hStatus, SB_SETTEXT, SBT_OWNERDRAW, (LPARAM)"Error: length overflow");
						return (LRESULT)0;
					}

					SendMessage(hStatus, SB_SETTEXT, SBT_OWNERDRAW, (LPARAM)"");
					GetDlgItemTextA(hEdit, which, form[i].value, form[i].max);
					if (verify) {
						CHAR status[260];
						if (verify(app, which, form[i].value, status)) {
							SetDlgItemTextA(hEdit, (ID_GUI_STATUS + which), status);
						} else {
							SendMessage(hReady, BM_SETCHECK, BST_UNCHECKED, 0);
							hEdit = GetDlgItem(hwnd, ID_GUI_STATUS);
							SendMessage(hEdit, SB_SETTEXT, SBT_OWNERDRAW, (LPARAM)status);
							return (LRESULT)0;
						}
					}
				}
				DestroyWindow(hwnd);
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int gui_form(gui_info *ui, ui_t *app, const char *title, Form *fill, int numFields, ui_form_cb verify) {
	int i, x = 0, spacing = 30;
	HWND hEdit, pWnd = app->wnd;

	ui->hinst = app->app_data;
	ui->title = title;
	ui->width = 320;
	ui->height = 300;
	ui->buf = NULL;
	ui->app = app;
	ui->txtCr = RGB(255, 0, 0);
	ui->bkCr = RGB(0, 0, 0);
	ui->user_data = verify;
	ui->app->app_data = fill;
	ui->app->code = numFields;
	ui->app->gui = ui;

	memset(&ui->wc, 0, sizeof(ui->wc));
	ui->wc.cbSize = sizeof(WNDCLASSEX);
	ui->wc.style = CS_VREDRAW | CS_HREDRAW | CS_NOCLOSE;
	ui->wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	ui->wc.lpfnWndProc = gui_wndproc_form;
	ui->wc.hInstance = ui->hinst;
	ui->wc.hCursor = LoadCursor(0, IDC_ARROW);
	ui->wc.lpszClassName = "Edit control";
	RegisterClassEx(&ui->wc);

	if ((ui->wnd = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_DLGMODALFRAME,
		ui->wc.lpszClassName, ui->title,
		WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CHILD | WS_SYSMENU,
		200, 200,
		ui->width, ui->height, pWnd, NULL, ui->hinst, NULL)) == NULL)
		return 0;

	SetWindowLongPtr(ui->wnd, GWLP_USERDATA, (LONG_PTR)ui);
	for (i = 0; i < numFields; i++) {
		x += spacing;
		if (fill[i].caption != NULL) {
			hEdit = CreateWindow("Static", fill[i].caption, WS_CHILD | WS_VISIBLE | SS_LEFT, 6, x - 10,
				fill[i].width, 11, ui->wnd, (HMENU)ID_GUI_STATIC, NULL, NULL);
			SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
		}

		CreateWindow("Edit", fill[i].value, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 5, (x + 5),
			fill[i].width, 21, ui->wnd, (HMENU)(fill[i].ID), NULL, NULL);
		CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_TEXT | BS_FLAT | BS_VCENTER, fill[i].width - 12, (x + 9),
			12, 12, ui->wnd, (HMENU)(ID_GUI_ERROR + fill[i].ID), NULL, NULL);
	}

	hEdit = CreateWindow("Button", "Confirm", WS_VISIBLE | WS_CHILD | BS_PUSHLIKE | WS_TABSTOP, 135, (ui->height - 83), 80, 25,
		ui->wnd, (HMENU)(ID_GUI_CONFIRM), NULL, NULL);
	SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));

	hEdit = CreateWindow("Button", "Cancel", WS_VISIBLE | WS_CHILD | BS_PUSHLIKE | WS_TABSTOP, 220, (ui->height - 83), 80, 25,
		ui->wnd, (HMENU)(ID_GUI_CANCEL), NULL, NULL);
	SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));

	hEdit = CreateWindow(STATUSCLASSNAME, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, ui->height, (ui->width), 5, ui->wnd, (HMENU)ID_GUI_STATUS, NULL, NULL);
	SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));

	ShowWindow(ui->wnd, SW_NORMAL);
	UpdateWindow(ui->wnd);
	return 1;
}

int gui_window(gui_info *ui, const char *title, const int width, const int height, uint32_t *buffer) {
	if (main_gui_info == NULL) {
		main_gui_info = ui;
	}

	ui->title = title;
	ui->width = (int)width;
	ui->height = (int)height;
	ui->buf = buffer;
	ui->hinst = GetModuleHandle(NULL);

	memset(&ui->wc, 0, sizeof(ui->wc));
	ui->wc.cbSize = sizeof(WNDCLASSEX);
	ui->wc.style = CS_VREDRAW | CS_HREDRAW;
	ui->wc.lpfnWndProc = (buffer == NULL ? gui_wndproc : gui_wndproc_arcade);
	ui->wc.hInstance = ui->hinst;
	ui->wc.hIcon = LoadIcon(ui->hinst, MAKEINTRESOURCE(ID_WINDOW_ICON));
	ui->wc.hCursor = LoadCursor(0, IDC_ARROW);
	ui->wc.lpszClassName = ui->title;
	RegisterClassEx(&ui->wc);

	if ((ui->wnd = CreateWindowEx((buffer == NULL ? WS_EX_WINDOWEDGE : (WS_EX_CLIENTEDGE | WS_EX_TOPMOST)),
		ui->title, ui->title, (buffer == NULL ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX)),
		CW_USEDEFAULT, CW_USEDEFAULT, ui->width, ui->height, NULL, NULL, ui->hinst, NULL)) == NULL)
		return 0;

	SetWindowLongPtr(ui->wnd, GWLP_USERDATA, (LONG_PTR)ui);
	ShowWindow(ui->wnd, SW_NORMAL);
	UpdateWindow(ui->wnd);
	return 1;
}

int gui_menu(gui_info *ui, int num_menu, menuitem_t *items, int number_items, int id, char *name) {
	int r = 0;
	menu_t *menu = &(ui->bar_info->menus[num_menu]);
	menu->num_items = number_items;
	menu->selected = none_selected;
	menu->menu_id = id;
	menu->items = items;
	menu->hMenu = CreateMenu();
	if (menu->hMenu) {
		for (r = 0; r < number_items; r++) {
			// add menu items
			AppendMenu(menu->hMenu, MF_STRING, items[r].menu_id, items[r].item_name);
		}
		AppendMenu(ui->bar_info->hMenubar, MF_POPUP, (UINT_PTR)menu->hMenu, name);

		// attach menu bar to the window
		SetMenu(ui->wnd, ui->bar_info->hMenubar);
	}

	return r;
}

int gui_menubar(gui_info *ui, int num_menu) {
	if ((ui->bar_info = (menu_bar_t *)calloc(1, sizeof(menu_bar_t)))) {
		ui->bar_info->hMenubar = CreateMenu();
		ui->bar_info->num_menus = num_menu;
		ui->bar_info->state = 0;
		ui->bar_info->menus = (menu_t *)calloc(1, GetNumMenus(ui->bar_info) * sizeof(menu_t));
		return 1;
	}

	return 0;
}

static int LoadStringEx(HMODULE hModule, UINT wID, PWSTR pBuffer, int cchBufferMax, WORD wLangId) {
	HRSRC hRsrc;
	int cch = 0;
	uintptr_t GroupId = (wID >> 4) + 1;

	if (pBuffer == NULL) {
		return 0;
	}

	hRsrc = FindResourceEx(hModule, RT_STRING, (LPCSTR)GroupId, wLangId);
	if (hRsrc) {
		HGLOBAL hStringSeg = LoadResource(hModule, hRsrc);
		PWSTR psz = (PWSTR)LockResource(hStringSeg);
		if (psz) {
			wID &= 0x0F;
			while (TRUE) {
				cch = *psz++;
				if (wID-- == 0) {
					break;
				}
				psz += cch;
			}

			if (cchBufferMax == 0) {
				*(PWSTR *)pBuffer = psz;
			} else {
				cchBufferMax--;
				if (cch > cchBufferMax) {
					cch = cchBufferMax;
				}

				RtlCopyMemory(pBuffer, psz, cch * sizeof(WCHAR));
			}
			UnlockResource(hStringSeg);
		}
	}

	if (cchBufferMax != 0) {
		pBuffer[cch] = 0;
	}
	return cch;
}

#define MAX_MSGTEXT     32
typedef int (WINAPI *PROC_SOFTMODALMESSAGEBOX)(PMSGBOXDATA lpmb);
typedef int (WINAPI *PROC_LOADSTRINGBASEEXW)(HINSTANCE hInstance, UINT uID, PWSTR lpBuffer, int nBufferMax, int LangId);

static int CustomBox(HWND hWnd, PCWSTR Text, PCWSTR Caption, UINT Type, ButtonW *pButtons, UINT cButtons, UINT dwTimeout, HINSTANCE hInstance, PCWSTR pszIcon, LANGID wLangId) {
	HMODULE hUser32 = LoadLibrary("user32");
	PROC_SOFTMODALMESSAGEBOX pSoftModalMessageBox = (PROC_SOFTMODALMESSAGEBOX)GetProcAddress(hUser32, "SoftModalMessageBox");
	PROC_LOADSTRINGBASEEXW LoadStringBaseExW = (PROC_LOADSTRINGBASEEXW)GetProcAddress(LoadLibrary("kernel32"), "LoadStringBaseExW");
	MSGBOXDATA mbd = {0};
	MSGBOXDATA *pmbd = &mbd;
	int ButtonIds[MAX_MSGBUTTONS] = {0};
	WCHAR *ButtonTexts[MAX_MSGBUTTONS] = {0};
	WCHAR TextBuffer[MAX_MSGBUTTONS][MAX_MSGTEXT] = {0};
	BOOL fCancel = FALSE;
	UINT i;
	ButtonW MsgBoxButtonNull = {0};

	if (pSoftModalMessageBox == 0) {
		return 0;
	}

	if (pButtons == NULL) {
		pButtons = &MsgBoxButtonNull;
		cButtons = 1;
	}

	if (cButtons == 0) {
		cButtons = 1;
	}

	// Max support 11 buttons
	if (cButtons > MAX_MSGBUTTONS) {
		cButtons = MAX_MSGBUTTONS;
	}

	for (i = 0; i < cButtons; i++) {
		ButtonIds[i] = pButtons[i].ID;
		ButtonTexts[i] = (WCHAR *)pButtons[i].label;
		if (ButtonIds[i] == IDCANCEL) {
			fCancel = TRUE;
		}

		// If user doesn't specify button text, try to load one from user32.dll resource
		if (ButtonTexts[i] == 0) {
			// Also can use the LoadStringBaseExW (available in Windows Vista or later) to load
			int n = (LoadStringBaseExW != NULL)
				? LoadStringBaseExW(hUser32, ButtonIds[i] + 800 - 1, TextBuffer[i], MAX_MSGTEXT, wLangId)
				: LoadStringEx(hUser32, ButtonIds[i] + 800 - 1, TextBuffer[i], MAX_MSGTEXT, wLangId);
			if (n == 0) {
				n = LoadStringW(hUser32, ButtonIds[i] + 800 - 1, TextBuffer[i], MAX_MSGTEXT);
			}

			ButtonTexts[i] = TextBuffer[i];
		}
	}

	mbd.cbSize = sizeof(MSGBOXPARAMSW);
	mbd.hwndOwner = hWnd;
	mbd.hInstance = hInstance;
	mbd.lpszText = Text;
	mbd.lpszCaption = Caption;
	mbd.dwStyle = Type;
	if (pszIcon != NULL) {
		mbd.lpszIcon = pszIcon;
		mbd.dwStyle &= (~MB_ICONMASK);
		mbd.dwStyle |= MB_USERICON;
	}

	if (LoadStringBaseExW == NULL) {
		pmbd = (MSGBOXDATA *)((UCHAR *)&mbd - sizeof(DWORD));
	}

	pmbd->wLanguageId = wLangId;
	pmbd->dwTimeout = (dwTimeout == 0) ? INFINITE : dwTimeout;
	pmbd->pidButton = ButtonIds;
	pmbd->ppszButtonText = ButtonTexts;
	pmbd->cButtons = cButtons;
	pmbd->DefButton = (mbd.dwStyle & MB_DEFMASK) >> 8;
	if (cButtons == 1 && pButtons[0].ID == IDOK) {
		pmbd->CancelId = IDOK;
	} else if (fCancel) {
		pmbd->CancelId = IDCANCEL;
		mbd.dwStyle |= MB_OKCANCEL;  // If MB_OK SoftModalMessageBox will return 1 always
	} else {
		mbd.dwStyle |= MB_OKCANCEL;  // If MB_OK SoftModalMessageBox will return 1 always
	}

	return pSoftModalMessageBox(&mbd);
}

/**
 * Parameters:
 *
 * - `hWnd` - Handle to the owner window of the message box to be created. If this parameter is NULL, the message box has no owner window.
 * - `Text` - Pointer to a null-terminated string that contains the message to be displayed.
 * - `Caption` - Pointer to a null-terminated string that contains the dialog box title. If this parameter is NULL, the default title Error is used.
 * - `Type` - Specifies the contents and behavior of the dialog box.
 * - `cButtons` - Specifies the count of pButtons
 * - `dwTimeout` - Specifies the time-out value to close the msgdlg automatically, in milliseconds. 0 means INFINITE
 * - `hInstance` - Handle to the module that contains the icon resource identified by the lpszIcon member, and the string resource identified by the lpszText or lpszCaption member.
 * - `lpszIcon` - Identifies an icon resource. This parameter can be either a null-terminated string or an integer resource identifier passed to the MAKEINTRESOURCE macro.
 * - `wLangId` - Specifies the language of default button text if not specify one in MSGBUTTON->ButtonText */
int message_box_ex(HWND hWnd, PCSTR Caption, PCSTR Text, UINT Type,
	Button *pButtons, UINT cButtons, UINT dwTimeout, HINSTANCE hInstance, PCSTR pszIcon, LANGID wLangId) {
	int ret;
	WCHAR *wText;
	WCHAR *wCaption;
	WCHAR TextBuffer[MAX_MSGBUTTONS][MAX_MSGTEXT] = {0};
	ButtonW wpButtons[MAX_MSGBUTTONS] = {0};
	UINT i;

	if (Text != NULL) {
		size_t TextLen = strlen(Text);
		wText = LocalAlloc(LPTR, (TextLen + 1) * sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, Text, -1, wText, (int)TextLen);
	} else {
		wText = L"";
	}

	if (Caption != NULL) {
		size_t CaptionLen = strlen(Caption);
		wCaption = LocalAlloc(LPTR, (CaptionLen + 1) * sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, Caption, -1, wCaption, (int)CaptionLen);
	} else {
		wCaption = L"";
	}

	for (i = 0; i < min(cButtons, MAX_MSGBUTTONS); i++) {
		wpButtons[i].ID = i + 1;
		wpButtons[i].result = pButtons[i].result;
		if (pButtons[i].label != NULL) {
			wpButtons[i].label = TextBuffer[i];
			MultiByteToWideChar(CP_ACP, 0, pButtons[i].label, -1, TextBuffer[i], MAX_MSGTEXT - 1);
		}
	}

	ret = CustomBox(hWnd, wText, wCaption, Type, wpButtons, cButtons, dwTimeout, hInstance, (PCWSTR)pszIcon, wLangId);
	if (Text != NULL)
		LocalFree(wText);

	if (Caption != NULL)
		LocalFree(wCaption);

	return ret;
}

int gui_message_box(ui_t *app, const char *title, const char *text, const Button *buttons, int numButtons) {
	return message_box_ex((app ? app->wnd : NULL), title, text, (numButtons == 1
		? MB_ICONINFORMATION
		: (numButtons > 2 ? MB_ICONEXCLAMATION : MB_ICONQUESTION)),
		(Button *)buttons, numButtons, 0, (app ? (HINSTANCE)app->app_data : NULL), (numButtons > 3 ? IDI_SHIELD : NULL), 0);
}

void gui_close(gui_info *ui) {
	if (ui) {
		if (ui->bar_info) {
			int i, count, x;
			menuitem_t *menuitem;
			for (i = 0; i < ui->bar_info->num_menus; i++) {
				count = ui->bar_info->menus[i].num_items;
				menuitem = ui->bar_info->menus[i].items;
				for (x = 0; x < count; x++)
					DeleteObject(menuitem[i].hfont);
				DestroyMenu(ui->bar_info->menus[i].hMenu);
			}
			free(ui->bar_info->menus);
			DestroyMenu(ui->bar_info->hMenubar);
			DeleteObject(ui->bar_info->font_info);
			free(ui->bar_info);
			ui->bar_info = NULL;
		}
		ui = NULL;
	}
}

int gui_loop(gui_info *ui) {
	while (PeekMessage(&ui->msg, NULL, 0, 0, PM_REMOVE)) {
		if (ui->msg.message == WM_QUIT || main_gui_shutdown)
			return -1;

		TranslateMessage(&ui->msg);
		DispatchMessage(&ui->msg);
	}

	InvalidateRect(ui->wnd, NULL, TRUE);
	return 0;
}

void gui_active(ui_t *app) {
	gui_handler(app->gui);
}

void gui_destroy(ui_t *app) {
	if (app && main_gui_shutdown)
		ExitProcess((int)app->gui->msg.wParam);
}

void gui_querymenu(gui_info *ui) {
	menu_t menu;
	menuitem_t *menuitem;
	int i, id, x, count;
	long lfHeight = ui->bar_info->lfHeight;
	for (i = 0; i < ui->bar_info->num_menus; i++) {
		menu = ui->bar_info->menus[i];
		count = menu.num_items;
		menuitem = menu.items;
		for (x = 0; x < count; x++) {
			id = menuitem[x].menu_id;
			menuitem[x].hfont = CreateFont(ui->bar_info->lfHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
				0, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, (FF_MODERN | DEFAULT_PITCH), ui->bar_info->font_names);
			ModifyMenu(menu.hMenu, id, MF_BYCOMMAND | MF_OWNERDRAW, id, (LPTSTR)&menuitem[x]);
		}
	}

	DrawMenuBar(ui->wnd);
}

void gui_font(gui_info *ui, const char *font) {
	HFONT hf;
	HDC hdc;
	long lfHeight;

	hdc = GetDC(ui->wnd);
	lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 96);
	ReleaseDC(ui->wnd, hdc);
	if ((hf = CreateFont(lfHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		0, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_MODERN), font))) {
		if (ui->bar_info->font_info)
			DeleteObject(ui->bar_info->font_info);

		ui->bar_info->lfHeight = lfHeight;
		ui->bar_info->font_names = font;
		ui->bar_info->font_info = hf;
	}
}

int gui_queryfont(gui_info *ui) {
	(void)ui;
	return 1;
}

int gui_handler(gui_info *ui) {
	while (GetMessage(&ui->msg, NULL, 0, 0)) {
		TranslateMessage(&ui->msg);
		DispatchMessage(&ui->msg);
	}

	return (int)ui->msg.wParam;
}
#else

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

void gui_open_dialog(ui_t *filedialog, void *data) {
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

void gui_active(ui_t *window) {
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

void gui_font(gui_info *ui, const char *font) {
	memcpy(ui->bar_info->font_names[0], font, strlen(font));
	ui->bar_info->font_names[0][strlen(font)] = '\0';
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

int gui_menubar(gui_info *ui, int num_menu) {
	int index = 0;
	if ((ui->bar_info = (menu_bar_t *)calloc(1, sizeof(menu_bar_t)))) {
		ui->bar_info->num_menus = num_menu;
		ui->bar_info->menus = (menu_t *)calloc(1, GetNumMenus(ui->bar_info) * sizeof(menu_t));
		ui->bar_info->num_fonts = 1;
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
	ui_t menu[1];
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
						memset(menu, 0, sizeof(ui_t));
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

int gui_message_box(ui_t *app, const char *title, const char *text, const Button *buttons, int numButtons) {
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

void gui_destroy(ui_t *app) {
	if (app) {
		XtDestroyWidget(app->app_data);
	}
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
