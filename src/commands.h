#ifndef COMMANDS_H
#define COMMANDS_H

/** \cond docNever */

#include <QUndoCommand>

//#include "xbytearray.h"
#include "qhexeditdata.h"

/*! CharCommand is a class to prived undo/redo functionality in QHexEdit.
A QUndoCommand represents a single editing action on a document. CharCommand
is responsable for manipulations on single chars. It can insert. replace and
remove characters. A manipulation stores allways to actions
1. redo (or do) action
2. undo action.

CharCommand also supports command compression via mergeWidht(). This allows
the user to execute a undo command contation e.g. 3 steps in a single command.
If you for example insert a new byt "34" this means for the editor doing 3
steps: insert a "00", replace it with "03" and the replace it with "34". These
3 steps are combined into a single step, insert a "34".
*/
class CharCommand : public QUndoCommand
{
public:
    enum { Id = 1234 };
    enum Cmd {insert, remove, replace};

    CharCommand(QHexEditData & data, Cmd cmd, size_t charPos, char newChar,
                QUndoCommand * parent = 0);

    void undo();
    void redo();
    bool mergeWith(const QUndoCommand *command);
    int id() const { return Id; }

private:
    QHexEditData & _data;
    size_t _charPos;
    bool _wasChanged;
    char _newChar;
    char _oldChar;
    Cmd _cmd;
};

/*! ArrayCommand provides undo/redo functionality for handling binary strings. It
can undo/redo insert, replace and remove binary strins (QByteArrays).
*/
class ArrayCommand : public QUndoCommand
{
public:
    enum Cmd {insert, remove, replace};
    ArrayCommand(QHexEditData & data, Cmd cmd, size_t baPos, QByteArray newBa = QByteArray(), size_t len = 0,
                 QUndoCommand * parent = 0);
    void undo();
    void redo();

private:
    QHexEditData & _data;
    Cmd _cmd;
    size_t _baPos;
    size_t _len;
    QByteArray _wasChanged;
    QByteArray _newBa;
    QByteArray _oldBa;
};

/** \endcond docNever */

#endif // COMMANDS_H
