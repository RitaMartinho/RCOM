#include <stdio.h>

#include "ApplicationLayer.h"
#include "tools.h"
//sets Al struct with paramenters
int Al_setter();

//sets tlv parameters
void tlv_setter( ControlPackage *tlv);

int sender(int fd);