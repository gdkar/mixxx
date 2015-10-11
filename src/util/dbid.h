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
    explicit constexpr DbId(int value):m_value(value){};
    explicit DbId(QVariant variant);
    constexpr bool isValid() const{return isValidValue(m_value);};
    // This function is needed for backward compatibility and
    // should only be used within legacy code. It can be deleted
    // after all integer IDs have been replaced by their type-safe
    // counterparts.
    constexpr int toInt() const{return m_value;}
    constexpr operator int()const{return m_value;}
    // This function should be used for value binding in DB queries
    // with bindValue().
    QVariant toVariant() const;
    QString toString() const;
    friend bool operator==(const DbId& lhs, const DbId& rhs) {
        return lhs.m_value == rhs.m_value;
    }
    friend bool operator!=(const DbId& lhs, const DbId& rhs) {
        return lhs.m_value != rhs.m_value;
    }
    friend bool operator<(const DbId& lhs, const DbId& rhs) {
        return lhs.m_value < rhs.m_value;
    }
    friend bool operator>(const DbId& lhs, const DbId& rhs) {
        return lhs.m_value > rhs.m_value;
    }
    friend bool operator<=(const DbId& lhs, const DbId& rhs) {
        return lhs.m_value <= rhs.m_value;
    }
    friend bool operator>=(const DbId& lhs, const DbId& rhs) {
        return lhs.m_value >= rhs.m_value;
    }
    friend std::ostream& operator<< (std::ostream& os, const DbId& dbId);
    friend QDebug& operator<< (QDebug& qd, const DbId& dbId) ;
    friend uint qHash(const DbId& dbId);
private:
    static const int kInvalidValue = -1;
    static const QVariant::Type kVariantType;
    static constexpr bool isValidValue(int value){return value>=0;};
    static int valueOf(QVariant /*pass-by-value*/ variant);
    int m_value = kInvalidValue;
};
Q_DECLARE_METATYPE(DbId)
Q_DECLARE_TYPEINFO(DbId,Q_PRIMITIVE_TYPE);