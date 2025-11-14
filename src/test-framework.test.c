#include "test_framework.h"

/* macaroni_tests.c */

#include <string.h>
#include <stdlib.h>
#include "test_framework.h"

typedef struct macaroni_plate_t {
    int pasta;
    int parmigiano;
    char *owner;
} macaroni_plate_t;

/* Dumb implementations just to show the framework */
static macaroni_plate_t *macaroni_plate__allocate(void)
{
    macaroni_plate_t *instance = malloc(sizeof *instance);
    if (!instance) {
        return NULL;
    }
    instance->pasta = 0;
    instance->parmigiano = 0;
    instance->owner = NULL;
    return instance;
}

static int macaroni_plate__clean(macaroni_plate_t *instance)
{
    if (!instance) {
        return -1;
    }
    free(instance->owner);
    instance->owner = NULL;
    instance->pasta = 0;
    instance->parmigiano = 0;
    return 0;
}

static int macaroni_plate__fill_up(
    macaroni_plate_t *instance,
    int pasta,
    int parmigiano,
    const char *owner_name)
{
    if (!instance) {
        return -1;
    }
    free(instance->owner);
    instance->owner = strdup(owner_name);
    instance->pasta = pasta;
    instance->parmigiano = parmigiano;
    return 0;
}

int main(void)
{
    TEST_SUITE_BEGIN(2);

    TEST("macaroni allocation test") {
        macaroni_plate_t *instance = macaroni_plate__allocate();
        ASSERT(instance, "instantiation of macaroni plate");
        /* if you want, free here */
        free(instance);
    }

    TEST("macaroni initialisation test") {
        macaroni_plate_t *instance = macaroni_plate__allocate();
        ASSERT(instance, "instantiation of macaroni plate");

        int cleanup_error = macaroni_plate__clean(instance);
        ASSERT(cleanup_error == 0, "cleanup of macaroni plate");

        ASSERT(instance->pasta == 0, "zero pasta");
        ASSERT(instance->parmigiano == 0, "zero parmigiano");
        ASSERT(instance->owner == NULL, "NULL owner name");

        int init_error = macaroni_plate__fill_up(instance, 11, 22, "Mario");
        ASSERT(init_error == 0, "initialisation of macaroni plate");

        ASSERTF(instance->pasta == 11, "pasta is %d", instance->pasta);
        ASSERTF(instance->parmigiano == 22, "parmigiano is %d", instance->parmigiano);
        ASSERTF(strcmp(instance->owner, "Mario") == 0,
                "owner name is %s", instance->owner);

        macaroni_plate__clean(instance);
        free(instance);
    }

    TEST_SUITE_END();

    return (test_context.total_failure_count == 0) ? 0 : 1;
}


static const char * example =" \n\
=== [1/2] [macaroni allocation test] ========================================== \n\
         - [1.1] [NOK] instantiation of macaroni plate \n\
=== [2/2] [macaroni allocation test] ========================================== \n\
         - [2.1] [ OK] instantiation of macaroni plate \n\
         - [2.2] [ OK] cleanup of macaroni plate \n\
         - [2.3] [ OK] zero pasta \n\
         - [2.4] [ OK] zero parmigiano \n\
         - [2.5] [ OK] NULL owner name \n\
         - [2.6] [ OK] initialisation of macaroni plate \n\
         - [2.7] [ OK] pasta is 8 \n\
         - [2.8] [ OK] parmigiano is 4 \n\
         - [2.9] [ OK] owner name is Giuseppe \n\
";