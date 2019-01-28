#include <everest/SObject.h>

BEGIN_NAMESPACE_EVEREST

MetaClass::MetaClass(ClassType type, const char *pszClassName)
    : m_eType(type)
    , m_pszClassName(pszClassName)
    , m_pSuperClass(nullptr)  { }


SObject::SObject_MetaClass::SObject_MetaClass()
    : MetaClass(MetaClass::Class, STR(EVEREST_NAMESPACE)".SObject")
{ }

MetaClass * SObject::S_SObject_Meta =  new SObject_MetaClass() ;


END_NAMESPACE_EVEREST