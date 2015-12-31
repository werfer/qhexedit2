
#include <QApplication>

#include "qhexedit_p.h"
#include "commands.h"

const int HEXCHARS_IN_LINE = 47;
const int GAP_ADR_HEX = 10;
const int GAP_HEX_ASCII = 16;
const int BYTES_PER_LINE = 16;

QHexEditPrivate::QHexEditPrivate(QScrollArea *parent) : QWidget(parent)
{
    // initial data (empty byte array)
    static QByteArray buffer;
//    static QByteArray buffer("Hello World");
    _data = QHexEditData::fromByteArray(buffer);
    // Test fixed size
//    _data = QHexEditData::fromMemory((u_int8_t *)buffer.data(), buffer.size());

    // one of the first things to do because the following calls will depend on adjust()
    // setFont(QFont("Inconsolata", 12));
    // http://stackoverflow.com/questions/1468022/how-to-specify-monospace-fonts-for-cross-platform-qt-applications
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    _undoStack = new QUndoStack(this);

    _scrollArea = parent;
    setAddressWidth(4);
    setAddressOffset(0);
    setAddressArea(true);
    setAsciiArea(true);
    setHighlighting(true);
    setOverwriteMode(true);
    setReadOnly(false);
    setAddressAreaColor(QColor(0xd4, 0xd4, 0xd4, 0xff));
    setHighlightingColor(QColor(0xff, 0xff, 0x99, 0xff));
    setSelectionColor(QColor(0x6d, 0x9e, 0xff, 0xff));

    adjustCursor(0, CURSORAREA_HEX);

    _size = 0;
    resetSelection(0);

    setFocusPolicy(Qt::StrongFocus);

    connect(&_cursorTimer, SIGNAL(timeout()), this, SLOT(updateCursor()));
    connect(this, SIGNAL(dataChanged()), this, SLOT(adjust()));
    _cursorTimer.setInterval(500);
    _cursorTimer.start();
}

void QHexEditPrivate::setAddressOffset(int offset)
{
    _data->setAddressOffset(offset);
    adjust();
}

int QHexEditPrivate::addressOffset()
{
    return _data->addressOffset();
}

void QHexEditPrivate::setData(std::unique_ptr<QHexEditData> data)
{
    _data = std::move(data);
    adjust();
    adjustCursor(0, CURSORAREA_HEX);
    resetSelection();
    ensureVisible(); // scroll to start
}

QHexEditData & QHexEditPrivate::data()
{
    return *_data;
}

void QHexEditPrivate::setAddressAreaColor(const QColor &color)
{
    _addressAreaColor = color;
    update();
}

QColor QHexEditPrivate::addressAreaColor()
{
    return _addressAreaColor;
}

void QHexEditPrivate::setHighlightingColor(const QColor &color)
{
    _highlightingColor = color;
    update();
}

QColor QHexEditPrivate::highlightingColor()
{
    return _highlightingColor;
}

void QHexEditPrivate::setSelectionColor(const QColor &color)
{
    _selectionColor = color;
    update();
}

QColor QHexEditPrivate::selectionColor()
{
    return _selectionColor;
}

void QHexEditPrivate::setReadOnly(bool readOnly)
{
    _readOnly = readOnly;
}

bool QHexEditPrivate::isReadOnly()
{
    return _readOnly;
}

int QHexEditPrivate::indexOf(const QByteArray & ba, size_t from)
{
    from = std::min(from, _data->size() - 1);
    const int idx = _data->indexOf(ba, from);
    if (idx > -1) {
        const int curPos = idx * 2;
        const int newPos = curPos + ba.length() * 2;
        adjustCursor(newPos, CURSORAREA_HEX);
        resetSelection(curPos);
        setSelection(newPos);
        ensureVisible();
    }
    return idx;
}

void QHexEditPrivate::insert(size_t index, const QByteArray & ba)
{
    if (_data->fixedSize() &&
        index >= _data->size())
    {
        return;
    }

    if (ba.length() <= 0) {
        return;
    }

    QUndoCommand * arrayCommand;
    if (_overwriteMode) {
        arrayCommand = new ArrayCommand(*_data, ArrayCommand::replace, index, ba, ba.length());
    } else {
        arrayCommand = new ArrayCommand(*_data, ArrayCommand::insert, index, ba, ba.length());
    }

    _undoStack->push(arrayCommand);
    emit dataChanged();
}

