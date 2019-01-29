#include <everest/SObject.h>
#include <string.h>
#include <iostream>

BEGIN_NAMESPACE_EVEREST

// SObject MetaClass实现
EVEREST_BEGIN_IMPL_CLASS_META_WITH_NS(EVEREST_NAMESPACE, SObject, nullptr)
EVEREST_END_IMPL_CLASS_META(SObject)


MetaClass::MetaClass(ClassType type, const char *pszClassName, const MetaClass *pSuperClass)
    : m_eType(type)
    , m_pszClassName(pszClassName)
    , m_pSuperClass(pSuperClass)  
    , m_pInterfaceNode(nullptr) {}

bool MetaClass::operator == (const MetaClass &other) const {
    if ( this == &other ) return true;
    if ( 0 == strcmp(this->GetClassName(), other.GetClassName())) 
        return true;
    else 
        return false;
}

const MetaClass * MetaClass::FindSuper(const MetaClass * pMeta) const {
    // 超类非空且查找类型是Class
    if ( m_pSuperClass != nullptr && pMeta->GetClassType() == Class) {
        if ( *m_pSuperClass == *pMeta ) return m_pSuperClass;   // 匹配直接超类

        // 没有匹配，则递归查找超类的超类
        auto pcls = m_pSuperClass->FindSuper(pMeta);
        if ( pcls ) return pcls;    // 如果匹配到就返回
    }

    // 超类没有匹配到，则检查接口类型
    if ( pMeta->GetClassType() != Interface ) return nullptr;   // 不是接口

    Node<const MetaClass *> * clsnode = this->m_pInterfaceNode;
    if ( clsnode == nullptr ) return nullptr;   // 没有接口类型可以查找
    
    do {
        if ( *clsnode->pData == *pMeta ) return clsnode->pData;   // 找到直接实现的接口
        
        // 递归找超接口
        const MetaClass * pcls = clsnode->pData->FindSuper(pMeta);
        if ( pcls ) return pcls;    // 匹配到超接口
    } while ( clsnode = clsnode->pNext);

    return nullptr;   // 都没有找到
}

bool SObject::InstanceOf(const MetaClass * cls) {
    // 本类检查
    const MetaClass *thiscls = this->GetMetaClass();
    if ( *cls == *thiscls ) return true;
    
    // 超类检查
    const MetaClass * supercls = thiscls->FindSuper(cls);
    return supercls != nullptr;
}

std::string SObject::ToString() const {
    const MetaClass *pcls = this->GetMetaClass();
    char buf[32];
    snprintf(buf, 32, "%p", this);
    std::string str;
    str.append(pcls->GetClassName()).append(1, '@').append(buf);
    return std::move(str);
}

END_NAMESPACE_EVEREST
