

#include <gui.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __linux__

void TestCallback(Widget w, XtPointer client, XtPointer call) {
	TextFieldReturnStruct *ret;
	int count;
	char *str, *s, *val;

	ret = (TextFieldReturnStruct *)call;
	printf("ret->string=%s\n", ret->string);
	str = TextFieldGetString(w);
	printf("TEXT: item=%s\n", str);
	s = str;
	while (*s)
		*s++ = '*';
	TextFieldSetString(w, str);
	TextFieldInsert(w, 4, "4");
	TextFieldReplace(w, 8, 10, "XXX");
	TextFieldSetEditable(w, True);
	TextFieldSetSelection(w, 5, 10, 0);
	XtVaGetValues(w, XtNstring, &val, NULL);
	printf("GetValues: %s\n", val);
	XtFree(str);
}

void EchoCallback(Widget w, XtPointer client, XtPointer call) {
	TextFieldReturnStruct *ret = (TextFieldReturnStruct *)call;
	TextFieldSetEditable(w, True);
	printf("ret->string=%s\n", ret->string);
}

void valueCB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldReturnStruct *ret = (TextFieldReturnStruct *)call_data;
	TextFieldSetEditable(w, True);
	printf("changed: string = %s\n", ret->string);
}

void focusCB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldReturnStruct *ret = (TextFieldReturnStruct *)call_data;
	printf("focus in, string=%s\n", ret->string);
}

void losefocusCB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldVerifyStruct *ret = (TextFieldVerifyStruct *)call_data;
	printf("focus out, text=%x\n", ret->text);
}

void gain1CB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldReturnStruct *ret = (TextFieldReturnStruct *)call_data;
	printf("gain primary, string=%s\n", ret->string);
}

void lose1CB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldReturnStruct *ret = (TextFieldReturnStruct *)call_data;
	printf("gain primary, string=%s\n", ret->string);
}

void modifyCB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldVerifyStruct *ret = (TextFieldVerifyStruct *)call_data;
	printf("modify, text=%x\n", ret->text);
	if (ret->text != NULL) {
		printf("length=%d\n", ret->text->length);
		if (ret->text->ptr[0] == ',')
			ret->doit = False;
	}
}

void motionCB(Widget w, XtPointer client, XtPointer call_data) {
	TextFieldVerifyStruct *ret = (TextFieldVerifyStruct *)call_data;
	printf("move, text=%x\n", ret->text);
	if (ret->newInsert > ret->curInsert)
		ret->doit = False;
}

void form_prompt(actionbar_t *formfield, void *data) {
	char *text = "Free alternative to the Motif XmTextField";
	Widget form = XtVaCreateManagedWidget("box", boxWidgetClass, formfield->wnd, 0);
	Widget t1 = XtVaCreateManagedWidget("variablewidth", textfieldWidgetClass, form,
		XtNwidth, 300,
		XtNstring, text,
		XtNinsertPosition, 100,
		XtNtop, XtChainTop,
		XtNleft, XtChainLeft,
		XtNright, XtChainRight,
		NULL);

	XtAddCallback(t1, XtNactivateCallback, EchoCallback, (XtPointer)NULL);
	XtAddCallback(t1, XtNvalueChangedCallback, valueCB, NULL);
	XtAddCallback(t1, XtNfocusCallback, focusCB, NULL);
	XtAddCallback(t1, XtNlosingFocusCallback, losefocusCB, NULL);
	XtAddCallback(t1, XtNgainPrimaryCallback, gain1CB, NULL);
	XtAddCallback(t1, XtNlosePrimaryCallback, lose1CB, NULL);
	XtAddCallback(t1, XtNmodifyVerifyCallback, modifyCB, NULL);
	XtAddCallback(t1, XtNmotionVerifyCallback, motionCB, NULL);
	Widget t2 = XtVaCreateManagedWidget("monospaced", textfieldWidgetClass, form,
		XtNstring, "Fixed Length",
		XtNinsertPosition, 0,
		XtNlength, 16,
		XtNbottom, XtChainBottom,
		XtNleft, XtChainLeft,
		XtNright, XtChainRight,
		XtNfromVert, t1,
		NULL);
	XtAddCallback(t2, XtNactivateCallback, EchoCallback, (XtPointer)NULL);
	t2 = XtVaCreateManagedWidget("monospaced", textfieldWidgetClass, form,
		XtNstring, "No Echo",
		XtNecho, True,
		XtNinsertPosition, 0,
		XtNbottom, XtChainBottom,
		XtNleft, XtChainLeft,
		XtNright, XtChainRight,
		XtNfromVert, t2,
		NULL);// gui_skeleton
	XtAddCallback(t2, XtNactivateCallback, EchoCallback, (XtPointer)NULL);
	t2 = XtVaCreateManagedWidget("default", textfieldWidgetClass, form,
		XtNstring, "No Pending Delete",
		XtNpendingDelete, False,
		XtNinsertPosition, 0,
		XtNbottom, XtChainBottom,
		XtNleft, XtChainLeft,
		XtNright, XtChainRight,
		XtNfromVert, t2,
		NULL);
	TextFieldSetEditable(t1, True);
	XtAddCallback(t2, XtNactivateCallback, EchoCallback, (XtPointer)NULL);

	formfield->app_data = (void *)form;
	formfield->name = "Form Fill";
	gui_active(formfield);
	XtDestroyWidget(form);
}

