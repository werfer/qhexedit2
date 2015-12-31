#ifndef QHEXEDITDATA_H
#define QHEXEDITDATA_H

/** \cond docNever */

#include <QtCore>

#include <memory>

/*! QHexEditData represents the content of QHexEdit.
QHexEditData comprehend the data itself and informations to store if it was
changed. The QHexEdit component uses these informations to perform nice
rendering of the data

QHexEditData also provides some functionality to insert, replace and remove
single chars and QByteArrays. Additionally some functions support rendering
and converting to readable strings.
*/
class QHexEditData
{
public:
    explicit QHexEditData();
    virtual ~QHexEditData();

private:
    QHexEditData(const QHexEditData & other) = delete;

public:
    int addressOffset() const;
    void setAddressOffset(int offset);

    int addressWidth() const;
    void setAddressWidth(size_t width);

    // TODO improve
    bool dataChanged(int i);
    QByteArray dataChanged(int i, int len);
    void setDataChanged(int i, bool state);
    void setDataChanged(int i, const QByteArray & state);

    size_t realAddressNumbers() const;

    QChar asciiChar(size_t index) const;
    QString toRedableString(size_t start = 0, size_t end = -1) const;

    // abstract members:
    virtual u_int8_t at(size_t addr) const = 0;
    virtual QByteArray range(size_t addr, size_t len) const = 0;

    virtual int indexOf(const QByteArray & ba, size_t from) const = 0;
    virtual int lastIndexOf(const QByteArray & ba, size_t from) const = 0;

    virtual size_t size() const = 0;
    virtual bool fixedSize() const = 0;

    virtual void insert(size_t addr, u_int8_t byte) = 0;
    virtual void insert(size_t addr, const QByteArray & ba) = 0;

    virtual void remove(size_t addr, size_t len) = 0;

    virtual void replace(size_t addr, u_int8_t byte) = 0;
    virtual void replace(size_t addr, const QByteArray & ba) = 0;
    virtual void replace(size_t addr, size_t len, const QByteArray & ba) = 0;

    virtual QByteArray toByteArray() const = 0;

    static std::unique_ptr<QHexEditData> fromMemory(u_int8_t * ptr, size_t size);
    static std::unique_ptr<QHexEditData> fromByteArray(QByteArray ba);
signals:

public slots:

protected:
    QByteArray _changedData;

private:
    int _addressOffset;                 // will be added to the real addres inside bytearray
    size_t _addressNumbers;             // wanted width of address area
    mutable size_t _realAddressNumbers; // real width of address area (can be greater then wanted width)
};

/** \endcond docNever */
#endif // QHEXEDITDATA_H
