#include <stdio.h>

void FortranConstructor() __attribute__((weak)) ;
void FortranDestructor() __attribute__((weak)) ;

char *EntryList_[1] = {NULL} ;  // empty entry list

#pragma weak name1=_name_0_1
int name1(int arg);
int _name_0_1(int arg){
fprintf(stderr, "C _name_0_1: %d\n",arg);
arg += 100 ;
return(arg);
}

#pragma weak name2=_name_0_2
int name2(int arg);
int _name_0_2(int arg){
fprintf(stderr, "C _name_0_2: %d\n",arg);
arg += 100 ;
return(arg);
}

#pragma weak name3=_name_0_3
int name3(int arg);
int _name_0_3(int arg){
fprintf(stderr, "C _name_0_3: %d\n",arg);
arg += 100 ;
return(arg);
}

void __attribute__ ((constructor)) Constructor0(void) {
   fprintf(stderr, "plugin constructor for plugin_0 : ");
   if( FortranConstructor ){
     fprintf(stderr, "FortranConstructor is Available [%p]\n", (void *)&FortranConstructor) ;
     FortranConstructor() ;
   }else{
     fprintf(stderr, "FortranConstructor is NOT FOUND\n") ;
   }
}

void __attribute__ ((destructor)) Destructor0(void) {
   fprintf(stderr, "plugin destructor for plugin_0 : ");
   if( FortranDestructor ){
     fprintf(stderr, "FortranDestructor is Available [%p]\n", (void *)&FortranDestructor) ;
     FortranDestructor() ;
   }else{
     fprintf(stderr, "FortranDestructor is NOT FOUND\n") ;
   }
}
