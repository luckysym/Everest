#include <everest/SObject.h>
#include <iostream>
#include <string.h>

using namespace everest;
using namespace std;

namespace test {

    class ITest2 {
        EVEREST_DECLARE_INTERFACE_META(ITest2)
    public:
        virtual ~ITest2() {}
        virtual void Show () = 0;
    }; // end class ITest2

    EVEREST_BEGIN_IMPL_INTERFACE_META_WITH_NS(test, ITest2, nullptr)
    EVEREST_END_IMPL_INTERFACE_META(ITest2)

} // end namespace test


class ITestIntf : public virtual test::ITest2 {
    EVEREST_DECLARE_INTERFACE_META(ITestIntf)
public:
    virtual ~ITestIntf() {}

    virtual void Do() = 0;
};

EVEREST_BEGIN_IMPL_INTERFACE_META(ITestIntf, nullptr)
    EVEREST_ADD_INTERFACE(test::ITest2);
EVEREST_END_IMPL_INTERFACE_META(ITestIntf)


class STestObject : public SObject, public virtual ITestIntf {
    EVEREST_DECLARE_CLASS_META(STestObject)
public:
    virtual void Do() {  }
    virtual void Show() { }
};

// 声明META CLASS的实现，WITHOUT_NS表示不带名称空间，参数为当前类，以及超类的MetaClass。
// Everest对象参考Java的继承规则，只能直接继承一个超类。
EVEREST_BEGIN_IMPL_CLASS_META(STestObject, SObject::Class())
    EVEREST_ADD_INTERFACE(ITestIntf);
EVEREST_END_IMPL_CLASS_META(STestObject)


int main(int argc, char **argv) 
{
    // 1
    SObject testobj;
    const MetaClass * m = testobj.GetMetaClass();
    cerr<<"1: SObject Meta: "<<m<<endl;
    const char *clsname = testobj.GetMetaClass()->GetClassName();
    cerr<<"1-1: "<<clsname<<endl;
    int r = strcmp("everest::SObject", clsname);
    if ( r != 0 ) return r;
 
    // 2
    STestObject testobj2;
    SObject *pobj2 = &testobj2;
    clsname = testobj2.GetMetaClass()->GetClassName();
    cerr<<"2. "<<clsname<<endl;
    r = strcmp("STestObject", clsname);
    if ( r != 0 ) return r;

    // 3
    clsname = pobj2->GetMetaClass()->GetClassName();
    cerr<<"3: "<<clsname<<endl;
    r = strcmp("STestObject", clsname);
    if ( r != 0 ) return r;

    m = testobj.GetMetaClass();
    cerr<<"3-1: "<<m<<endl;
    
    // 4
    clsname = testobj2.GetMetaClass()->GetSuperClass()->GetClassName();
    // cerr<<"4-0: "<<(void *)clsname<<endl;
    m = testobj.GetMetaClass();
    cerr<<"4-1: "<<m<<endl;
    cerr<<"4: "<<clsname<<endl;
    r = strcmp("everest::SObject", clsname);    
    if ( r != 0 ) return r;
    
    // 5
    bool isok = pobj2->InstanceOf(SObject::Class());
    cerr<<"5: "<<isok<<endl;

    // 6
    cerr<<pobj2->ToString()<<endl;
    cerr<<testobj.ToString()<<endl;
    // return -1;
    
    return isok?0:-1;
}
