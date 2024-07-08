#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <rmn/test_helpers.h>
#include <rmn/function_pointers.h>

int main(int argc, char **argv){
  void *plugin0, *plugin1, *plugin2 ;
  proc_ptr name1_0, name2_0, name3_0 ;
  proc_ptr name1_1, name2_1, name3_1 ;
  proc_ptr name1_2, name2_2, name3_2 ;

  start_of_test(" plugins C");
  plugin0 = dlopen("libplugin0.so", RTLD_LAZY) ;
  if(plugin0 == NULL) {
    fprintf(stderr, "ERROR : plugin0 open failed '%s'\n", dlerror()) ;
    exit(1) ;
  }
  plugin1 = dlopen("libplugin1.so", RTLD_LAZY) ;
  if(plugin1 == NULL) {
    fprintf(stderr, "ERROR : plugin1 open failed '%s'\n", dlerror()) ;
    exit(1) ;
  }
  plugin2 = dlopen("libplugin2.so", RTLD_LAZY) ;
  if(plugin2 == NULL) {
    fprintf(stderr, "ERROR : plugin2 open failed '%s'\n", dlerror()) ;
    exit(1) ;
  }
  fprintf(stderr, "plugin addresses = %p %p %p\n", plugin0, plugin1, plugin2) ;

  name1_0 = (proc_ptr) data2proc_ptr( dlsym(plugin0, "name1") ) ; fprintf(stderr, "%d\n", (*name1_0)(1) ) ;
  if(name1_0 == NULL) exit(1) ;
  name2_0 = (proc_ptr) data2proc_ptr( dlsym(plugin0, "name2") ) ; fprintf(stderr, "%d\n", (*name2_0)(2) ) ;
  if(name2_0 == NULL) exit(1) ;
  name3_0 = (proc_ptr) data2proc_ptr( dlsym(plugin0, "name3") ) ; fprintf(stderr, "%d\n", (*name3_0)(3) ) ;
  if(name3_0 == NULL) exit(1) ;
  name1_1 = (proc_ptr) data2proc_ptr( dlsym(plugin1, "name1") ) ; fprintf(stderr, "%d\n", (*name1_1)(1) ) ;
  if(name1_1 == NULL) exit(1) ;
  name2_1 = (proc_ptr) data2proc_ptr( dlsym(plugin1, "name2") ) ; fprintf(stderr, "%d\n", (*name2_1)(2) ) ;
  if(name2_1 == NULL) exit(1) ;
  name3_1 = (proc_ptr) data2proc_ptr( dlsym(plugin1, "name3") ) ; fprintf(stderr, "%d\n", (*name3_1)(3) ) ;
  if(name3_1 == NULL) exit(1) ;
  name1_2 = (proc_ptr) data2proc_ptr( dlsym(plugin2, "name1") ) ; fprintf(stderr, "%d\n", (*name1_2)(1) ) ;
  if(name1_2 == NULL) exit(1) ;
  name2_2 = (proc_ptr) data2proc_ptr( dlsym(plugin2, "name2") ) ; fprintf(stderr, "%d\n", (*name2_2)(2) ) ;
  if(name2_2 == NULL) exit(1) ;
  name3_2 = (proc_ptr) data2proc_ptr( dlsym(plugin2, "name3") ) ; fprintf(stderr, "%d\n", (*name3_2)(3) ) ;
  if(name3_2 == NULL) exit(1) ;

}