#endif

#define W 320
#define H 240

static void fenster_rect(gui_info *f, int x, int y, int w, int h,
	uint32_t c) {
	int row, col;
	for (row = 0; row < h; row++) {
		for (col = 0; col < w; col++) {
			gui_pixel(f, x + col, y + row) = c;
		}
	}
}

// clang-format off
static uint16_t font5x3[] = {0x0000,0x2092,0x002d,0x5f7d,0x279e,0x52a5,0x7ad6,0x0012,0x4494,0x1491,0x017a,0x05d0,0x1400,0x01c0,0x0400,0x12a4,0x2b6a,0x749a,0x752a,0x38a3,0x4f4a,0x38cf,0x3bce,0x12a7,0x3aae,0x49ae,0x0410,0x1410,0x4454,0x0e38,0x1511,0x10e3,0x73ee,0x5f7a,0x3beb,0x624e,0x3b6b,0x73cf,0x13cf,0x6b4e,0x5bed,0x7497,0x2b27,0x5add,0x7249,0x5b7d,0x5b6b,0x3b6e,0x12eb,0x4f6b,0x5aeb,0x388e,0x2497,0x6b6d,0x256d,0x5f6d,0x5aad,0x24ad,0x72a7,0x6496,0x4889,0x3493,0x002a,0xf000,0x0011,0x6b98,0x3b79,0x7270,0x7b74,0x6750,0x95d6,0xb9ee,0x5b59,0x6410,0xb482,0x56e8,0x6492,0x5be8,0x5b58,0x3b70,0x976a,0xcd6a,0x1370,0x38f0,0x64ba,0x3b68,0x2568,0x5f68,0x54a8,0xb9ad,0x73b8,0x64d6,0x2492,0x3593,0x03e0};
// clang-format on
static void fenster_text(gui_info *f, int x, int y, char *s, int scale,
	uint32_t c) {
	int dy, dx;
	while (*s) {
		char chr = *s++;
		if (chr > 32) {
			uint16_t bmp = font5x3[chr - 32];
			for (dy = 0; dy < 5; dy++) {
				for (dx = 0; dx < 3; dx++) {
					if (bmp >> (dy * 3 + dx) & 1) {
						fenster_rect(f, x + dx * scale, y + dy * scale, scale, scale, c);
					}
				}
			}
		}
		x = x + 4 * scale;
	}
}

/* ============================================================
 * A small example demostrating keymaps/keycodes:
 * - On all platforms keys usually correspond to upper-case ASCII
 * - Enter code is 10, Tab is 9, Backspace is 8, Escape is 27
 * - Delete is 127, Space is 32
 * - Modifiers are: Ctrl=1, Shift=2, Ctrl+Shift=3
 *
 * This demo prints currently pressed keys with modifiers.
 * ============================================================ */
