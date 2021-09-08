#include <stdio.h>
#include <string.h>

#define member_size(type, member) sizeof(((type *)0)->member)

typedef struct Person
{
    char name[20];
    char role[20];
    int age;
} Person;

typedef struct Jedi
{
    char rank[20];
    short form;
} Jedi;

main(int argc, char *argv[])
{
    Person luke;

    strncpy(luke.name, "Luke Skywalker", member_size(Person, name));
    strncpy(luke.role, "Jedi Master", member_size(Person, name));
    luke.age = 40;

    printf("%s is a %s age %d\n", luke.name, luke.role, luke.age);
}