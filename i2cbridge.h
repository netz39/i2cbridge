/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __I2CBRIDGE_H__
#define __I2CBRIDGE_H__

#include <stdint.h>

#define I2CBRIDGE_NAME  "i2cbridge"
#define I2CBRIDGE_PORT  3939
#define I2CBRIDGE_PWD   "/var/lib/" I2CBRIDGE_NAME
#define I2CBRIDGE_UNIX  "unix"

#define I2CBRIDGE_CMD_READ8   0
#define I2CBRIDGE_CMD_READ16  1
#define I2CBRIDGE_CMD_WRITE8  2
#define I2CBRIDGE_CMD_WRITE16 3

#define I2CBRIDGE_ERROR_OK       0
#define I2CBRIDGE_ERROR_INTERNAL 1
#define I2CBRIDGE_ERROR_COMMAND  2
#define I2CBRIDGE_ERROR_ADDRESS  3
#define I2CBRIDGE_ERROR_I2C      4

struct i2cbridge_request
{
    uint8_t cmd;
    uint8_t addr;
    uint8_t reg;
    uint16_t data;
} __attribute__((packed));

struct i2cbridge_response
{
    uint8_t status;
    uint16_t data;
} __attribute__((packed));

#endif
