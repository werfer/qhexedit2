#ifndef QHEXEDIT_P_H
#define QHEXEDIT_P_H

/** \cond docNever */

#include <memory>

#include <QtGui>
#include <QWidget>
#include <QScrollArea>
#include <QUndoStack>

#include "xbytearray.h"
#include "qhexeditdata.h"

typedef enum _CursorArea {
    CURSORAREA_HEX,
    CURSORAREA_ASCII
} CursorArea_t;

class QHexEditPrivate : public QWidget
{
Q_OBJECT

public:
    QHexEditPrivate(QScrollArea *parent);

    void setAddressAreaColor(QColor const &color);
    QColor addressAreaColor();

    void setAddressOffset(int offset);
    int addressOffset();

    void adjustCursor(size_t position, CursorArea_t area);
    int cursorPos() const;
    CursorArea_t cursorArea() const;

    void setData(std::unique_ptr<QHexEditData> data);
    QHexEditData & data();

    void setHighlightingColor(QColor const &color);
    QColor highlightingColor();

    void setOverwriteMode(bool overwriteMode);
    bool overwriteMode();

    void setReadOnly(bool readOnly);
    bool isReadOnly();

    void setSelectionColor(QColor const &color);
    QColor selectionColor();

    int indexOf(const QByteArray & ba, size_t from = 0);
    void insert(size_t index, const QByteArray & ba);
    void insert(size_t index, char ch);
    int lastIndexOf(const QByteArray & ba, size_t from = 0);
    void remove(size_t index, int len = 1);
    void replace(size_t index, char ch);
    void replace(size_t index, const QByteArray & ba);
    void replace(size_t from, int len, const QByteArray & after);

    void setAddressArea(bool addressArea);
    void setAddressWidth(int addressWidth);
    void setAsciiArea(bool asciiArea);
    void setHighlighting(bool mode);

    virtual void setFont(const QFont &font);
    virtual const QFont & font() const;

    void undo();
    void redo();

    QString toRedableString();
    QString selectionToReadableString();

signals:
    void currentAddressChanged(int address);
    void currentSizeChanged(size_t size);
    void dataChanged();
    void overwriteModeChanged(bool state);

protected:
    void keyPressEvent(QKeyEvent * event);
    bool cursorEvent(QKeyEvent * event);
    bool selectEvent(QKeyEvent * event);
    bool editEvent(QKeyEvent * event);
    bool copyEvent(QKeyEvent * event);
    bool modeEvent(QKeyEvent * event);

    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);

    void paintEvent(QPaintEvent *event);

    // calc cursorpos from graphics position. DOES NOT STORE POSITION
    int calcCursorInfo(QPoint pnt, int & pos, CursorArea_t & area);


    void resetSelection(int pos);       // set selectionStart and selectionEnd to pos
    void resetSelection();              // set selectionEnd to selectionStart
    void setSelection(int pos);         // set min (if below init) or max (if greater init)
    int getSelectionBegin();
    int getSelectionEnd();


private slots:
    void updateCursor();
    void adjust();

private:
    void ensureVisible();

    QFont _monospacedFont;

    QColor _addressAreaColor;
    QColor _highlightingColor;
    QColor _selectionColor;
    QScrollArea * _scrollArea;
    QTimer _cursorTimer;
    QUndoStack * _undoStack;

    std::unique_ptr<QHexEditData> _data;

    bool _blink;                            // true: then cursor blinks
    bool _renderingRequired;                // Flag to store that rendering is necessary
    bool _addressArea;                      // left area of QHexEdit
    bool _asciiArea;                        // medium area
    bool _highlighting;                     // highlighting of changed bytes
    bool _overwriteMode;
    bool _readOnly;                         // true: the user can only look and navigate

    int _charWidth, _charHeight;            // char dimensions (dpendend on font)
    int _cursorX, _cursorY;                 // graphics position of the cursor
    int _cursorPosition;                    // character positioin in stream (on byte ends in to steps)
    int _xPosAdr, _xPosHex, _xPosAscii;     // graphics x-position of the areas

    int _selectionBegin;                    // First selected char
    int _selectionEnd;                      // Last selected char
    int _selectionInit;                     // That's, where we pressed the mouse button

    size_t _size;


    CursorArea_t _cursorArea;
};

/** \endcond docNever */

#endif

