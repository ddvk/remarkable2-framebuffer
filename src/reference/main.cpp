#include <QCoreApplication>
#include <QImage>
#include <iostream>
#include <fbmanager.h>

using namespace std;

int main()
{
    FBManager fb;
    fb.SendUpdate();
    cout << "Done" << endl;
    return 0; 
}