void QHexEditPrivate::insert(size_t index, char ch)
{
    if (_data->fixedSize() &&
        index >= _data->size())
    {
        return;
    }

    QUndoCommand *charCommand = new CharCommand(*_data, CharCommand::insert, index, ch);
    _undoStack->push(charCommand);
    emit dataChanged();
}

int QHexEditPrivate::lastIndexOf(const QByteArray & ba, size_t from)
{
    const size_t length = static_cast<size_t>(ba.length());
    if (length > from) {
        from = 0;
    } else {
        from -= length;
    }

    const int idx = _data->lastIndexOf(ba, from);
    if (idx > -1)
    {
        const int curPos = idx * 2;
        const int newPos = curPos + ba.length() * 2;
        adjustCursor(curPos, CURSORAREA_HEX);
        resetSelection(curPos);
        setSelection(newPos);
        ensureVisible();
    }
    return idx;
}

void QHexEditPrivate::remove(size_t index, int len)
{
    if (index >= _data->size()) {
        return;
    }

    if (len < 0)
    {
        return;
    }

    if (len == 1)
    {
        if (_overwriteMode)
        {
            QUndoCommand *charCommand = new CharCommand(*_data, CharCommand::replace, index, char(0));
            _undoStack->push(charCommand);
            emit dataChanged();
        }
        else
        {
            QUndoCommand *charCommand = new CharCommand(*_data, CharCommand::remove, index, char(0));
            _undoStack->push(charCommand);
            emit dataChanged();
        }
    }
    else
    {
        QByteArray ba = QByteArray(len, char(0));
        if (_overwriteMode)
        {
            QUndoCommand *arrayCommand = new ArrayCommand(*_data, ArrayCommand::replace, index, ba, ba.length());
            _undoStack->push(arrayCommand);
            emit dataChanged();
        }
        else
        {
            QUndoCommand *arrayCommand= new ArrayCommand(*_data, ArrayCommand::remove, index, ba, len);
            _undoStack->push(arrayCommand);
            emit dataChanged();
        }
    }
}

void QHexEditPrivate::replace(size_t index, char ch)
{
    if (index >= _data->size()) {
        return;
    }

    QUndoCommand *charCommand = new CharCommand(*_data, CharCommand::replace, index, ch);
    _undoStack->push(charCommand);
    resetSelection();
    emit dataChanged();
}

void QHexEditPrivate::replace(size_t index, const QByteArray & ba)
{
    if (index >= _data->size()) {
        return;
    }

    QUndoCommand *arrayCommand= new ArrayCommand(*_data, ArrayCommand::replace, index, ba, ba.length());
    _undoStack->push(arrayCommand);
    resetSelection();
    emit dataChanged();
}

void QHexEditPrivate::replace(size_t from, int len, const QByteArray & after)
{
    if (from >= _data->size()) {
        return;
    }

    QUndoCommand *arrayCommand= new ArrayCommand(*_data, ArrayCommand::replace, from, after, len);
    _undoStack->push(arrayCommand);
    resetSelection();
    emit dataChanged();
}

void QHexEditPrivate::setAddressArea(bool addressArea)
{
    _addressArea = addressArea;
    adjust();

    adjustCursor(_cursorPosition, _cursorArea);
}

void QHexEditPrivate::setAddressWidth(int addressWidth)
{
    _data->setAddressWidth(addressWidth);
    adjust();

    adjustCursor(_cursorPosition, _cursorArea);
}

void QHexEditPrivate::setAsciiArea(bool asciiArea)
{
    _asciiArea = asciiArea;
    adjust();
}

void QHexEditPrivate::setFont(const QFont &font)
{
    // we have to maintain our own font because Qt doesn't always respect our choice
    _monospacedFont = font;
    adjust();
}

const QFont & QHexEditPrivate::font() const
{
    return _monospacedFont;
}

void QHexEditPrivate::setHighlighting(bool mode)
{
    _highlighting = mode;
    update();
}

void QHexEditPrivate::setOverwriteMode(bool overwriteMode)
{
    if (overwriteMode != _overwriteMode)
    {
        _overwriteMode = overwriteMode;
        overwriteModeChanged(_overwriteMode);
    }
}

