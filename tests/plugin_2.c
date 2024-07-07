#include <stdio.h>
#include <stdint.h>

void FortranConstructor() __attribute__((weak)) ;
void FortranDestructor() __attribute__((weak)) ;

char *EntryList_[4] = { "name1","name2","name3",NULL} ;   // fully static method

#pragma weak name1=_name_2_1
int name1(int arg);
int _name_2_1(int arg){
fprintf(stderr, "C _name_2_1: %d\n",arg);
arg += 300 ;
return(arg);
}

#pragma weak name2=_name_2_2
int name2(int arg);
int _name_2_2(int arg){
fprintf(stderr, "C _name_2_2: %d\n",arg);
arg += 300 ;
return(arg);
}

#pragma weak name3=_name_2_3
int name3(int arg);
int _name_2_3(int arg){
fprintf(stderr, "C _name_2_3: %d\n",arg);
arg += 300 ;
return(arg);
}

void __attribute__ ((constructor)) Constructor2(void) {
   fprintf(stderr, "plugin constructor for plugin_2 : ");
   if( FortranConstructor ){
     fprintf(stderr, "FortranConstructor is Available [%16ld]\n", (uint64_t)&FortranConstructor) ;
     FortranConstructor() ;
   }else{
     fprintf(stderr, "FortranConstructor is NOT FOUND\n") ;
   }
}

void __attribute__ ((destructor)) Destructor2(void) {
   fprintf(stderr, "plugin destructor for plugin_2 : ");
   if( FortranDestructor ){
     fprintf(stderr, "FortranDestructor is Available [%16ld]\n", (uint64_t)&FortranDestructor) ;
     FortranDestructor() ;
   }else{
     fprintf(stderr, "FortranDestructor is NOT FOUND\n") ;
   }
}