void key_box(actionbar_t *app, void *data) {
	uint32_t buf[W * H] = {0};
	gui_info f = {0};
	gui_window(&f, "Press any key...", W, H, buf);
	int64_t now = gui_time();
	int i;
	while (gui_loop(&f) == 0) {
		int has_keys = 0;
		char s[32];
		char *p = s;
		for (i = 0; i < 128; i++) {
			if (f.keys[i]) {
				has_keys = 1;
				*p++ = i;
			}
		}
		*p = '\0';
		fenster_rect(&f, 0, 0, W, H, 0);
		/* draw mouse "pointer" */
		if (f.x > 5 && f.y > 5 && f.x < f.width - 5 && f.y < f.height - 5) {
			fenster_rect(&f, f.x - 3, f.y - 3, 6, 6, f.mouse ? 0xffffff : 0xff0000);
		}
		fenster_text(&f, 8, 8, s, 4, 0xffffff);
		if (has_keys) {
			if (f.mod & 1) {
				fenster_text(&f, 8, 40, "Ctrl", 4, 0xffffff);
			}
			if (f.mod & 2) {
				fenster_text(&f, 8, 80, "Shift", 4, 0xffffff);
			}
		}
		if (f.keys[27]) {
			break;
		}
		int64_t time = gui_time();
		if (time - now < 1000 / 60) {
			gui_sleep(time - now);
		}
		now = time;
	}
	gui_close(&f);
}

/* ============================================================
 * A very minimal example of a Fenster app:
 * - Opens a window
 * - Starts a loop
 * - Changes pixel colours based on some "shader" formula
 * - Sleeps if needed to maintain a frame rate of 60 FPS
 * - Closes a window
 * ============================================================ */
void color_box(actionbar_t *app, void *data) {
	int i, j;
	uint32_t buf[W * H];
	gui_info f = {0};
	gui_window(&f, "hello", W, H, buf);
	uint32_t t = 0;
	int64_t now = gui_time();
	while (gui_loop(&f) == 0) {
		t++;
		for (i = 0; i < 320; i++) {
			for (j = 0; j < 240; j++) {
			  /* White noise: */
				//gui_pixel(&f, i, j) = (rand() << 16) ^ (rand() << 8) ^ rand();

			  /* Colorful and moving: */
				gui_pixel(&f, i, j) = i * j * t;

			  /* Munching squares: */
				//gui_pixel(&f, i, j) = i ^ j ^ t;
			}
		}

		int64_t time = gui_time();
		if (time - now < 1000 / 60) {
			gui_sleep(time - now);
		}

		now = time;
	}

	gui_close(&f);
}

void message_box(actionbar_t *app, void *data) {
	Button buttons[3];
	char lang_bt_eng[] = "English";
	buttons[0].label = lang_bt_eng;

	buttons[0].result = 1;

	int res = gui_message_box("Language",
		"Please choose a language.", buttons, 1);

	if (res == 1) {
		buttons[0].label = "No";
		buttons[1].label = "Yes";
		res = gui_message_box("Answer this question",
			"Do you like to program in C language?", buttons, 2);
		if (res == 1) {
			buttons[0].label = "Accept";
			res = gui_message_box("Oops",
				"Unfortunately, you are a bad person.\nThere is nothing I can do for you.", buttons, 1);
		}
	}
}

int main(int argc, char **argv) {
	int error = -1;
	gui_info ui = {0};
	if (gui_window(&ui, "Skeleton", 600, 600, NULL) && gui_create_menu(&ui, 2, 2)) {
		menuitem_t items[] = {
			{no_menu," Open", gui_open_dialog, 'o', NULL},
			{no_menu," Form", form_prompt, 'f', NULL},
		};

		menuitem_t items_two[] = {
			{no_menu," Alert Box", message_box, 'a', NULL},
			{no_menu," Arcade Box", color_box, 'x', NULL},
			{no_menu," Key Box", key_box, 'w', NULL},
		};

		gui_font(&ui, 0, "lucidasans-12");
		gui_font(&ui, 1, "lucidasans-10");
		if (!gui_queryfont(&ui)) {
			return 1;
		}

		if (!gui_menu(&ui, 0, items, 2, 1, " File")
			|| !gui_menu(&ui, 1, items_two, 3, 2, " Mode")) {
			return 1;
		}

		gui_querymenu(&ui);
		error = gui_handler(&ui);
		gui_close(&ui);
	}

	return error;
}