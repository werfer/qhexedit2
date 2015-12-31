#include "commands.h"

CharCommand::CharCommand(QHexEditData & data, Cmd cmd, size_t charPos, char newChar, QUndoCommand * parent)
    : QUndoCommand(parent),
      _data(data),
      _charPos(charPos),
      _newChar(newChar),
      _cmd(cmd)
{ }

bool CharCommand::mergeWith(const QUndoCommand *command)
{
    const CharCommand *nextCommand = static_cast<const CharCommand *>(command);
    bool result = false;

    if (_cmd != remove &&
        nextCommand->_cmd == replace &&
        nextCommand->_charPos == _charPos)
    {
        _newChar = nextCommand->_newChar;
        result = true;
    }
    return result;
}

void CharCommand::undo()
{
    switch (_cmd)
    {
        case insert:
            _data.remove(_charPos, 1);
            break;
        case replace:
            _data.replace(_charPos, _oldChar);
            _data.setDataChanged(_charPos, _wasChanged);
            break;
        case remove:
            _data.insert(_charPos, _oldChar);
            _data.setDataChanged(_charPos, _wasChanged);
            break;
    }
}

void CharCommand::redo()
{
    switch (_cmd)
    {
        case insert:
            _data.insert(_charPos, _newChar);
            break;
        case replace:
            _oldChar = static_cast<char>(_data.at(_charPos));
            _wasChanged = _data.dataChanged(_charPos);
            _data.replace(_charPos, _newChar);
            break;
        case remove:
            _oldChar = static_cast<char>(_data.at(_charPos));
            _wasChanged = _data.dataChanged(_charPos);
            _data.remove(_charPos, 1);
            break;
    }
}



ArrayCommand::ArrayCommand(QHexEditData & data, Cmd cmd, size_t baPos, QByteArray newBa, size_t len, QUndoCommand * parent)
    : QUndoCommand(parent),
      _data(data),
      _cmd(cmd),
      _baPos(baPos),
      _len(len),
      _newBa(newBa)
{ }

void ArrayCommand::undo()
{
    switch (_cmd)
    {
        case insert:
            _data.remove(_baPos, _newBa.length());
            break;
        case replace:
            _data.replace(_baPos, _oldBa);
            _data.setDataChanged(_baPos, _wasChanged);
            break;
        case remove:
            _data.insert(_baPos, _oldBa);
            _data.setDataChanged(_baPos, _wasChanged);
            break;
    }
}

void ArrayCommand::redo()
{
    switch (_cmd)
    {
        case insert:
            _data.insert(_baPos, _newBa);
            break;
        case replace:
            _oldBa = _data.range(_baPos, _len);
            _wasChanged = _data.dataChanged(_baPos, _len);
            _data.replace(_baPos, _newBa);
            break;
        case remove:
            _oldBa = _data.range(_baPos, _len);
            _wasChanged = _data.dataChanged(_baPos, _len);
            _data.remove(_baPos, _len);
            break;
    }
}
