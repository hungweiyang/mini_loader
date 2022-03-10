#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>
#include <string.h>

void *load_base;
char *run_base;
Elf64_Ehdr *ehdr;
int (*func)(int);

void load_elf()
{
  int fd = open("app.so", O_RDONLY);
  struct stat sb;
  fstat(fd, &sb);
  ehdr = load_base = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  
  if (load_base == MAP_FAILED) {
      perror("Failed to allocate memory for app.so");
      exit(1);
  }
}

void parse_elf()
{
  // find .shstrtab first
  Elf64_Shdr *shdr = load_base + ehdr->e_shoff;
  char *shstrtab = load_base + shdr[ehdr->e_shstrndx].sh_offset;
  
  for (int i = 0; i < ehdr->e_shnum; ++i, ++shdr) {
    const char *section_name = shstrtab + shdr->sh_name; 
    if (!strcmp(".text", section_name)) break;
  }

  // allocate a memory for .text
  run_base = mmap(NULL, shdr->sh_size + shdr->sh_offset, PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (run_base == MAP_FAILED) {
      perror("Failed to allocate memory for .text");
      exit(1);
  }

  // copy the .text
  memcpy(run_base + shdr->sh_offset, load_base + shdr->sh_offset, shdr->sh_size);

  // find .symtab
  shdr = load_base + ehdr->e_shoff;
  for (int i = 0; i < ehdr->e_shnum; ++i, ++shdr) {
    const char *section_name = shstrtab + shdr->sh_name; 
    if (!strcmp(".symtab", section_name)) break;
  }

  Elf64_Sym *symbol_table = load_base + shdr->sh_offset;
  int num_symbols = shdr->sh_size / shdr ->sh_entsize;

  // find .strtab
  shdr = load_base + ehdr->e_shoff;
  for (int i = 0; i < ehdr->e_shnum; ++i, ++shdr) {
    const char *section_name = shstrtab + shdr->sh_name; 
    if (!strcmp(".strtab", section_name)) break;
  }
  
  char *strtab = load_base + shdr->sh_offset;

  // find add10()
  for (int i = 0; i < num_symbols; ++i) {
    const char *symbol_name = strtab + symbol_table[i].st_name;
    if (!strcmp("add10", symbol_name)) {
      func = (int(*)(int))(run_base + symbol_table[i].st_value);
      break;
    }
  }
}

void execute_func()
{
  printf("func = %d\n", func(10));
}

int main()
{
  load_elf();
  parse_elf();
  execute_func();

  return 0;
}