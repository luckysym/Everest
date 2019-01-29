#pragma once

#include <assert.h>
#include <string>

#define EVEREST_NAMESPACE everest 
#define BEGIN_NAMESPACE_EVEREST namespace everest {
#define END_NAMESPACE_EVEREST }
#define TO_STR(a)  #a
#define STR(a)  TO_STR(a)

// 声明一个EVEREST MetaClass
#define EVEREST_DECLARE_CLASS_META(cls) \
    private: \
        class cls##_MetaClass : public EVEREST_NAMESPACE::MetaClass { \
        public: \
            cls##_MetaClass(); \
            virtual EVEREST_NAMESPACE::SObject * CreateInstance() { return new cls(); } \
        }; \
        static MetaClass * const S_##cls##_Meta; \
    public: \
        static  const EVEREST_NAMESPACE::MetaClass * Class() { return S_##cls##_Meta; } \
        virtual const EVEREST_NAMESPACE::MetaClass * GetMetaClass() const { return S_##cls##_Meta; } \
    protected: \

// 声明一个EVEREST接口类型的MetaClass
#define EVEREST_DECLARE_INTERFACE_META(cls) \
    private: \
        class cls##_MetaClass : public EVEREST_NAMESPACE::MetaClass { \
        public: \
            cls##_MetaClass(); \
            virtual EVEREST_NAMESPACE::SObject * CreateInstance() { \
                assert("interface can't be instantiating" == nullptr); \
            } \
        }; \
        static EVEREST_NAMESPACE::MetaClass * const S_##cls##_Meta; \
    public: \
        static  const EVEREST_NAMESPACE::MetaClass * Class() { return S_##cls##_Meta; } \
        virtual const EVEREST_NAMESPACE::MetaClass * GetMetaClass() const { return S_##cls##_Meta; } \
    protected: \


#define EVEREST_BEGIN_IMPL_CLASS_META_WITH_NS(ns, cls, supercls) \
    cls::cls##_MetaClass::cls##_MetaClass() \
        : EVEREST_NAMESPACE::MetaClass(MetaClass::Class, STR(ns)"::"#cls, supercls) {  \

// 开始实现一个EVEREST CLASS的MetaClass, cls不带名称空间
#define EVEREST_BEGIN_IMPL_CLASS_META(cls, supercls) \
    cls::cls##_MetaClass::cls##_MetaClass() \
        : EVEREST_NAMESPACE::MetaClass(MetaClass::Class, #cls, supercls) {  \

// 结束一个EVEREST CLASS的MetaClass的实现
#define EVEREST_END_IMPL_CLASS_META(cls) \
        }  \
    EVEREST_NAMESPACE::MetaClass * const cls::S_##cls##_Meta =  new cls##_MetaClass(); \


// 开始实现一个EVEREST CLASS的MetaClass, cls不带名称空间
#define EVEREST_BEGIN_IMPL_INTERFACE_META(cls, supercls) \
    cls::cls##_MetaClass::cls##_MetaClass() \
        : EVEREST_NAMESPACE::MetaClass(MetaClass::Interface, #cls, supercls) {  \

#define EVEREST_BEGIN_IMPL_INTERFACE_META_WITH_NS(ns, cls, supercls) \
    cls::cls##_MetaClass::cls##_MetaClass() \
        : EVEREST_NAMESPACE::MetaClass(MetaClass::Interface, STR(ns)"::"#cls, supercls) {  \

// 结束一个EVEREST INTERFACE的MetaClass的实现
#define EVEREST_END_IMPL_INTERFACE_META(cls) EVEREST_END_IMPL_CLASS_META(cls)


// 增加一个实现的接口，放在EVEREST_BEGIN_IMPL_CLASS_META/EVEREST_END_IMPL_CLASS_META之间
#define EVEREST_ADD_INTERFACE(intf) AddInterface(intf::Class())




BEGIN_NAMESPACE_EVEREST

class SObject;

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
    const MetaClass  *m_pSuperClass;
    Node<const MetaClass *> * m_pInterfaceNode;

public:
    MetaClass(ClassType type, const char *pszClassName, const MetaClass *pSuperClass = nullptr);

    ClassType    GetClassType() const { return m_eType; }
    const char * GetClassName() const { return m_pszClassName; }
    const MetaClass * GetSuperClass() const { return m_pSuperClass; }
    const Node<const MetaClass *> * GetInterfaceNode() const { return m_pInterfaceNode; }

    void AddInterface(const MetaClass *pMeta) { 
        assert(pMeta->GetClassType() == Interface);
        Node<const MetaClass*> * node = new Node<const MetaClass*>();
        node->pData = pMeta;
        node->pNext = m_pInterfaceNode;
        m_pInterfaceNode = node;
    }

    const MetaClass * FindSuper(const MetaClass * pMeta) const;

    bool operator==(const MetaClass& other) const;

    virtual SObject * CreateInstance() = 0;
}; // end class MetaClass

class SObject {
    EVEREST_DECLARE_CLASS_META(SObject)
public:
    bool InstanceOf(const MetaClass * cls);

public:
    virtual std::string ToString() const;

}; // end class SObject



END_NAMESPACE_EVEREST