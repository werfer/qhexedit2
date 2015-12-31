#include "qhexeditdata.h"

#include <cassert>
#include <cmath>

QHexEditData::QHexEditData()
{
    _addressNumbers = 4;
    _addressOffset = 0;
}

QHexEditData::~QHexEditData()
{ }

int QHexEditData::addressOffset() const
{
    return _addressOffset;
}

void QHexEditData::setAddressOffset(int offset)
{
    _addressOffset = offset;
}

int QHexEditData::addressWidth() const
{
    return _addressNumbers;
}

void QHexEditData::setAddressWidth(size_t width)
{
    _addressNumbers = width;
}

bool QHexEditData::dataChanged(int i)
{
    return bool(_changedData[i]);
}

QByteArray QHexEditData::dataChanged(int i, int len)
{
    return _changedData.mid(i, len);
}

void QHexEditData::setDataChanged(int i, bool state)
{
    _changedData[i] = char(state);
}

void QHexEditData::setDataChanged(int i, const QByteArray & state)
{
    int length = state.length();
    int len;
    if ((i + length) > _changedData.length())
        len = _changedData.length() - i;
    else
        len = length;
    _changedData.replace(i, len, state);
}

size_t QHexEditData::realAddressNumbers() const
{
    // the number of nibbles
    _realAddressNumbers = std::ceil(std::log(size() + _addressOffset + 1) / (std::log(2) * 4.0));
    _realAddressNumbers = std::max(_realAddressNumbers, _addressNumbers);
    return _realAddressNumbers;
}

QChar QHexEditData::asciiChar(size_t index) const
{
    char ch = at(index);
    if ((ch < 0x20) or (ch > 0x7e))
            ch = '.';
    return QChar(ch);
}

