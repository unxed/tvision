/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TTEXT.H                                                               */
/*                                                                         */
/*   Defines classes and functions related to string manipulation.         */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#ifndef TVISION_TTEXT_H
#define TVISION_TTEXT_H

#ifndef __BORLANDC__
#include <internal/codepage.h>
#include <algorithm>
#include <string_view>
#include <cstring>
#include <cwchar>
#endif

class TStringView {

    // This class exists only to compensate for the lack of std::string_view
    // in Borland C++. Unless you are programming for that compiler, you should
    // always use std::string_view.

    const char _FAR *str;
    size_t len;

public:

    constexpr TStringView();
    TStringView(const char _FAR *str);
    constexpr TStringView(const char _FAR *str, size_t len);
#ifndef __BORLANDC__
    constexpr TStringView(std::string_view text);
    constexpr operator std::string_view() const;
#endif

    constexpr const char* data() const;
    constexpr size_t size() const;
    constexpr Boolean empty() const;
    constexpr const char& operator[](size_t pos) const;
    constexpr const char& front() const;
    constexpr const char& back() const;

    constexpr TStringView substr(size_t pos) const;
    constexpr TStringView substr(size_t pos, size_t n) const;

};

inline constexpr TStringView::TStringView() :
    str(0),
    len(0)
{
}

#pragma warn -inl

inline TStringView::TStringView(const char _FAR *str) :
    str(str),
    len(0)
{
    while (*str++)
        ++len;
}

#pragma warn .inl

inline constexpr TStringView::TStringView(const char _FAR *str, size_t len) :
    str(str),
    len(len)
{
}

#ifndef __BORLANDC__
inline constexpr TStringView::TStringView(std::string_view text) :
    str(text.data()),
    len(text.size())
{
}

inline constexpr TStringView::operator std::string_view() const
{
    return {str, len};
}
#endif

inline constexpr const char* TStringView::data() const
{
    return str;
}

inline constexpr size_t TStringView::size() const
{
    return len;
}

inline constexpr Boolean TStringView::empty() const
{
    return Boolean(size() == 0);
}

inline constexpr const char& TStringView::operator[](size_t pos) const
{
    return str[pos];
}

inline constexpr const char& TStringView::front() const
{
    return str[0];
}

inline constexpr const char& TStringView::back() const
{
    return str[len - 1];
}

inline constexpr TStringView TStringView::substr(size_t pos) const
{
    return TStringView(str + pos, len - pos);
}

inline constexpr TStringView TStringView::substr(size_t pos, size_t n) const
{
    size_t tail = len - pos;
    if (n > tail)
        n = tail;
    return TStringView(str + pos, n);
}

class TText {

    // This class is actually a namespace.

public:

    static size_t next(TStringView text);
    static size_t prev(TStringView text, size_t index);
    static size_t wseek(TStringView text, int count, Boolean incRemainder=True);

    static void eat(TScreenCell *cell, size_t n, size_t &width, TStringView text, size_t &bytes);
    static size_t fill(TSpan<TScreenCell> cells, size_t width, TStringView text, TCellAttribs attr);
    static void next(TStringView text, size_t &bytes, size_t &width);
    static void wseek(TStringView text, size_t &index, size_t &remainder, int count);

};

#ifdef __BORLANDC__

inline size_t TText::next(TStringView text)
{
    return text.size() ? 1 : 0;
}

inline size_t TText::prev(TStringView, size_t index)
{
    return index ? 1 : 0;
}

inline size_t TText::wseek(TStringView text, int count, Boolean)
{
    return count > 0 ? min(count, text.size()) : 0;
}

inline void TText::eat( TScreenCell *cell, size_t n, size_t &width,
                        TStringView text, size_t &bytes )
{
    if (n) {
        ::setChar(*cell, text[0]);
        ++width;
        ++bytes;
    }
}

#pragma warn -inl

inline size_t TText::fill( TSpan<TScreenCell> cells, size_t width,
                           TStringView text, TCellAttribs attr )
{
    if (cells.size()) {
        size_t count = min(min(cells.size(), width), text.size());
        for (size_t i = 0; i < count; ++i)
            ::setCell(cells[i], text[i], attr);
        return count;
    }
    return 0;
}

#pragma warn .inl

inline void TText::next(TStringView text, size_t &bytes, size_t &width)
{
    if (text.size()) {
        ++bytes;
        ++width;
    }
}

inline void TText::wseek(TStringView text, size_t &index, size_t &remainder, int count)
{
    if (count > 0)
        index += count;
    remainder = 0;
}

#else

inline size_t TText::next(TStringView text)
// Measures the length in bytes of the first multibyte character in 'text'.
// If the sequence is not valid UTF-8, length is 1.
{
    if (text.size()) {
        std::mbstate_t state {};
        int64_t len = std::mbrtowc(nullptr, text.data(), text.size(), &state);
        return len <= 1 ? 1 : len;
    }
    return 0;
}