bool QHexEditPrivate::overwriteMode()
{
    return _overwriteMode;
}

void QHexEditPrivate::redo()
{
    _undoStack->redo();
    emit dataChanged();
    adjustCursor(_cursorPosition, _cursorArea);
    update();
}

void QHexEditPrivate::undo()
{
    _undoStack->undo();
    emit dataChanged();
    adjustCursor(_cursorPosition, _cursorArea);
    update();
}

QString QHexEditPrivate::toRedableString()
{
    return _data->toRedableString();
}


QString QHexEditPrivate::selectionToReadableString()
{
    return _data->toRedableString(getSelectionBegin(), getSelectionEnd());
}

void QHexEditPrivate::keyPressEvent(QKeyEvent *event)
{
    if (cursorEvent(event)) {
        ;  // pass
    } else if (selectEvent(event)) {
        ;
    } else if (!_readOnly && editEvent(event)) {
        ;
    } else if (copyEvent(event)) {
        ;
    } else if (modeEvent(event)) {
        ;
    } else {
        return;
    }

    ensureVisible();
    update();
}

bool QHexEditPrivate::cursorEvent(QKeyEvent * event)
{
    int steps = (_cursorArea == CURSORAREA_HEX) ? 2 : 1;

    /*****************************************************************************/
    /* Cursor movements */
    /*****************************************************************************/
    if (event->matches(QKeySequence::MoveToNextChar)) {
        adjustCursor(_cursorPosition + 1, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToPreviousChar)) {
        adjustCursor(_cursorPosition - 1, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToEndOfLine)) {
        int cPos = _cursorPosition | (steps * BYTES_PER_LINE - 1);
        adjustCursor(cPos, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToStartOfLine)) {
        int cPos = _cursorPosition - (_cursorPosition % (steps * BYTES_PER_LINE));
        adjustCursor(cPos, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToPreviousLine)) {
        int cPos = _cursorPosition - (steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToNextLine)) {
        int cPos = _cursorPosition + (steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToNextPage)) {
        int cPos = _cursorPosition + (((_scrollArea->viewport()->height() / _charHeight) - 1) * steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToPreviousPage)) {
        int cPos = _cursorPosition - (((_scrollArea->viewport()->height() / _charHeight) - 1) * steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToEndOfDocument)) {
        adjustCursor(steps * _data->size(), _cursorArea);
        resetSelection(_cursorPosition);
    } else if (event->matches(QKeySequence::MoveToStartOfDocument)) {
        adjustCursor(0, CURSORAREA_HEX);
        resetSelection(_cursorPosition);
    } else {
        return false;
    }

    return true;
}

bool QHexEditPrivate::selectEvent(QKeyEvent * event)
{
    int steps = (_cursorArea == CURSORAREA_HEX) ? 2 : 1;

    /*****************************************************************************/
    /* Select commands */
    /*****************************************************************************/
    if (event->matches(QKeySequence::SelectAll)) {
        resetSelection(0);
        setSelection(2 * _data->size() + 1);
        return true;
    } else if (event->matches(QKeySequence::SelectNextChar)) {
        int cPos = _cursorPosition + 1;
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectPreviousChar)) {
        int cPos = _cursorPosition - 1;
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectEndOfLine)) {
        int cPos = _cursorPosition - (_cursorPosition % (steps * BYTES_PER_LINE)) + (steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectStartOfLine)) {
        int cPos = _cursorPosition - (_cursorPosition % (steps * BYTES_PER_LINE));
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectPreviousLine)) {
        int cPos = _cursorPosition - (steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectNextLine)) {
        int cPos = _cursorPosition + (steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectNextPage)) {
        int cPos = _cursorPosition + (((_scrollArea->viewport()->height() / _charHeight) - 1) * steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectPreviousPage)) {
        int cPos = _cursorPosition - (((_scrollArea->viewport()->height() / _charHeight) - 1) * steps * BYTES_PER_LINE);
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectEndOfDocument)) {
        int cPos = _data->size() * 2;
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else if (event->matches(QKeySequence::SelectStartOfDocument)) {
        int cPos = 0;
        adjustCursor(cPos, _cursorArea);
        setSelection(cPos);
    } else {
        return false;
    }

    // finished event handling
    return true;
}

