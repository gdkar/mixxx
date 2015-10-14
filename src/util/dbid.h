_Pragma("once")
#include <ostream>

#include <QDebug>
#include <QString>
#include <QVariant>
#include <QtDebug>
#include "util/assert.h"

// Base class for ID values of objects that are stored in the database.
//
// Implicit conversion from/to the native value type has been disabled
// on purpose in favor of explicit conversion. The internal value type
// is also hidden as an implementation detail.
//
// Although used as a base class this class has a non-virtual destructor,
// because destruction is trivial and for maximum efficiency! Derived
// classes may add additional behavior (= member functions), but should
// not add any additional state (= member variables). Inheritance is
// only needed for type-safety.
class DbId {
protected:
    // Alias for the corresponding native type. This typedef
    // should actually not be needed by users of this class,
    // but it keeps the implementation of this class flexible
    // if we ever gonna need to change it from 'int' to 'long'
    // or any other type.
    typedef int value_type;
public:
    DbId() = default;
    explicit DbId(int value) noexcept;
    explicit DbId(QVariant variant) ;
    bool isValid() const;
    // This function is needed for backward compatibility and
    // should only be used within legacy code. It can be deleted
    // after all integer IDs have been replaced by their type-safe
    // counterparts.
    int toInt() const;
    operator int()const;
    // This function should be used for value binding in DB queries
    // with bindValue().
    QVariant toVariant() const;
    QString toString() const;
    friend bool operator==(const DbId& lhs, const DbId& rhs);
    friend bool operator!=(const DbId& lhs, const DbId& rhs);
    friend bool operator<(const DbId& lhs, const DbId& rhs);
    friend bool operator>(const DbId& lhs, const DbId& rhs);
    friend bool operator<=(const DbId& lhs, const DbId& rhs);
    friend bool operator>=(const DbId& lhs, const DbId& rhs);
    friend std::ostream& operator<< (std::ostream& os, const DbId& dbId);
    friend QDebug& operator<< (QDebug& qd, const DbId& dbId) ;
    friend uint qHash(const DbId& dbId);
private:
    static const int kInvalidValue = -1;
    static const QVariant::Type kVariantType;
    static bool isValidValue(int value);
    static int valueOf(QVariant /*pass-by-value*/ variant);
    int m_value = kInvalidValue;
};
Q_DECLARE_METATYPE(DbId)
Q_DECLARE_TYPEINFO(DbId,Q_PRIMITIVE_TYPE);