inline size_t TText::prev(TStringView text, size_t index)
// Measures the length in bytes of the character in 'text' right before position 'index'.
// If 'index' > 0 and that position is not preceded by a valid UTF-8 sequence, length is 1.
{
    if (index) {
        // Try reading backwards character by character, until a valid
        // character is found. This tolerates invalid characters.
        size_t lead = std::min<size_t>(index, 4);
        for (size_t i = 1; i <= lead; ++i) {
            std::mbstate_t state {};
            int64_t size = std::mbrtowc(nullptr, &text[index - i], i, &state);
            if (size > 0)
                return (size_t) size == i ? i : 1;
        }
        return 1;
    }
    return 0;
}

inline size_t TText::wseek(TStringView text, int count, Boolean incRemainder)
// Seeks a string by an amount of display columns ('count'). If that amount
// partially overlaps a multi-column character, the whole character is included,
// unless 'incRemainder' is False.
// Returns the number of bytes seeked.
{
    size_t index = 0, remainder = 0;
    wseek(text, index, remainder, count);
    if (!incRemainder && remainder)
        index -= TText::prev(text, index);
    return index;
}

inline void TText::eat( TScreenCell *cell, size_t n, size_t &width,
                        TStringView text, size_t &bytes )
// Reads a single character from a multibyte-encoded string. The display width of
// a character may be 1 or more cells, and all such cells (from cell[0] to, at most,
// cell[n-1]) are updated accordingly.
//
// * cell: TScreenCell to write to. If you want it to have attributes, you should set them first.
// * n: maximum number of cells that can be written to.
// * width (output parameter): gets increased by the display width of the text in cell.
// * text: input text.
// * bytes (output parameter): gets increased by the number of bytes read from 'text'.
{
    if (n) {
        auto &dst = cell->Char;
        auto &attr = cell->Attr;
        cell->extraWidth = 0;
        wchar_t wc;
        std::mbstate_t state {};
        int64_t len = std::mbrtowc(&wc, text.data(), text.size(), &state);
        if (len <= 1) {
            bytes += 1;
            width += 1;
            if (len < 0)
                dst = CpTranslator::toUtf8Int(text[0]);
            else if (len == 0) // '\0'
                dst = ' ';
            else {
                dst = 0;
                memcpy(&dst.bytes, text.data(), 1);
            }
        } else {
            int cWidth = wcwidth(wc);
            bytes += len;
            if (cWidth <= 0) {
                width += 1;
                dst = *(uint32_t *) "�";
            } else {
                width += cWidth;
                dst = 0;
                memcpy(&dst.bytes, text.data(), len);
                // Set extraWidth attribute and fill trailing cells.
                cell->extraWidth = std::min<size_t>(cWidth - 1, 7);
                while (--cWidth && --n) {
                    ++cell;
                    auto trailCell = *cell;
                    trailCell.Char = TScreenCell::wideCharTrail;
                    trailCell.Attr = attr;
                    *cell = trailCell;
                }
            }
        }
    }
}

inline size_t TText::fill( TSpan<TScreenCell> cells, size_t width,
                           TStringView text, TCellAttribs attr )
// Fills at most 'width' 'cells' with characters from 'text'.
// Returns the number of cells filled. Note that one cell is always one display column.
{
    if (cells.size()) {
        if (cells.size() < width)
            width = cells.size();
        size_t w = 0, b = 0;
        while (w < width && b < text.size()) {
            ::setAttr(cells[w], attr);
            TText::eat(&cells[w], width - w, w, text.substr(b), b);
        }
        // TText::eat always increases 'w' by the width of the processed
        // text, but it never fills more cells than there are available.
        return std::min(w, cells.size());
    }
    return 0;
}

inline void TText::next(TStringView text, size_t &bytes, size_t &width)
// Measures the length and width of the first character in 'text'.
//
// * text: input text.
// * bytes (output parameter): gets increased by the length of the first character in 'text'.
// * width (output parameter): gets increased by the display width of the first character in 'text'.
{
    if (text.size()) {
        wchar_t wc;
        std::mbstate_t state {};
        int64_t len = std::mbrtowc(&wc, text.data(), text.size(), &state);
        if (len <= 1) {
            bytes += 1;
            width += 1;
        } else {
            bytes += len;
            width += std::clamp(wcwidth(wc), 1, 8);
        }
    }
}

inline void TText::wseek(TStringView text, size_t &index, size_t &remainder, int count)
// Seeks a string by an amount of display columns. If that amount overlaps a multi-column
// character, 'index' is left pointing to the next character and 'remainder' is set to
// the number of extra seeked columns.
//
// * index (input and output parameter): start position.
// * remainder (output parameter): number of columns in the middle of a wide character.
// * count: number of columns to seek.
{
    if (count > 0) {
        while (index < text.size()) {
            size_t width = 0;
            TText::next({&text[index], text.size() - index}, index, width);
            count -= width;
            if (count <= 0) {
                // Immediately return when the requested width is exceeded.
                remainder = -count;
                return;
            }
        }
    }
    // No remainder when the end of string was reached.
    remainder = 0;
}

#endif // __BORLANDC__

#endif // TVISION_TTEXT_H
