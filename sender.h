#include <stdio.h>

#include "ApplicationLayer.h"
#include "tools.h"
//sets Al struct with paramenters
void Al_setter();

//sets tlv parameters
void tlv_setter( ControlPackage *tlv);

void sender(int fd);