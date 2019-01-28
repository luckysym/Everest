#pragma once

#define EVEREST_NAMESPACE everest 
#define BEGIN_NAMESPACE_EVEREST namespace everest {
#define END_NAMESPACE_EVEREST }
#define TO_STR(a)  #a
#define STR(a)  TO_STR(a)

BEGIN_NAMESPACE_EVEREST

class MetaClass {
public:
    enum ClassType {
        Class,
        Interface
    };
    template <class T>
    struct Node {
        Node<T> * pNext;
        T         pData;
    };
private:
    ClassType   m_eType;
    const char *m_pszClassName;
    MetaClass  *m_pSuperClass;
    Node<MetaClass *> * m_pHeadIntf;

public:
    MetaClass(ClassType type, const char *pszClassName);

    const char * GetClassName() const { return m_pszClassName; }
    MetaClass * GetSuperClass() { return m_pSuperClass; }
    const MetaClass * GetSuperClass() const { return m_pSuperClass; }

    void SetSuperClass(MetaClass *pMeta) { m_pSuperClass = pMeta; }
    
    void AddInterface(MetaClass *pMeta) { 
        Node<MetaClass*> * node = new Node<MetaClass*>();
        node->pData = pMeta;
        node->pNext = m_pHeadIntf;
        m_pHeadIntf = node;
    }
}; // end class MetaClass


class SObject {
private:
    class SObject_MetaClass : public MetaClass {
    public:
        SObject_MetaClass();
    }; // end class SObject_MetaClass
    static MetaClass * S_SObject_Meta;
public:
    static  MetaClass * Meta() { return S_SObject_Meta; }
    virtual MetaClass * GetMetaClass() { return S_SObject_Meta; }
protected:

public:
    
}; // end class SObject




END_NAMESPACE_EVEREST