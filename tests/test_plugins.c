#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <rmn/test_helpers.h>

typedef int (*fptr)(int) ;

int main(int argc, char **argv){
  void *plugin0, *plugin1, *plugin2 ;
  fptr name1_0, name2_0, name3_0 ;
  fptr name1_1, name2_1, name3_1 ;
  fptr name1_2, name2_2, name3_2 ;

  start_of_test("test_plugins C");
  plugin0 = dlopen("libplugin0.so", RTLD_LAZY) ;
  if(plugin0 == NULL) printf("ERROR : plugin0 open failed '%s'\n", dlerror()) ;
  plugin1 = dlopen("libplugin1.so", RTLD_LAZY) ;
  if(plugin1 == NULL) printf("ERROR : plugin1 open failed '%s'\n", dlerror()) ;
  plugin2 = dlopen("libplugin2.so", RTLD_LAZY) ;
  if(plugin2 == NULL) printf("ERROR : plugin2 open failed '%s'\n", dlerror()) ;
  printf("plugin addresses = %p %p %p\n", plugin0, plugin1, plugin2) ;

  name1_0 = (fptr) dlsym(plugin0, "name1") ; printf("%d\n", (*name1_0)(1) ) ;
  name2_0 = (fptr) dlsym(plugin0, "name2") ; printf("%d\n", (*name2_0)(2) ) ;
  name3_0 = (fptr) dlsym(plugin0, "name3") ; printf("%d\n", (*name3_0)(3) ) ;
  name1_1 = (fptr) dlsym(plugin1, "name1") ; printf("%d\n", (*name1_1)(1) ) ;
  name2_1 = (fptr) dlsym(plugin1, "name2") ; printf("%d\n", (*name2_1)(2) ) ;
  name3_1 = (fptr) dlsym(plugin1, "name3") ; printf("%d\n", (*name3_1)(3) ) ;
  name1_2 = (fptr) dlsym(plugin2, "name1") ; printf("%d\n", (*name1_2)(1) ) ;
  name2_2 = (fptr) dlsym(plugin2, "name2") ; printf("%d\n", (*name2_2)(2) ) ;
  name3_2 = (fptr) dlsym(plugin2, "name3") ; printf("%d\n", (*name3_2)(3) ) ;

}
