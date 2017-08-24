#ifndef INCLUDED_EVEREST_APPLICATION_H
#define INCLUDED_EVEREST_APPLICATION_H

#pragma once 

#include <string>
#include <memory>
#include <set>

namespace everest
{
    class Application;
    class ApplicationContext;
    class Properties;
    
    typedef std::string String;
    
    class Properties
    {
    public:
        struct Item
        {
            String  name;
            String  value;
        };
        
        struct ItemLess {
            
            bool operator () (const Item & t1, const Item &t2) const {
                if ( t1.name < t2.name ) return true;
                else return false;
            }
        };
    
        typedef std::set<Item, ItemLess> Container;
        
        struct Iterator : Container::iterator 
        {
            Iterator() {};
            Iterator(const Container::iterator& iter) :  Container::iterator(iter) {}
            
            const Item * operator -> () const {
                return Container::iterator::operator->();
            }
        };
        
        struct ConstIterator : Container::const_iterator 
        {
            ConstIterator() {};
            ConstIterator(const Container::const_iterator& iter) :  Container::const_iterator(iter) {}
            
            const Item * operator -> () const {
                return Container::const_iterator::operator->();
            }
        };
        
    private:
        Container  m_container;
        
    public:
        Properties()  {}
        ~Properties() {}
        
        size_t count() const { return m_container.size(); }
        
        bool   exist(const char * name) const {
            return m_container.find(Item{String(name)}) != m_container.end();
        }
        
        Iterator begin() { return Iterator(m_container.begin()); }
        ConstIterator begin() const { return Iterator(m_container.begin());}
        
        Iterator end() { return Iterator(m_container.end());}
        ConstIterator end() const { return Iterator(m_container.end());}
        
        String get(const char *name) const {
            Container::iterator it = m_container.find(Item{String(name)});
            if ( it != m_container.end()) return it->value;
            else return String();
        }
        
        bool add(const char * name, const char *value) {
            Item itm;
            itm.name = name;
            itm.value = value;
            std::pair<Container::iterator, bool> ret = m_container.insert(itm);
            return ret.second;
        }
        
        bool add(const char * name, size_t namelen, const char *value, size_t valuelen) {
            Item itm;
            itm.name.assign(name, namelen);
            itm.value.assign(value, valuelen);
            std::pair<Container::iterator, bool> ret = m_container.insert(itm);
            return ret.second;
        }
        
    }; // end of class Properties
    
    
    class ApplicationContext
    {
    private:
        int        m_result;      // 程序运行结果值
        Properties m_props;
        
    public:
        ApplicationContext();
        ~ApplicationContext();
        
        Properties & properties() { return m_props; }
        const Properties & properties() const { return m_props; }
        
        int result() const { return m_result; }
        void result(int ret) { m_result = ret; }
    }; // end of ApplicationContext
    
    
    class Application
    {
    public:
        typedef int (*PFN_MAIN)(ApplicationContext &);
    
    private:
        ApplicationContext * m_ctx;
        
    public:
        Application(int argc, char **argv);
        ~Application();
        
        void run(PFN_MAIN proc);
        int  result() const { return m_ctx->result(); }
    }; // end of class Application
    

    
    
} // end of namespace everest 

#endif // INCLUDED_EVEREST_APPLICATION_H