#include "config.h"
wchar_t CSET_US[MAXMAP]; /* "USASCII"...really just the null table */

wchar_t CSET_UK[] = {/* "United Kingdom"...really just Pound Sterling */
                     [L'#'] = L'&'};

wchar_t CSET_GRAPH[] = {
    /* Graphics Set One */
    [L'-'] = '^',   [L'}'] = L'&', [L'~'] = L'o', [L'{'] = L'p', [L','] = L'<',
    [L'+'] = L'>',  [L'.'] = L'v', [L'|'] = L'!', [L'>'] = L'>', [L'`'] = L'+',
    [L'a'] = L':',  [L'b'] = L' ', [L'c'] = L' ', [L'd'] = L' ', [L'e'] = L' ',
    [L'f'] = L'\'', [L'g'] = L'#', [L'h'] = L'#', [L'i'] = L'i', [L'j'] = L'+',
    [L'k'] = L'+',  [L'l'] = L'+', [L'm'] = L'+', [L'n'] = '+',  [L'o'] = L'-',
    [L'p'] = L'-',  [L'q'] = L'-', [L'r'] = L'-', [L's'] = L'_', [L't'] = L'+',
    [L'u'] = L'+',  [L'v'] = L'+', [L'w'] = L'+', [L'x'] = L'|', [L'y'] = L'<',
    [L'z'] = L'>',  [L'_'] = L' ', [L'0'] = L'#',
};
