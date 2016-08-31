#include "uartbuffer.h"
#include <stdlib.h>
#include <string.h>

/*
  implement a simple ringbuffer of bytes
 */

UARTBuffer::UARTBuffer(uint32_t _size)
{
    size = _size;
    buf = new uint8_t[size];
    allocated = true;
    head = tail = 0;
}

UARTBuffer::UARTBuffer(uint8_t *_buf, uint32_t _size)
{
    size = _size;
    buf = _buf;
    head = tail = 0;
}

UARTBuffer::~UARTBuffer(void)
{
    if (allocated) {
        delete [] buf;
    }
}

uint32_t UARTBuffer::available(void) const
{
    uint32_t _tail;
    return ((head > (_tail=tail))? (size - head) + _tail: _tail - head);
}

uint32_t UARTBuffer::space(void) const
{
    uint32_t _head;
    return (((_head=head) > tail)?(_head - tail) - 1:((size - tail) + _head) - 1);
}

bool UARTBuffer::empty(void) const
{
    return head == tail;
}

uint32_t UARTBuffer::write(const uint8_t *data, uint32_t len)
{
    if (len > space()) {
        len = space();
    }
    if (len == 0) {
        return 0;
    }
    if (tail+len <= size) {
        // perform as single memcpy
        memcpy(&buf[tail], data, len);
        tail = (tail + len) % size;
        return len;
    }

    // perform as two memcpy calls
    uint32_t n = size - tail;
    if (n > len) {
        n = len;
    }
    memcpy(&buf[tail], data, n);
    tail = (tail + n) % size;
    data += n;
    n = len - n;
    if (n > 0) {
        memcpy(&buf[tail], data, n);
        tail = (tail + n) % size;
    }
    return len;
}

uint32_t UARTBuffer::read(uint8_t *data, uint32_t len)
{
    if (len > available()) {
        len = available();
    }
    if (len == 0) {
        return 0;
    }
    if (head+len <= size) {
        // perform as single memcpy
        memcpy(data, &buf[head], len);
        head = (head + len) % size;
        return len;
    }

    // perform as two memcpy calls
    uint32_t n = size - head;
    if (n > len) {
        n = len;
    }
    memcpy(data, &buf[head], n);
    head = (head + n) % size;
    data += n;
    n = len - n;
    if (n > 0) {
        memcpy(data, &buf[head], n);
        head = (head + n) % size;
    }
    return len;
}
