#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool cmt_util_debug_enabled(void) {
    static bool checked = false;
    static bool enabled = false;

    if (!checked) {
        enabled = getenv("CMT_DEBUG") != NULL;
        checked = true;
    }

    return enabled;
}
