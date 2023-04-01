#include <stdio.h>

int FortranConstructor2(int) __attribute__((weak)) ;

char *EntryList_[4] = { "name1","name2","name3",NULL} ;   // fully static method

#pragma weak name1=_name_2_1
int name1(int arg);
int _name_2_1(int arg){
printf("C _name_2_1: %d\n",arg);
arg += 300 ;
return(arg);
}

#pragma weak name2=_name_2_2
int name2(int arg);
int _name_2_2(int arg){
printf("C _name_2_2: %d\n",arg);
arg += 300 ;
return(arg);
}

#pragma weak name3=_name_2_3
int name3(int arg);
int _name_2_3(int arg){
printf("C _name_2_3: %d\n",arg);
arg += 300 ;
return(arg);
}

void __attribute__ ((constructor)) Constructor2(void) {
   printf("plugin constructor for plugin_2\n");
   if( FortranConstructor2 ){
     printf("FortranConstructor2 is Available [%p]\n", &FortranConstructor2) ;
     FortranConstructor2(123456) ;
   }else{
     printf("FortranConstructor2 is NOT FOUND\n") ;
   }
}

void __attribute__ ((destructor)) Destructor2(void) {
   printf("plugin destructor for plugin_2\n");
}

