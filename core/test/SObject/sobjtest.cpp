#include <everest/SObject.h>
#include <iostream>
#include <string.h>

using namespace everest;
using namespace std;

class ITestIntf {
private:
    class ITestIntf_MetaClass : public MetaClass {
    public:
        ITestIntf_MetaClass();
    };
    static MetaClass * S_ITestIntf_Meta;
public:
    static  MetaClass * Meta() { return S_ITestIntf_Meta; }
    virtual MetaClass * GetMetaClass() { return S_ITestIntf_Meta; }
public:
    virtual ~ITestIntf() {}

    virtual void Do() = 0;
};

ITestIntf::ITestIntf_MetaClass::ITestIntf_MetaClass()
    : MetaClass(MetaClass::Interface, "ITestIntf") {}

MetaClass * ITestIntf::S_ITestIntf_Meta =  new ITestIntf_MetaClass() ;

class STestObject : public SObject, public virtual ITestIntf {
private:
    class STestObject_MetaClass : public MetaClass {
    public:
        STestObject_MetaClass();
    }; // end class STestObject_MetaClass
    static MetaClass * S_STestObject_Meta;
public:
    virtual MetaClass * GetMetaClass() { return S_STestObject_Meta; }

public:
    virtual void Do() {  }
};

STestObject::STestObject_MetaClass::STestObject_MetaClass()
    : MetaClass(MetaClass::Class, "STestObject") 
{
    MetaClass::SetSuperClass(SObject::Meta());
    MetaClass::AddInterface(ITestIntf::Meta());
}

MetaClass * STestObject::S_STestObject_Meta = new STestObject_MetaClass();

int main(int argc, char **argv) 
{
    // 1
    SObject testobj;
    const char *clsname = testobj.GetMetaClass()->GetClassName();
    cerr<<clsname<<endl;
    int r = strcmp("everest.SObject", clsname);    
    if ( r != 0 ) return r;

    // 2
    STestObject testobj2;
    SObject *pobj2 = &testobj2;
    clsname = testobj2.GetMetaClass()->GetClassName();
    cerr<<clsname<<endl;
    r = strcmp("STestObject", clsname);
    if ( r != 0 ) return r;
    
    // 3
    clsname = pobj2->GetMetaClass()->GetClassName();
    cerr<<clsname<<endl;
    r = strcmp("STestObject", clsname);
    if ( r != 0 ) return r;

    // 4
    clsname = testobj2.GetMetaClass()->GetSuperClass()->GetClassName();
    cerr<<clsname<<endl;
    r = strcmp("everest.SObject", clsname);    
    if ( r != 0 ) return r;

}
