#include "parrot/core/binary.h"
#include "parrot/core/array.h"

struct ParrotBuffer {
    ParrotBufferRead read;
    ParrotBufferWrite write;
    ParrotScope *scope;

    size_t bytes_written;
    size_t read_position;
};

ParrotBuffer *ParrotBuffer_new(ParrotScope *scope, ParrotBufferRead read, ParrotBufferWrite write) {
    ParrotBuffer *self = PARROT_ALLOC(ParrotBuffer);

    self->read = read;
    self->write = write;
    self->scope = scope;

    return self;
}

static bool file_read(ParrotScope *scope, size_t position, uint8_t *out) {
    FILE *file = ParrotScope_get_ctx(scope, FILE *);

    size_t old_position = ftell(file);
    fseek(file, position, SEEK_SET);
    bool success = fread(out, sizeof(*out), 1, file) == sizeof(*out);
    fseek(file, old_position, SEEK_SET);

    return success;
}

static void file_write(ParrotScope *scope, uint8_t byte) {
    FILE *file = ParrotScope_get_ctx(scope, FILE *);
    fwrite(&byte, sizeof(byte), 1, file);
}

ParrotBuffer *ParrotBuffer_new_file(FILE *file) {
    PARROT_FAIL_NULL(file);

    ParrotScope *scope = ParrotScope_new(NULL);
    ParrotScope_set_ctx(scope, file);

    return ParrotBuffer_new(scope, file_read, file_write);
}

typedef struct {
    const uint8_t **p_data;
    size_t size;

    ParrotScope *write_scope;
    ParrotBufferWrite write;
} BytearrayCtx;

static bool bytearray_read(ParrotScope *scope, size_t position, uint8_t *out) {
    BytearrayCtx *ctx = ParrotScope_get_ctx(scope, BytearrayCtx *);

    if (position >= ctx->size) {
        return false;
    }

    *out = (*ctx->p_data)[position];
    return true;
}

static void bytearray_write_wrapper(ParrotScope *scope, uint8_t byte) {
    BytearrayCtx *ctx = ParrotScope_get_ctx(scope, BytearrayCtx *);

    if (ctx->write) {
        ctx->write(ctx->write_scope, byte);
    }
}

ParrotBuffer *
ParrotBuffer_new_bytearray(ParrotScope *write_scope, const void **p_data, size_t size, ParrotBufferWrite write) {
    PARROT_FAIL_NULL(p_data);

    ParrotScope *scope = ParrotScope_new(NULL);
    BytearrayCtx *ctx = ParrotScope_alloc_ctx(scope, BytearrayCtx);

    ctx->p_data = (const uint8_t **)p_data;
    ctx->size = size;

    ctx->write_scope = write_scope;
    ctx->write = write;

    if (ctx->write_scope) {
        ParrotScope_set_parent(write_scope, scope);
    }

    return ParrotBuffer_new(scope, bytearray_read, bytearray_write_wrapper);
}

static bool stbds_array_read(ParrotScope *scope, size_t position, uint8_t *out) {
    uint8_t **p_arr_data = ParrotScope_get_ctx(scope, uint8_t **);

    if (position >= ParrotArray_size(*p_arr_data)) {
        return false;
    }

    *out = (*p_arr_data)[position];
    return true;
}

static void stbds_array_write(ParrotScope *scope, uint8_t byte) {
    uint8_t **p_arr_data = ParrotScope_get_ctx(scope, uint8_t **);
    ParrotArray_push(*p_arr_data, byte);
}

ParrotBuffer *ParrotBuffer_new_array_raw(uint8_t **p_arr_data) {
    ParrotScope *scope = ParrotScope_new(NULL);
    ParrotScope_set_ctx(scope, p_arr_data);
    return ParrotBuffer_new(scope, stbds_array_read, stbds_array_write);
}

void ParrotBuffer_delete(ParrotBuffer *self) {
    PARROT_FAIL_NULL(self);

    if (self->scope) {
        ParrotScope_delete(self->scope);
    }

    free(self);
}

void ParrotBuffer_vdelete(void *self) {
    ParrotBuffer_delete((ParrotBuffer *)self);
}

void ParrotBuffer_rseek(ParrotBuffer *self, size_t position) {
    PARROT_FAIL_NULL(self);

    self->read_position = position;
}

size_t ParrotBuffer_rtell(ParrotBuffer *self) {
    PARROT_FAIL_NULL(self);

    return self->read_position;
}

void ParrotBuffer_pad(ParrotBuffer *self, uint8_t data, size_t count) {
    PARROT_FAIL_NULL(self);

    for (size_t i = 0; i < count; i++) {
        ParrotBuffer_write8(self, data);
    }
}

