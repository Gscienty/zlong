#include "http/interface.h"
#include "cargs.h" 

int main(int argc, char ** argv)
{
    return cargs_run(argc, argv, NULL);
}