QString QHexEditData::toRedableString(size_t start, size_t end) const
{
    size_t addrWidth = realAddressNumbers();
    if (_addressNumbers > addrWidth)
        addrWidth = _addressNumbers;

    QString result;
    for (size_t i=start; i < end; i += 16)
    {
        QString addrStr = QString("%1").arg(_addressOffset + i, addrWidth, 16, QChar('0'));
        QString hexStr;
        QString ascStr;
        for (int j=0; j<16; j++)
        {
            if ((i + j) < size())
            {
                hexStr.append(" ").append(range(i+j, 1).toHex()); // TODO improve
                ascStr.append(asciiChar(i+j));
            }
        }
        result += addrStr + " " + QString("%1").arg(hexStr, -48) + "  " + QString("%1").arg(ascStr, -17) + "\n";
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// QHexEditByteArrayData implementation:
class QHexEditMemoryData : public QHexEditData
{
public:
    explicit QHexEditMemoryData(u_int8_t * ptr, size_t size);
    virtual ~QHexEditMemoryData();

    // abstract members:
    virtual u_int8_t at(size_t addr) const;
    virtual QByteArray range(size_t addr, size_t len) const;

    virtual int indexOf(const QByteArray & ba, size_t from) const;
    virtual int lastIndexOf(const QByteArray & ba, size_t from) const;

    virtual size_t size() const;
    virtual bool fixedSize() const;

    // overlapping bytes will be discarded
    virtual void insert(size_t addr, u_int8_t byte);
    virtual void insert(size_t addr, const QByteArray & ba);

    virtual void remove(size_t addr, size_t len);

    virtual void replace(size_t addr, u_int8_t byte);
    virtual void replace(size_t addr, const QByteArray & ba);
    virtual void replace(size_t addr, size_t len, const QByteArray & ba);

    virtual QByteArray toByteArray() const;

private:
    void moveUp(size_t addr, size_t n);
    void moveDown(size_t addr, size_t n);

    u_int8_t * _ptr;
    size_t _size;
    QByteArray _wrapper;
};

QHexEditMemoryData::QHexEditMemoryData(u_int8_t * ptr, size_t size) :
    _ptr(ptr),
    _size(size)
{
    _wrapper = QByteArray::fromRawData(reinterpret_cast<const char *>(_ptr), _size);
}

QHexEditMemoryData::~QHexEditMemoryData()
{ }

u_int8_t QHexEditMemoryData::at(size_t addr) const
{
    assert(addr < _size);
    return static_cast<u_int8_t>(*(_ptr + addr));
}

QByteArray QHexEditMemoryData::range(size_t addr, size_t len) const
{
    assert(addr < _size);
    return QByteArray(reinterpret_cast<const char *>(_ptr + addr), len);
}

int QHexEditMemoryData::indexOf(const QByteArray & ba, size_t from) const
{
    return _wrapper.indexOf(ba, static_cast<int>(from));
}

int QHexEditMemoryData::lastIndexOf(const QByteArray & ba, size_t from) const
{
    return _wrapper.lastIndexOf(ba, static_cast<int>(from));
}

size_t QHexEditMemoryData::size() const
{
    return _size;
}

bool QHexEditMemoryData::fixedSize() const
{
    return true;
}

void QHexEditMemoryData::insert(size_t addr, u_int8_t byte)
{// TODO update _changedData
    assert(addr < _size);
    moveDown(addr, 1);
    *(_ptr + addr) = byte;
}

void QHexEditMemoryData::insert(size_t addr, const QByteArray & ba)
{// TODO update _changedData
    assert(addr < _size);

    moveDown(addr, ba.length());

    size_t len = std::min(_size - addr, static_cast<size_t>(ba.length()));
    memcpy(_ptr + addr, ba.data(), len);
}

void QHexEditMemoryData::remove(size_t addr, size_t len)
{// TODO update _changedData
    assert(addr < _size);

    moveUp(addr, len);

    // the remaining space is filled with zeros
    memset(_ptr + (_size - len), 0, len);
}

void QHexEditMemoryData::replace(size_t addr, u_int8_t byte)
{// TODO update _changedData
    assert(addr < _size);
    *(_ptr + addr) = byte;
}

void QHexEditMemoryData::replace(size_t addr, const QByteArray & ba)
{// TODO update _changedData
    assert(addr < _size);
    size_t len = std::min(_size - addr, static_cast<size_t>(ba.length()));
    memcpy(_ptr + addr, ba.data(), len);
}

void QHexEditMemoryData::replace(size_t addr, size_t len, const QByteArray & ba)
{// TODO update _changedData
    assert(addr < _size);
    assert(ba.length() >= static_cast<int>(len));
    len = std::min(_size - addr, len);
    memcpy(_ptr + addr, ba.data(), len);
}

void QHexEditMemoryData::moveUp(size_t addr, size_t n)
{
    assert(addr < _size);
// TODO test
    if (n == 0 || n >= _size) {
        return;
    }

    size_t i = addr;
    for (; (i + n) < _size; ++i) {
        *(_ptr + i) = *(_ptr + i + n);
    }
}

void QHexEditMemoryData::moveDown(size_t addr, size_t n)
{
    assert(addr < _size);
// TODO test
    if (n == 0 || n >= _size) {
        return;
    }

    size_t i = _size - n - 1;
    for (; i >= addr; --i) {
        *(_ptr + i + n) = *(_ptr + i);

        if (i == 0) {
            break;
        }
    }
}

QByteArray QHexEditMemoryData::toByteArray() const
{
    return _wrapper;
}

////////////////////////////////////////////////////////////////////////////////
// QHexEditByteArrayData implementation:
class QHexEditByteArrayData : public QHexEditData
{
public:
    explicit QHexEditByteArrayData(QByteArray data);
    virtual ~QHexEditByteArrayData();

    virtual u_int8_t at(size_t addr) const;
    virtual QByteArray range(size_t addr, size_t len) const;

    virtual int indexOf(const QByteArray & ba, size_t from) const;
    virtual int lastIndexOf(const QByteArray & ba, size_t from) const;

    virtual size_t size() const;
    virtual bool fixedSize() const;

    // overlapping bytes will be discarded
    virtual void insert(size_t addr, u_int8_t byte);
    virtual void insert(size_t addr, const QByteArray & ba);

    virtual void remove(size_t addr, size_t len);

    virtual void replace(size_t addr, u_int8_t byte);
    virtual void replace(size_t addr, const QByteArray & ba);
    virtual void replace(size_t addr, size_t len, const QByteArray & ba);

    virtual QByteArray toByteArray() const;

private:
    QByteArray _data;
};

QHexEditByteArrayData::QHexEditByteArrayData(QByteArray data) :
    _data(data)
{ }

QHexEditByteArrayData::~QHexEditByteArrayData()
{ }

u_int8_t QHexEditByteArrayData::at(size_t addr) const
{
    return _data.at(addr);
}

QByteArray QHexEditByteArrayData::range(size_t addr, size_t len) const
{
    return _data.mid(addr, len);
}

int QHexEditByteArrayData::indexOf(const QByteArray & ba, size_t from) const
{
    return _data.indexOf(ba, from);
}

int QHexEditByteArrayData::lastIndexOf(const QByteArray & ba, size_t from) const
{
    return _data.lastIndexOf(ba, from);
}

size_t QHexEditByteArrayData::size() const
{
    return _data.size();
}

bool QHexEditByteArrayData::fixedSize() const
{
    return false;
}

void QHexEditByteArrayData::insert(size_t addr, u_int8_t byte)
{
    _data.insert(addr, byte);
    _changedData.insert(addr, char(1));
}

void QHexEditByteArrayData::insert(size_t addr, const QByteArray & ba)
{
    _data.insert(addr, ba);
    _changedData.insert(addr, QByteArray(ba.length(), char(1)));
}

void QHexEditByteArrayData::remove(size_t addr, size_t len)
{
    _data.remove(addr, len);
    _changedData.remove(addr, len);
}

void QHexEditByteArrayData::replace(size_t addr, u_int8_t byte)
{
    int i = static_cast<int>(addr);
    _data[i] = static_cast<char>(byte);
    _changedData[i] = char(1);
}

void QHexEditByteArrayData::replace(size_t addr, const QByteArray & ba)
{
    int len = ba.length();
    replace(addr, len, ba);
}

void QHexEditByteArrayData::replace(size_t addr, size_t len, const QByteArray & ba)
{
    size_t length = _data.length();
    if ((addr + len) > length) {
        len = _data.length() - addr;
    }

    _data.replace(addr, len, ba.mid(0, len));
    _changedData.replace(addr, len, QByteArray(len, char(1)));
}

QByteArray QHexEditByteArrayData::toByteArray() const
{
    return _data;
}

////////////////////////////////////////////////////////////////////////////////
// QHexEditData construction:
std::unique_ptr<QHexEditData> QHexEditData::fromMemory(u_int8_t * ptr, size_t size)
{
    return std::unique_ptr<QHexEditData>(new QHexEditMemoryData(ptr, size));
}

std::unique_ptr<QHexEditData> QHexEditData::fromByteArray(QByteArray ba)
{
    return std::unique_ptr<QHexEditData>(new QHexEditByteArrayData(ba));
}