void ParrotBuffer_pad_until(ParrotBuffer *self, uint8_t data, size_t until_position) {
    PARROT_FAIL_NULL(self);

    while (until_position >= self->bytes_written) {
        ParrotBuffer_write8(self, data);
    }
}

size_t ParrotBuffer_read(ParrotBuffer *self, ParrotBufferEndian endian, void *out_ptr, size_t size) {
    uint8_t *out = out_ptr;

    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    PARROT_RET_COND_V(!self->read, 0);

    for (size_t i = 0; i < size; i++) {
        if (!self->read(self->scope, self->read_position, &out[i])) {
            return i;
        }
        self->read_position++;
    }

    if (endian != ParrotBufferEndian_HOST) {
        uint16_t endian_test_big = 1;
        uint8_t endian_test = 0;
        memcpy(&endian_test, &endian_test_big, sizeof(uint8_t));

        ParrotBufferEndian host_endian = endian_test == 1 ? ParrotBufferEndian_LITTLE : ParrotBufferEndian_BIG;
        if (endian != host_endian) {
            for (size_t i = 0; i < size / 2; i++) {
                uint8_t tmp = out[i];
                out[i] = out[size - 1 - i];
                out[size - 1 - i] = tmp;
            }
        }
    }

    return size;
}

void ParrotBuffer_write(ParrotBuffer *self, ParrotBufferEndian endian, const void *data_ptr, size_t size) {
    const uint8_t *data = data_ptr;

    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(data);

    PARROT_RET_COND(!self->write);

    uint16_t endian_test_big = 1;
    uint8_t endian_test = 0;
    memcpy(&endian_test, &endian_test_big, sizeof(uint8_t));

    ParrotBufferEndian host_endian = endian_test == 1 ? ParrotBufferEndian_LITTLE : ParrotBufferEndian_BIG;
    if (endian != ParrotBufferEndian_HOST && endian != host_endian) {
        for (size_t i = size; i > 0; i--) {
            self->write(self->scope, data[i - 1]);
            self->bytes_written++;
        }
    } else {
        for (size_t i = 0; i < size; i++) {
            self->write(self->scope, data[i]);
            self->bytes_written++;
        }
    }
}

bool ParrotBuffer_read8(ParrotBuffer *self, uint8_t *out) {
    return ParrotBuffer_read(self, ParrotBufferEndian_HOST, out, sizeof(*out));
}

bool ParrotBuffer_read16(ParrotBuffer *self, ParrotBufferEndian endian, uint16_t *out) {
    return ParrotBuffer_read(self, endian, out, sizeof(*out));
}

bool ParrotBuffer_read32(ParrotBuffer *self, ParrotBufferEndian endian, uint32_t *out) {
    return ParrotBuffer_read(self, endian, out, sizeof(*out));
}

bool ParrotBuffer_read64(ParrotBuffer *self, ParrotBufferEndian endian, uint64_t *out) {
    return ParrotBuffer_read(self, endian, out, sizeof(*out));
}

void ParrotBuffer_write8(ParrotBuffer *self, uint8_t data) {
    ParrotBuffer_write(self, ParrotBufferEndian_HOST, &data, sizeof(data));
}

void ParrotBuffer_write16(ParrotBuffer *self, ParrotBufferEndian endian, uint16_t data) {
    ParrotBuffer_write(self, endian, &data, sizeof(data));
}

void ParrotBuffer_write32(ParrotBuffer *self, ParrotBufferEndian endian, uint32_t data) {
    ParrotBuffer_write(self, endian, &data, sizeof(data));
}

void ParrotBuffer_write64(ParrotBuffer *self, ParrotBufferEndian endian, uint64_t data) {
    ParrotBuffer_write(self, endian, &data, sizeof(data));
}

bool ParrotBuffer_read8s(ParrotBuffer *self, int8_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint8_t value;

    if (!ParrotBuffer_read8(self, &value)) {
        return false;
    }

    if (value <= INT8_MAX) {
        *out = (int8_t)value;
    } else {
        *out = -(int8_t)(UINT8_MAX - value) - 1;
    }

    return true;
}

bool ParrotBuffer_read16s(ParrotBuffer *self, ParrotBufferEndian endian, int16_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint16_t value;

    if (!ParrotBuffer_read16(self, endian, &value)) {
        return false;
    }

    if (value <= INT16_MAX) {
        *out = (int16_t)value;
    } else {
        *out = -(int16_t)(UINT16_MAX - value) - 1;
    }

    return true;
}

bool ParrotBuffer_read32s(ParrotBuffer *self, ParrotBufferEndian endian, int32_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint32_t value;

    if (!ParrotBuffer_read32(self, endian, &value)) {
        return false;
    }

    if (value <= INT32_MAX) {
        *out = (int32_t)value;
    } else {
        *out = -(int32_t)(UINT32_MAX - value) - 1;
    }

    return true;
}

