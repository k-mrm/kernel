#include <types.h>
#include <compiler.h>
#include <module.h>
#include <printk.h>

void
testmodule (void)
{
        printk ("Hello\n");
}

MODULE_DECL test = {
        .name           = "Test",
        .description    = "just testtest",
        .init           = testmodule,
        .delete         = NULL,
};
