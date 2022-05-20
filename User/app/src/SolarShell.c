#include "string.h"

typedef void (* shellFunc) (void);
typedef void* CommandParaTypeDef;

typedef struct{
    char *name;
    shellFunc func;
    char *desc;
}SHELL_CommandTypeDef;



SHELL_CommandTypeDef* findCommand(char *str)
{

}




static void do_menu(CommandParaTypeDef para);

const SHELL_CommandTypeDef ShellCommandList[] =
{
    {(char *)"menu",do_menu,(char *) "show the menu" },
};