bool ParrotBuffer_read64s(ParrotBuffer *self, ParrotBufferEndian endian, int64_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint64_t value;

    if (!ParrotBuffer_read64(self, endian, &value)) {
        return false;
    }

    if (value <= INT64_MAX) {
        *out = (int64_t)value;
    } else {
        *out = -(int64_t)(UINT64_MAX - value) - 1;
    }

    return true;
}

void ParrotBuffer_write8s(ParrotBuffer *self, int8_t data) {
    PARROT_FAIL_NULL(self);

    uint8_t value;

    if (data < 0) {
        value = (uint8_t)(UINT8_MAX - (uint8_t)(-(data + 1)));
    } else {
        value = (uint8_t)data;
    }

    ParrotBuffer_write8(self, value);
}

void ParrotBuffer_write16s(ParrotBuffer *self, ParrotBufferEndian endian, int16_t data) {
    PARROT_FAIL_NULL(self);

    uint16_t value;

    if (data < 0) {
        value = (uint16_t)(UINT16_MAX - (uint16_t)(-(data + 1)));
    } else {
        value = (uint16_t)data;
    }

    ParrotBuffer_write16(self, endian, value);
}

void ParrotBuffer_write32s(ParrotBuffer *self, ParrotBufferEndian endian, int32_t data) {
    PARROT_FAIL_NULL(self);

    uint32_t value;

    if (data < 0) {
        value = (uint32_t)(UINT32_MAX - (uint32_t)(-(data + 1)));
    } else {
        value = (uint32_t)data;
    }

    ParrotBuffer_write32(self, endian, value);
}

void ParrotBuffer_write64s(ParrotBuffer *self, ParrotBufferEndian endian, int64_t data) {
    PARROT_FAIL_NULL(self);

    uint64_t value;

    if (data < 0) {
        value = (uint64_t)(UINT64_MAX - (uint64_t)(-(data + 1)));
    } else {
        value = (uint64_t)data;
    }

    ParrotBuffer_write64(self, endian, value);
}

void ParrotBuffer_write_ascii(ParrotBuffer *self, const char *str) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(str);

    while (*str) {
        ParrotBuffer_write8(self, *str++);
    }
    ParrotBuffer_write8(self, 0);
}

uint8_t Parrot_char_to_ascii(char c) {
    switch (c) {
    case '\0':
        return 0x00;
    case '0':
        return 0x30;
    case '1':
        return 0x31;
    case '2':
        return 0x32;
    case '3':
        return 0x33;
    case '4':
        return 0x34;
    case '5':
        return 0x35;
    case '6':
        return 0x36;
    case '7':
        return 0x37;
    case '8':
        return 0x38;
    case '9':
        return 0x39;
    case 'A':
        return 0x41;
    case 'B':
        return 0x42;
    case 'C':
        return 0x43;
    case 'D':
        return 0x44;
    case 'E':
        return 0x45;
    case 'F':
        return 0x46;
    case 'G':
        return 0x47;
    case 'H':
        return 0x48;
    case 'I':
        return 0x49;
    case 'J':
        return 0x4A;
    case 'K':
        return 0x4B;
    case 'L':
        return 0x4C;
    case 'M':
        return 0x4D;
    case 'N':
        return 0x4E;
    case 'O':
        return 0x4F;
    case 'P':
        return 0x50;
    case 'Q':
        return 0x51;
    case 'R':
        return 0x52;
    case 'S':
        return 0x53;
    case 'T':
        return 0x54;
    case 'U':
        return 0x55;
    case 'V':
        return 0x56;
    case 'W':
        return 0x57;
    case 'X':
        return 0x58;
    case 'Y':
        return 0x59;
    case 'Z':
        return 0x5A;
    case 'a':
        return 0x61;
    case 'b':
        return 0x62;
    case 'c':
        return 0x63;
    case 'd':
        return 0x64;
    case 'e':
        return 0x65;
    case 'f':
        return 0x66;
    case 'g':
        return 0x67;
    case 'h':
        return 0x68;
    case 'i':
        return 0x69;
    case 'j':
        return 0x6A;
    case 'k':
        return 0x6B;
    case 'l':
        return 0x6C;
    case 'm':
        return 0x6D;
    case 'n':
        return 0x6E;
    case 'o':
        return 0x6F;
    case 'p':
        return 0x70;
    case 'q':
        return 0x71;
    case 'r':
        return 0x72;
    case 's':
        return 0x73;
    case 't':
        return 0x74;
    case 'u':
        return 0x75;
    case 'v':
        return 0x76;
    case 'w':
        return 0x77;
    case 'x':
        return 0x78;
    case 'y':
        return 0x79;
    case 'z':
        return 0x7A;
    case '!':
        return 0x21;
    case '"':
        return 0x22;
    case '#':
        return 0x23;
    case '%':
        return 0x25;
    case '&':
        return 0x26;
    case '\'':
        return 0x27;
    case '(':
        return 0x28;
    case ')':
        return 0x29;
    case '*':
        return 0x2A;
    case '+':
        return 0x2B;
    case ',':
        return 0x2C;
    case '-':
        return 0x2D;
    case '.':
        return 0x2E;
    case '/':
        return 0x2F;
    case ':':
        return 0x3A;
    case ';':
        return 0x3B;
    case '<':
        return 0x3C;
    case '=':
        return 0x3D;
    case '>':
        return 0x3E;
    default:
    case '?':
        return 0x3F;
    case '[':
        return 0x5B;
    case '\\':
        return 0x5C;
    case ']':
        return 0x5D;
    case '^':
        return 0x5E;
    case '_':
        return 0x5F;
    case '{':
        return 0x7B;
    case '|':
        return 0x7C;
    case '}':
        return 0x7D;
    case '~':
        return 0x7E;
    }
}

