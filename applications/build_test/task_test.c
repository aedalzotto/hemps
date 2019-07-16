#include <api.h>
#include <stdlib.h>

Message msg;

int main(){
    volatile int a = 8;
	volatile int b = 3;
	volatile int c = b + a;

    Echo(itoa(GetTick()));
    
    exit();
}
