#include <stdint.h>
#include <stdbool.h>

class UARTBuffer {
public:
    UARTBuffer(uint32_t size);
    // optionally provide the memory (for shared memory)
    UARTBuffer(uint8_t *_buf, uint32_t size);
    ~UARTBuffer(void);
    
    uint32_t available(void) const;
    uint32_t space(void) const;
    bool empty(void) const;
    uint32_t write(const uint8_t *data, uint32_t len);
    uint32_t read(uint8_t *data, uint32_t len);
    
private:
    uint8_t *buf = nullptr;
    uint32_t size = 0;

    // head is where the next available data is. tail is where new
    // data is written
    volatile uint32_t head = 0;
    volatile uint32_t tail = 0;
    bool allocated = false;
};

/*
  ring buffer class for objects of fixed size
 */
template <class T>
class ObjectBuffer {
public:
    ObjectBuffer(uint32_t _size) {
        size = _size;
        buffer = new UARTBuffer(size * sizeof(T));
    }
    // optionally provide the memory
    ObjectBuffer(T *ptr, uint32_t _size) {
        size = _size;
        buffer = new UARTBuffer((uint8_t *)ptr, size * sizeof(T));
    }
    ~ObjectBuffer(void) {
        delete buffer;
    }

    uint32_t available(void) const {
        return buffer->available() / sizeof(T);
    }
    uint32_t space(void) const {
        return buffer->space() / sizeof(T);
    }
    bool empty(void) const {
        return buffer->empty();
    }
    bool push(const T &object) {
        if (buffer->space() < sizeof(T)) {
            return false;
        }
        return buffer->write((uint8_t*)&object, sizeof(T)) == sizeof(T);
    }
    bool pop(T &object) {
        if (buffer->available() < sizeof(T)) {
            return false;
        }
        return buffer->read((uint8_t*)&object, sizeof(T)) == sizeof(T);
    }

private:
    UARTBuffer *buffer = nullptr;
    uint32_t size = 0;
};
    
