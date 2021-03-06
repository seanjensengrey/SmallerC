/*
  How to compile for DOS (huge and small memory models):
    smlrcc -dosh exc.c -o exc.exe
    smlrcc -doss exc.c -o exc.exe

  How to compile for Windows:
    smlrcc -win exc.c -o exc.exe
*/

#include <stdio.h>

static volatile int zero = 0, one = 1;

int main(int argc, char* argv[])
{
  int i;

  if (argc > 1)
  {
    for (i = 1; i <= 3; i++)
    {
      printf("Iteration %d of 3\n", i);

      if (i == 2)
      {
        printf("Invoking #%s...\n", argv[1]);

        if (!strcmp(argv[1], "DE"))
        {
          one /= zero; // #DE
        }
        else if (!strcmp(argv[1], "BP"))
        {
          asm("int3"); // #BP
        }
        else if (!strcmp(argv[1], "OF"))
        {
          asm("mov al, 0x7f\n"
              "add al, al\n"
              "into"); // #OF
        }
        else if (!strcmp(argv[1], "UD"))
        {
          asm("ud2"); // #UD
        }
      }
    }
  }
  else
  {
    system("exc.exe DE");
    system("exc.exe BP");
    system("exc.exe OF");
    system("exc.exe UD");
  }

  return 0;
}
