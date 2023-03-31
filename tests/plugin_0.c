#include <stdio.h>

char *EntryList_[1] = {NULL} ;  // empty entry list

#pragma weak name1=_name_0_1
int name1(int arg);
int _name_0_1(int arg){
printf("C _name_0_1: %d\n",arg);
arg += 100 ;
return(arg);
}

#pragma weak name2=_name_0_2
int name2(int arg);
int _name_0_2(int arg){
printf("C _name_0_2: %d\n",arg);
arg += 100 ;
return(arg);
}

#pragma weak name3=_name_0_3
int name3(int arg);
int _name_0_3(int arg){
printf("C _name_0_3: %d\n",arg);
arg += 100 ;
return(arg);
}

void __attribute__ ((constructor)) Constructor0(void) {
   printf("plugin constructor for plugin_0\n");
}

void __attribute__ ((destructor)) Destructor0(void) {
   printf("plugin destructor for plugin_0\n");
}