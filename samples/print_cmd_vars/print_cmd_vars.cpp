#include <everest/application.h>
#include <iostream>

int app_main(everest::ApplicationContext & ctx)
{
    using namespace std;
    using namespace everest;
    
    Properties & props = ctx.properties();
    
    Properties::Iterator iter = props.begin();
    while ( iter != props.end() ) {
        cout<<iter->name<<" = " <<iter->value<<endl;
        ++iter;
    }
    
    return 0;
} // app_main()

int main(int argc, char **argv) 
{
    using namespace everest;
    using namespace std;
    
    cout<<"start print_cmd_vars"<<endl;
    Application app(argc, argv);
    app.run(app_main);
    cout<<"end print_cmd_vars "<<app.result()<<endl;
    return app.result();
}
