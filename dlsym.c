#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <link.h>
#include <dlfcn.h>

Elf64_Addr my_dlsym(const char *symbol_name)
{
  void *handle;
  struct link_map *it;
  
  handle = dlopen(NULL, RTLD_NOW);
  if (handle == NULL) {
    fprintf(stderr, "dlopen() failed: %s\n", dlerror());
    exit(1);
  }

  /* Discover the size of the buffer that we must pass to
     RTLD_DI_SERINFO. */
  if (dlinfo(handle, RTLD_DI_LINKMAP, &it) == -1) {
    fprintf(stderr, "RTLD_DI_LINKMAP failed: %s\n", dlerror());
    exit(1);
  }

  for (; it; it = it->l_next) {
    if (!strcmp(it->l_name, "") || !strcmp(it->l_name, "linux-vdso.so.1"))
      continue;

    Elf64_Dyn *d;
    char *strtab = 0;
    Elf64_Sym *symtab = 0;
    unsigned int num_of_sym = 0, sym_size = 0;

    for (d = it->l_ld; d->d_tag != DT_NULL; ++d) {
      if (d->d_tag == DT_STRTAB) {
        strtab = (char*)d->d_un.d_ptr;
        // printf("strtab: %p\n", strtab);
      }

      if (d->d_tag == DT_SYMTAB) {
        symtab = (Elf64_Sym*)d->d_un.d_ptr;
        // printf("symtab: %p\n", symtab);
      }

      if (d->d_tag == DT_SYMENT) {
        sym_size = d->d_un.d_val;
        num_of_sym = (strtab - (char*)symtab) / sym_size;
        // printf("num of symbol: %u\n", num_of_sym);
      }
    }

    Elf64_Sym *s = symtab;
    Elf64_Addr symbol_addr = 0;

    for (unsigned int i = 0; i < num_of_sym; i++) {
      // if (s->st_name)
      //     printf("%s, 0x%lx\n", &strtab[s->st_name], s->st_value);

      if (s->st_name && !strcmp(&strtab[s->st_name], symbol_name) && s->st_shndx != SHN_UNDEF) {
          symbol_addr = (Elf64_Addr)(it->l_addr + s->st_value);
          printf("Found %s at 0x%lx\n", symbol_name, symbol_addr);
          return symbol_addr;
      }

      s = (Elf64_Sym*)((char*)s + sym_size);
    }
  }

  exit(1);
}