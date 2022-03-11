#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h#include <elf.h>
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

Elf64_Shdr *lookup_section(const char *name)
{
  Elf64_Shdr *shdr = load_base + ehdr->e_shoff;
  char *shstrtab = load_base + shdr[ehdr->e_shstrndx].sh_offset;
  
  for (int i = 0; i < ehdr->e_shnum; ++i, ++shdr) {
    const char *section_name = shstrtab + shdr->sh_name; 
    if (!strcmp(name, section_name)) break;
  }

  return shdr;
}

void parse_elf()
{
  Elf64_Shdr *shdr;
  
  // allocate a memory for .text and .got.plt
  shdr = lookup_section(".got.plt");
  run_base = mmap(NULL, shdr->sh_size + shdr->sh_addr, PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (run_base == MAP_FAILED) {
      perror("Failed to allocate memory for .text");
      exit(1);
  }

  // copy .text from the ELF file
  shdr = lookup_section(".text");
  memcpy(run_base + shdr->sh_offset, load_base + shdr->sh_offset, shdr->sh_size);

  // copy .plt from the ELF file
  shdr = lookup_section(".plt");
  memcpy(run_base + shdr->sh_offset, load_base + shdr->sh_offset, shdr->sh_size);
  
  // copy .got.plt from the ELF file
  shdr = lookup_section(".got.plt");
  memcpy(run_base + shdr->sh_addr, load_base + shdr->sh_offset, shdr->sh_size);
  
  // find .symtab
  shdr = lookup_section(".symtab");
  Elf64_Sym *symbol_table = load_base + shdr->sh_offset;
  int num_symbols = shdr->sh_size / shdr ->sh_entsize;

  // find .strtab
  shdr = lookup_section(".strtab");
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

void do_relocation()
{
  // find .rela.plt
  Elf64_Shdr *shdr = lookup_section(".rela.plt");
  Elf64_Rela *rela_plt = load_base + shdr->sh_offset;
  int num_of_relocation = shdr->sh_size / shdr ->sh_entsize;
  
  shdr = lookup_section(".dynsym");
  Elf64_Sym *dynsym_table = load_base + shdr->sh_offset;
  
  for (int i = 0 ; i < num_of_relocation; ++i, ++rela_plt) {
    
    int relocation_type = ELF64_R_TYPE(rela_plt->r_info);
    int symbol_index = ELF64_R_SYM(rela_plt->r_info);
    
    switch (relocation_type) {  
      case R_X86_64_JUMP_SLOT: {
        Elf64_Addr patch_value = (Elf64_Addr)run_base + dynsym_table[symbol_index].st_value;
        Elf64_Addr *patch_offset = (Elf64_Addr*)(run_base + rela_plt->r_offset);
        *patch_offset = patch_value;
        //printf("0x%p, %lx\n", patch_offset, patch_value);
        break;
      }
      
      default:
        perror("not supported relocation type");
        exit(1);
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
  do_relocation();
  execute_func();

  return 0;
}