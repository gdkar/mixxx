#include "util/dbid.h"


/*static*/ const QVariant::Type DbId::kVariantType = DbId().toVariant().type();

/*static*/ int DbId::valueOf(QVariant /*pass-by-value*/ variant) {
    // The parameter "variant" is passed by value since we need to use
    // the in place QVariant::convert(). Due to Qt's implicit sharing
    // the value is only copied if the type is different. The redundant
    // conversion inside value() is bypassed since the type already
    // matches. We cannot use value(), only because it returns the
    // valid id 0 in case of conversion errors.
    if (variant.convert(kVariantType)) {
        auto value = variant.value<int>();
        if (isValidValue(value)) {return value;}
    }
    return kInvalidValue;
}
std::ostream& operator<< ( std::ostream &os, const DbId& dbId){return ( os << dbId.m_value );}
DbId::DbId(QVariant v):DbId(valueOf(std::move(v))){}
QVariant DbId::toVariant()const{return QVariant(m_value);}
QString  DbId::toString ()const{return QString::number(m_value);}
uint qHash(const DbId &dbId){return qHash(dbId.m_value);}
QDebug&       operator<< ( QDebug& q, const DbId& dbId){  return q<< dbId.m_value;}