bool QHexEditPrivate::editEvent(QKeyEvent * event)
{
    const int size = static_cast<int>(_data->size());
    int steps, charX, posX, posBa;
    if (_cursorArea == CURSORAREA_HEX) {
        steps = 2;
        charX = (_cursorX - _xPosHex) / _charWidth;
        posX  = (charX / 3) * 2 + (charX % 3);
        posBa = (_cursorY / _charHeight) * BYTES_PER_LINE + posX / 2;
    } else {
        steps = 1;
        charX = (_cursorX - _xPosAscii) / _charWidth;
        posX  = charX;
        posBa = (_cursorY / _charHeight) * BYTES_PER_LINE + posX;
    }

    /*****************************************************************************/
    /* Edit Commands */
    /*****************************************************************************/
    int key = int(event->text()[0].toLatin1());

    if ((_cursorArea == CURSORAREA_HEX) &&
        ((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f')))
    {   /* Hex input */
        if (_data->fixedSize() && posBa >= size) {
            return true;
        }

        if (getSelectionBegin() != getSelectionEnd())
        {
            posBa = getSelectionBegin();
            remove(posBa, getSelectionEnd() - posBa);
            adjustCursor(steps * posBa, _cursorArea);
            resetSelection(steps * posBa);
        }

        // If insert mode, then insert a byte
        if (_overwriteMode == false && (charX % 3) == 0) {
            insert(posBa, char(0));
        }

        // Change content
        if (_data->size() > 0)
        {
            QByteArray hexValue = _data->range(posBa, 1).toHex();
            if ((charX % 3) == 0)
                hexValue[0] = key;
            else
                hexValue[1] = key;

            replace(posBa, QByteArray().fromHex(hexValue)[0]);

            adjustCursor(_cursorPosition + 1, _cursorArea);
            resetSelection(_cursorPosition);
        }

        return true;
    }
    else if (_cursorArea == CURSORAREA_ASCII &&
             QChar(key).isPrint())
    {   /* ASCII input */
        if (_data->fixedSize() && posBa >= size) {
            return true;
        }

        if (_overwriteMode == false) {
            insert(posBa, static_cast<char>(key));
        } else if (_data->size() > 0) {
            replace(posBa, static_cast<char>(key));
        }

        adjustCursor(_cursorPosition + 1, _cursorArea);
        resetSelection(_cursorPosition);

        return true;
    }

    /* Cut & Paste */
    if (event->matches(QKeySequence::Cut))
    {
        QString result = QString();
        for (int idx = getSelectionBegin(); idx < getSelectionEnd(); idx++)
        {
            result += _data->range(idx, 1).toHex() + " ";
            if ((idx % 16) == 15)
                result.append("\n");
        }
        remove(getSelectionBegin(), getSelectionEnd() - getSelectionBegin());
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(result);
        adjustCursor(getSelectionBegin() * steps, _cursorArea);
        resetSelection(getSelectionBegin() * steps);

        return true;
    }

    if (event->matches(QKeySequence::Paste))
    {
        QClipboard *clipboard = QApplication::clipboard();
        QByteArray ba = QByteArray().fromHex(clipboard->text().toLatin1());
        insert(_cursorPosition / steps, ba);
        adjustCursor(_cursorPosition + steps * ba.length(), _cursorArea);
        resetSelection(getSelectionBegin());

        return true;
    }

    /* Delete char */
    if (event->matches(QKeySequence::Delete))
    {
        if (getSelectionBegin() != getSelectionEnd())
        {
            posBa = getSelectionBegin();
            remove(posBa, getSelectionEnd() - posBa);
            adjustCursor(steps * posBa, _cursorArea);
            resetSelection(steps * posBa);
        }
        else
        {
            if (_overwriteMode)
                replace(posBa, char(0));
            else
                remove(posBa, 1);
        }

        return true;
    }

    /* Backspace */
    if ((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
    {
        if (getSelectionBegin() != getSelectionEnd())
        {
            posBa = getSelectionBegin();
            remove(posBa, getSelectionEnd() - posBa);
            adjustCursor(steps * posBa, _cursorArea);
            resetSelection(steps * posBa);
        }
        else if (posBa > 0)
        {
            if (_overwriteMode)
                replace(posBa - 1, char(0));
            else
                remove(posBa - 1, 1);

            adjustCursor(_cursorPosition - steps, _cursorArea);
        }

        return true;
    }

    /* undo */
    if (event->matches(QKeySequence::Undo))
    {
        undo();
        return true;
    }

    /* redo */
    if (event->matches(QKeySequence::Redo))
    {
        redo();
        return true;
    }

    return false;
}

bool QHexEditPrivate::copyEvent(QKeyEvent * event)
{
    if (event->matches(QKeySequence::Copy))
    {
        QString result = QString();
        for (int idx = getSelectionBegin(); idx < getSelectionEnd(); idx++)
        {
            //result += _xData.data().mid(idx, 1).toHex() + " ";
            result += _data->range(idx, 1).toHex() + " ";
            if ((idx % 16) == 15)
                result.append('\n');
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(result);

        return true;
    }

    return false;
}

bool QHexEditPrivate::modeEvent(QKeyEvent * event)
{
    // Switch between insert/overwrite mode
    if ((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
    {
        adjustCursor(_cursorPosition, _cursorArea);
        setOverwriteMode(!overwriteMode());

        return true;
    }

    return false;
}

void QHexEditPrivate::mouseMoveEvent(QMouseEvent * event)
{
    int actPos;
    CursorArea_t area;

    _blink = false;
    update();

    int result = calcCursorInfo(event->pos(), actPos, area);
    if (result != 0) {
        return;
    }

    adjustCursor(actPos, area);
    setSelection(actPos);
}

void QHexEditPrivate::mousePressEvent(QMouseEvent * event)
{
    int cPos;
    CursorArea_t cArea;

    _blink = false;
    update();

    int result = calcCursorInfo(event->pos(), cPos, cArea);
    if (result != 0) {
        return;
    }

    // It's important to adjust the cursor before the selection gets modified.
    // Otherwise the new selection may refer to the last highlighted area.
    adjustCursor(cPos, cArea);
    resetSelection(cPos);
}

void QHexEditPrivate::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setFont(_monospacedFont);

    const int top = event->rect().top();
    const int bottom = event->rect().bottom();

    // draw some patterns if needed
    painter.fillRect(event->rect(), this->palette().color(QPalette::Base));
    if (_addressArea)
        painter.fillRect(QRect(_xPosAdr, event->rect().top(), _xPosHex - GAP_ADR_HEX + _charWidth / 2, height()), _addressAreaColor);
    if (_asciiArea)
    {
        int linePos = _xPosAscii - (GAP_HEX_ASCII / 2);
        painter.setPen(Qt::gray);
        painter.drawLine(linePos, event->rect().top(), linePos, height());
    }

    painter.setPen(this->palette().color(QPalette::WindowText));

    // calc position
    size_t firstLineIdx = 0;
    if (top > (_charHeight * _charHeight)) { // underflow condition
        firstLineIdx = ((top /  _charHeight) - _charHeight) * BYTES_PER_LINE;
    }

    size_t lastLineIdx = ((bottom / _charHeight) + _charHeight) * BYTES_PER_LINE;
    if (lastLineIdx > _data->size()) {
        lastLineIdx = _data->size();
    }

    int yPosStart = ((firstLineIdx) / BYTES_PER_LINE) * _charHeight + _charHeight;

    // paint address area
    if (_addressArea)
    {
        for (size_t lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=_charHeight)
        {
            QString address = QString("%1")
                              .arg(lineIdx + _data->addressOffset(), _data->realAddressNumbers(), 16, QChar('0'));
            painter.drawText(_xPosAdr, yPos, address);
        }
    }

    // paint hex area
    QByteArray hexBa(_data->range(firstLineIdx, lastLineIdx - firstLineIdx + 1).toHex());
    QBrush highLighted = QBrush(_highlightingColor);
    QPen colHighlighted = QPen(this->palette().color(QPalette::WindowText));
    QBrush selected = QBrush(_selectionColor);
    QPen colSelected = QPen(Qt::white);
    QPen colStandard = QPen(this->palette().color(QPalette::WindowText));

    painter.setBackgroundMode(Qt::TransparentMode);

    for (size_t lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=_charHeight)
    {
        QByteArray hex;
        int xPos = _xPosHex;
        for (int colIdx = 0; ((lineIdx + colIdx) < _data->size() and (colIdx < BYTES_PER_LINE)); colIdx++)
        {
            int posBa = lineIdx + colIdx;
            if ((getSelectionBegin() <= posBa) && (getSelectionEnd() > posBa))
            {
                painter.setBackground(selected);
                painter.setBackgroundMode(Qt::OpaqueMode);
                painter.setPen(colSelected);
            }
            else if (_highlighting)
            {
                // highlight diff bytes
                painter.setBackground(highLighted);
                if (_data->dataChanged(posBa))
                {
                    painter.setPen(colHighlighted);
                    painter.setBackgroundMode(Qt::OpaqueMode);
                }
                else
                {
                    painter.setPen(colStandard);
                    painter.setBackgroundMode(Qt::TransparentMode);
                }
            }

            // render hex value
            if (colIdx == 0)
            {
                hex = hexBa.mid((lineIdx - firstLineIdx) * 2, 2);
                painter.drawText(xPos, yPos, hex);
                xPos += 2 * _charWidth;
            } else {
                hex = hexBa.mid((lineIdx + colIdx - firstLineIdx) * 2, 2).prepend(" ");
                painter.drawText(xPos, yPos, hex);
                xPos += 3 * _charWidth;
            }

#if 0
            if (colIdx != 0) {
                xPos += _charWidth;
            }

            hex = hexBa.mid((lineIdx + colIdx - firstLineIdx) * 2, 2);
            painter.drawText(xPos, yPos, hex);
            xPos += 2 * _charWidth;
#endif
        }
    }
    painter.setBackgroundMode(Qt::TransparentMode);
    painter.setPen(this->palette().color(QPalette::WindowText));

    // paint ascii area
    if (_asciiArea)
    {
        for (size_t lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=_charHeight)
        {
            int xPosAscii = _xPosAscii;
            for (int colIdx = 0; ((lineIdx + colIdx) < _data->size() and (colIdx < BYTES_PER_LINE)); colIdx++)
            {
                int posBa = lineIdx + colIdx;

                if ((getSelectionBegin() <= posBa) && (getSelectionEnd() > posBa))
                {
                    painter.setBackground(selected);
                    painter.setBackgroundMode(Qt::OpaqueMode);
                    painter.setPen(colSelected);
                } else {
                    painter.setBackgroundMode(Qt::TransparentMode);
                    painter.setPen(colStandard);
                }

                painter.drawText(xPosAscii, yPos, _data->asciiChar(lineIdx + colIdx));
                xPosAscii += _charWidth;
            }
        }
    }

    // paint cursor
    if (_blink && !_readOnly && hasFocus())
    {
        if (_overwriteMode)
            painter.fillRect(_cursorX, _cursorY + _charHeight - 2, _charWidth, 2, this->palette().color(QPalette::WindowText));
        else
            painter.fillRect(_cursorX, _cursorY, 2, _charHeight, this->palette().color(QPalette::WindowText));
    }

    if (_size != _data->size())
    {
        _size = _data->size();
        emit currentSizeChanged(_size);
    }
}

void QHexEditPrivate::adjustCursor(size_t position, CursorArea_t area)
{
    int cursorPosition = static_cast<int>(position);
    const int factor = (area == CURSORAREA_HEX) ? 2 : 1;
    const int size = static_cast<int>(_data->size());
    _cursorArea = area;

    // delete cursor
    _blink = false;
    update();

    // cursor in range?
    if (_overwriteMode) {
        cursorPosition = std::min(cursorPosition, size * factor - 1);
    } else {
        cursorPosition = std::min(cursorPosition, size * factor);
    }
    cursorPosition = std::max(cursorPosition, 0);

    // calc position
    _cursorPosition = cursorPosition;
    _cursorY = (cursorPosition / (factor * BYTES_PER_LINE)) * _charHeight + 4;

    int x = (cursorPosition % (factor * BYTES_PER_LINE));
    if (area == CURSORAREA_HEX) {
        _cursorX = (((x / 2) * 3) + (x % 2)) * _charWidth + _xPosHex;
    } else {
        _cursorX = x * _charWidth + _xPosAscii;
    }

    // immiadately draw cursor
    _blink = true;
    update();

    emit currentAddressChanged(_cursorPosition / factor);
}

int QHexEditPrivate::calcCursorInfo(QPoint pnt, int & pos, CursorArea_t & area)
{
    // find char under cursor
    const int hexAreaEnd = _xPosHex + (HEXCHARS_IN_LINE + 2) * _charWidth;
    if ((pnt.x() >= _xPosHex) and (pnt.x() < hexAreaEnd))
    {
        int x = (pnt.x() - _xPosHex) / _charWidth;
        if ((x % 3) == 0)
            x = (x / 3) * 2;
        else
            x = ((x / 3) * 2) + 1;
        int y = ((pnt.y() - 3) / _charHeight) * 2 * BYTES_PER_LINE;

        pos = x + y;
        area = CURSORAREA_HEX;
        return 0;
    }

    const int asciiAreaEnd = _xPosAscii + (BYTES_PER_LINE * _charWidth);
    if (_asciiArea and (pnt.x() >= _xPosAscii) and (pnt.x() < asciiAreaEnd)) {
        int x = (pnt.x() - _xPosAscii) / _charWidth;
        int y = ((pnt.y() - 3) / _charHeight) * BYTES_PER_LINE;
/*
        printf("ascii cursor: %d\n", x+y);
        fflush(stdout);
*/
        pos = x + y;
        area = CURSORAREA_ASCII;
        return 0;
    }

    return -1;
}

int QHexEditPrivate::cursorPos() const
{
    return _cursorPosition;
}

void QHexEditPrivate::resetSelection()
{
    _selectionBegin = _selectionInit;
    _selectionEnd = _selectionInit;
}

void QHexEditPrivate::resetSelection(int pos)
{
    if (_cursorArea == CURSORAREA_HEX) {
        pos /= 2;
    }

    pos = std::max(pos, 0);
    pos = std::min(pos, static_cast<int>(_data->size()));

    _selectionInit = pos;
    _selectionBegin = pos;
    _selectionEnd = pos;
}

void QHexEditPrivate::setSelection(int pos)
{
    if (_cursorArea == CURSORAREA_HEX) {
        pos /= 2;
    }

    pos = std::max(pos, 0);
    pos = std::min(pos, static_cast<int>(_data->size()));

    if (pos >= _selectionInit) {
        _selectionEnd = pos;
        _selectionBegin = _selectionInit;
    } else {
        _selectionBegin = pos;
        _selectionEnd = _selectionInit;
    }

//    std::cout << "begin:" << _selectionBegin << " end:" << _selectionEnd << std::endl;
}

int QHexEditPrivate::getSelectionBegin()
{
    return _selectionBegin;
}

int QHexEditPrivate::getSelectionEnd()
{
    return _selectionEnd;
}


void QHexEditPrivate::updateCursor()
{
    _blink = !_blink;
    update(_cursorX, _cursorY, _charWidth, _charHeight);
}

void QHexEditPrivate::adjust()
{
    QFontMetrics metrics(_monospacedFont);
    _charWidth = metrics.width(QLatin1Char('9'));
    _charHeight = metrics.height();

    _xPosAdr = 0;
    if (_addressArea)
        _xPosHex = _data->realAddressNumbers() * _charWidth + GAP_ADR_HEX;
    else
        _xPosHex = 0;
    _xPosAscii = _xPosHex + HEXCHARS_IN_LINE * _charWidth + GAP_HEX_ASCII;

    // tell QAbstractScollbar, how big we are
    setMinimumHeight(((_data->size() / 16 + 1) * _charHeight) + 5);
    if(_asciiArea)
        setMinimumWidth(_xPosAscii + (BYTES_PER_LINE * _charWidth));
    else
        setMinimumWidth(_xPosHex + HEXCHARS_IN_LINE * _charWidth);

    update();
}

void QHexEditPrivate::ensureVisible()
{
    // scrolls to cursorx, cusory (which are set by setCursorPos)
    // x-margin is 3 pixels, y-margin is half of charHeight
    _scrollArea->ensureVisible(_cursorX, _cursorY + _charHeight/2, 3, _charHeight/2 + 2);
}
