#include <stdio.h>

/* Function declarations */
void greetMorning();
void greeEvening();
void greetNight();

void greet(void (*greeter)());

int main()
{
    // Pass pointer to greetMorning function
    greet(greetMorning);

    // Pass pointer to greetEvening function
    greet(greeEvening);

    // Pass pointer to greetNight function
    greet(greetNight);

    return 0;
}

/**
 * Function to print greeting message.
 * 
 * @greeter     Function pointer that can point to functions
 *              with no return type and no parameters.
 */
void greet(void (*greeter)())
{
    // Call greeter function pointer
    greeter();
}

void greetMorning()
{
    printf("Good, morning!\n");
}

void greeEvening()
{
    printf("Good, evening!\n");
}

void greetNight()
{
    printf("Good, night!\n");
}