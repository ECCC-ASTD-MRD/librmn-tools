#include <stdio.h>

void FortranConstructor() __attribute__((weak)) ;
void FortranDestructor() __attribute__((weak)) ;

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
   printf("plugin constructor for plugin_2 : ");
   if( FortranConstructor ){
     printf("FortranConstructor is Available [%p]\n", &FortranConstructor) ;
     FortranConstructor() ;
   }else{
     printf("FortranConstructor is NOT FOUND\n") ;
   }
}

void __attribute__ ((destructor)) Destructor2(void) {
   printf("plugin destructor for plugin_2 : ");
   if( FortranDestructor ){
     printf("FortranDestructor is Available [%p]\n", &FortranDestructor) ;
     FortranDestructor() ;
   }else{
     printf("FortranDestructor is NOT FOUND\n") ;
   }
}

