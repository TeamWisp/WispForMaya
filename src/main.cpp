#include <maya/MSimple.h>
#include <maya/MIOStream.h>

DeclareSimpleCommand(helloWorld, "Team Wisp", "2018/2019");

MStatus helloWorld::doIt(const MArgList&)
{
    cout << "Hello, world!" << endl;
    return MS::kSuccess;
}