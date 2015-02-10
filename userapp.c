#include "userapp.h"

int main(int argc, char* argv[])
{
   unsigned long pid = getpid();

   system("echo '%u'>/proc/mp1/status", pid);
   return 0;
}
