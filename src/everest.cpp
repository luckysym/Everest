#include <everest/application.h>
#include <string.h>
#include <iostream>

using namespace std;

namespace everest
{
    static const char * const sg_default_value_1 = "1";
    
    Application :: Application(int argc, char **argv)
        : m_ctx(new ApplicationContext)
    {
        Properties & props = m_ctx->properties();
        for(int i = 1 ; i < argc; ++i ) {
            if ( argv[i][0] == '-' && argv[i][1] == '-' ) {
                int pn_len, pv_len;
                const char * pn = argv[i] + 2;
                const char * pv = strchr(pn, '=');
                if ( pv == nullptr ) {
                    pv = sg_default_value_1;
                    pn_len = strlen(pn);
                    pv_len = 1;
                } else {
                    pn_len = pv - pn;
                    pv += 1;
                    if ( pv[0] == '\0' ) {
                        pv = sg_default_value_1;
                        pv_len = 1;
                    } else {
                        pv_len = strlen(pv);
                    }
                }
                props.add(pn, pn_len, pv, pv_len);
            }
        } // end for 
    } // end of Application()
    
    Application :: ~Application () {}
    
    void Application::run(PFN_MAIN proc) 
    {
        int ret = proc(*m_ctx);
        m_ctx->result(ret);
        return ;
    }
    
    ApplicationContext :: ApplicationContext () {}
    ApplicationContext :: ~ApplicationContext() {}
    
} // end of namespace everest 