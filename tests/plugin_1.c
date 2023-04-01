#include <stdio.h>
#include <string.h>

int FortranConstructor1(int) __attribute__((weak)) ;

// list of names, NULL terminated, mandatory
char *EntryList_[4] = { "name1","name2","name3",NULL} ;

#pragma weak name1=_name_1_1
int name1(int arg);
int _name_1_1(int arg){
printf("C _name_1_1: %d\n",arg);
arg += 200 ;
return(arg);
}

#pragma weak name2=_name_1_2
int name2(int arg);
int _name_1_2(int arg){
printf("C _name_1_2: %d\n",arg);
arg += 200 ;
return(arg);
}

#pragma weak name3=_name_1_3
int name3(int arg);
int _name_1_3(int arg){
printf("C _name_1_3: %d\n",arg);
arg += 200 ;
return(arg);
}

void *AddressList_[4] = {_name_1_1, _name_1_2, _name_1_3, NULL} ;

int get_symbol_number(){  // like fortran, function to get number of symbols, optional
  return(3);
}

void __attribute__ ((constructor)) Constructor1(void) {
   printf("plugin constructor for plugin_1\n");
   if( FortranConstructor1 ){
     printf("FortranConstructor1 is Available [%p]\n", &FortranConstructor1) ;
     FortranConstructor1(123456) ;
   }else{
     printf("FortranConstructor1 is NOT FOUND\n") ;
   }
}

void __attribute__ ((destructor)) Destructor1(void) {
   printf("plugin destructor for plugin_1\n");
}