char Parrot_ascii_to_char(uint8_t ascii) {
    switch (ascii) {
    case 0x00:
        return '\0';
    case 0x30:
        return '0';
    case 0x31:
        return '1';
    case 0x32:
        return '2';
    case 0x33:
        return '3';
    case 0x34:
        return '4';
    case 0x35:
        return '5';
    case 0x36:
        return '6';
    case 0x37:
        return '7';
    case 0x38:
        return '8';
    case 0x39:
        return '9';
    case 0x41:
        return 'A';
    case 0x42:
        return 'B';
    case 0x43:
        return 'C';
    case 0x44:
        return 'D';
    case 0x45:
        return 'E';
    case 0x46:
        return 'F';
    case 0x47:
        return 'G';
    case 0x48:
        return 'H';
    case 0x49:
        return 'I';
    case 0x4A:
        return 'J';
    case 0x4B:
        return 'K';
    case 0x4C:
        return 'L';
    case 0x4D:
        return 'M';
    case 0x4E:
        return 'N';
    case 0x4F:
        return 'O';
    case 0x50:
        return 'P';
    case 0x51:
        return 'Q';
    case 0x52:
        return 'R';
    case 0x53:
        return 'S';
    case 0x54:
        return 'T';
    case 0x55:
        return 'U';
    case 0x56:
        return 'V';
    case 0x57:
        return 'W';
    case 0x58:
        return 'X';
    case 0x59:
        return 'Y';
    case 0x5A:
        return 'Z';
    case 0x61:
        return 'a';
    case 0x62:
        return 'b';
    case 0x63:
        return 'c';
    case 0x64:
        return 'd';
    case 0x65:
        return 'e';
    case 0x66:
        return 'f';
    case 0x67:
        return 'g';
    case 0x68:
        return 'h';
    case 0x69:
        return 'i';
    case 0x6A:
        return 'j';
    case 0x6B:
        return 'k';
    case 0x6C:
        return 'l';
    case 0x6D:
        return 'm';
    case 0x6E:
        return 'n';
    case 0x6F:
        return 'o';
    case 0x70:
        return 'p';
    case 0x71:
        return 'q';
    case 0x72:
        return 'r';
    case 0x73:
        return 's';
    case 0x74:
        return 't';
    case 0x75:
        return 'u';
    case 0x76:
        return 'v';
    case 0x77:
        return 'w';
    case 0x78:
        return 'x';
    case 0x79:
        return 'y';
    case 0x7A:
        return 'z';
    case 0x21:
        return '!';
    case 0x22:
        return '"';
    case 0x23:
        return '#';
    case 0x25:
        return '%';
    case 0x26:
        return '&';
    case 0x27:
        return '\'';
    case 0x28:
        return '(';
    case 0x29:
        return ')';
    case 0x2A:
        return '*';
    case 0x2B:
        return '+';
    case 0x2C:
        return ',';
    case 0x2D:
        return '-';
    case 0x2E:
        return '.';
    case 0x2F:
        return '/';
    case 0x3A:
        return ':';
    case 0x3B:
        return ';';
    case 0x3C:
        return '<';
    case 0x3D:
        return '=';
    case 0x3E:
        return '>';
    default:
    case 0x3F:
        return '?';
    case 0x5B:
        return '[';
    case 0x5C:
        return '\\';
    case 0x5D:
        return ']';
    case 0x5E:
        return '^';
    case 0x5F:
        return '_';
    case 0x7B:
        return '{';
    case 0x7C:
        return '|';
    case 0x7D:
        return '}';
    case 0x7E:
        return '~';
    }
}
