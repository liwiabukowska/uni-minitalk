#pragma once

#define ESC_CSI "\e["

#define ESC_CURS_UP(n) ESC_CSI #n "A"
#define ESC_CURS_DOWN(n) ESC_CSI #n "B"
#define ESC_CURS_FORW(n) ESC_CSI #n "C"
#define ESC_CURS_BACK(n) ESC_CSI #n "D"
#define ESC_LINE_NEXT(n) ESC_CSI #n "E"
#define ESC_LINE_PREV(n) ESC_CSI #n "F"
#define ESC_CURS_COL(n) ESC_CSI #n "G"
#define ESC_CURS_POS(x, y) ESC_CSI #y ";" #x "H"

#define ESC_CLEAR_RAW(n) ESC_CSI #n "J"
#define ESC_CLEAR_CUR_END ESC_CLEAR_RAW(0)
#define ESC_CLEAR_BEG_CUR ESC_CLEAR_RAW(1)
#define ESC_CLEAR_SCREEN ESC_CLEAR_RAW(2)
#define ESC_CLEAR_ALL ESC_CLEAR_RAW(3)

#define ESC_CLEAR_LINE_RAW(n) ESC_CSI #n "K"
#define ESC_CLEAR_LINE_CUREND ESC_CLEAR_LINE_RAW(0)
#define ESC_CLEAR_LINE_BEGCUR ESC_CLEAR_LINE_RAW(1)
#define ESC_CLEAR_LINE ESC_CLEAR_LINE_RAW(2)

#define ESC_SCROLL_UP(n) ESC_CSI #n "S"
#define ESC_SCROLL_DOWN(n) ESC_CSI #n "T"

#define ESC_HVP(x, y) ESC_CSI #y ";" #x "f"

#define ESC_SGR(txt) ESC_CSI txt "m"
#define ESC_SGR_CLEAR ESC_SGR("0")
#define ESC_SGR_BOLD ESC_SGR("1")
#define ESC_SGR_FAINT ESC_SGR("2")
#define ESC_SGR_ITALIC ESC_SGR("3")
#define ESC_SGR_UNDERLINE ESC_SGR("4")
#define ESC_SGR_BLINK_SLOW ESC_SGR("5")
#define ESC_SGR_BLINK_FAST ESC_SGR("6")

#define ESC_SGR_UNDERLINE_STOP ESC_SGR("24")
#define ESC_SGR_BLINK_STOP ESC_SGR("25")
// ...
#define ESC_SGR_CROSSED_OUT(txt) ESC_SGR("9")

#define ESC_SGR_FGR_COLOR_RGB(r, g, b) ESC_SGR("38;2;" #r ";" #g ";" #b)
#define ESC_SGR_FGR_COLOR_N(n) ESC_SGR("38;5;" #n)

#define ESC_SGR_BGR_COLOR_RGB(r, g, b) ESC_SGR("48;2;" #r ";" #g ";" #b)
#define ESC_SGR_BGR_COLOR_N(n) ESC_SGR("48;5;" #n)
// ...