#ifndef CMT_UTIL_H
#define CMT_UTIL_H
#include <stdbool.h>

/**
 */
bool cmt_util_debug_enabled(void);

/** Read whole file `name` contents into `data` and set `length`.
 * @param name[in]    - file path
 * @param max[in]     - size of `data` in bytes
 * @param data[out]   - file contents
 * @param length[out] - actual size in `bytes` written to `data`
 *
 * @return
 * |     |                    |
 * |-----|--------------------|
 * |   0 |success             |
 * | < 0 |negative errno value| */
int cmt_util_read_whole_file(const char *name, size_t max, void *data, size_t *length);

/** Write the contents of `data` into file `name`.
 * @param name[in]    - file path
 * @param length[in]  - size of `data` in bytes
 * @param data[out]   - file contents
 *
 * @return
 * |     |                    |
 * |-----|--------------------|
 * |   0 |success             |
 * | < 0 |negative errno value| */
int cmt_util_write_whole_file(const char *name, size_t length, const void *data);

#endif /* CMT_UTIL_H */
