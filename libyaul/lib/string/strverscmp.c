/*-
 * Copyright (c) 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <ctype.h>
#include <string.h>

int
strverscmp(const char *l0, const char *r0)
{
        const uint8_t *l = (const void *)l0;
        const uint8_t *r = (const void *)r0;
        size_t i, dp, j;
        int z = 1;

        /* Find maximal matching prefix and track its maximal digit suffix and
         * whether those digits are all zeros. */
        for (dp = i = 0; l[i] == r[i]; i++) {
                int c = l[i];

                if (!c) {
                        return 0;
                }

                if (!isdigit(c)) {
                        dp = i + 1, z = 1;
                } else if (c != '0') {
                        z = 0;
                }
        }

        if (l[dp] != '0' && r[dp] != '0') {
                /* If we're not looking at a digit sequence that began with a
                 * zero, longest digit string is greater. */
                for (j = i; isdigit(l[j]); j++) {
                        if (!isdigit(r[j])) {
                                return 1;
                        }
                }

                if (isdigit(r[j])) {
                        return -1;
                }
        } else if (z && dp < i && (isdigit(l[i]) || isdigit(r[i]))) {
                /* Otherwise, if common prefix of digit sequence is all zeros,
                 * digits order less than non-digits. */
                return (uint8_t)(l[i] - '0') - (uint8_t)(r[i] - '0');
        }

        return l[i] - r[i];
}
