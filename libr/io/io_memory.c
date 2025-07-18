/* radare - LGPL - Copyright 2008-2022 - pancake */

#include "io_memory.h"

static inline ut32 _io_malloc_sz(RIODesc *desc) {
	R_RETURN_VAL_IF_FAIL (desc, 0);
	RIOMalloc *mal = (RIOMalloc*)desc->data;
	return mal? mal->size: 0;
}

static inline ut8* _io_malloc_buf(RIODesc *desc) {
	R_RETURN_VAL_IF_FAIL (desc, NULL);
	RIOMalloc *mal = (RIOMalloc*)desc->data;
	return mal->buf;
}

#if 0
static inline ut8* _io_malloc_set_buf(RIODesc *desc, ut8* buf) {
	if (!desc) {
		return NULL;
	}
	RIOMalloc *mal = (RIOMalloc*)desc->data;
	return mal->buf = buf;
}
#endif

static inline ut64 _io_malloc_off(RIODesc *desc) {
	R_RETURN_VAL_IF_FAIL (desc, 0);
	RIOMalloc *mal = (RIOMalloc*)desc->data;
	return mal->offset;
}

static inline void _io_malloc_set_off(RIODesc *desc, ut64 off) {
	R_RETURN_IF_FAIL (desc);
	RIOMalloc *mal = (RIOMalloc*)desc->data;
	mal->offset = off;
}

R_IPI int io_memory_write(RIO *io, RIODesc *fd, const ut8 *buf, int count) {
	R_RETURN_VAL_IF_FAIL (io && fd && buf, -1);
	if (count < 0 || !fd->data) {
		return -1;
	}
	ut64 off = _io_malloc_off (fd);
	ut64 msz = _io_malloc_sz (fd);
	if (off > msz) {
		return -1;
	}
	if (off + count > msz) {
		count -= off + count - msz;
	}
	if (count > 0) {
		memcpy (_io_malloc_buf (fd) + off, buf, count);
		_io_malloc_set_off (fd, off + count);
		return count;
	}
	return -1;
}

R_IPI bool io_memory_resize(RIO *io, RIODesc *fd, ut64 count) {
	R_RETURN_VAL_IF_FAIL (io && fd, false);
	if (count == 0) { // TODO: why cant truncate to 0 bytes
		return false;
	}
	ut32 mallocsz = _io_malloc_sz (fd);
	if (_io_malloc_off (fd) > mallocsz) {
		return false;
	}
	RIOMalloc *mal = (RIOMalloc*)fd->data;
	ut8 *new_buf = realloc (mal->buf, count);
	if (!new_buf) {
		return false;
	}
	mal->buf = new_buf;
	if (count > mal->size) {
		memset (mal->buf + mal->size, 0, count - mal->size);
	}
	mal->size = count;
	return true;
}

R_IPI int io_memory_read(RIO *io, RIODesc *fd, ut8 *buf, int count) {
	R_RETURN_VAL_IF_FAIL (io && fd && buf, -1);
	memset (buf, io->Oxff, count);
	if (!fd->data) {
		return -1;
	}
	ut32 mallocsz = _io_malloc_sz (fd);
	ut64 off = _io_malloc_off (fd);
	if (off > mallocsz) {
		return -1;
	}
	int left = count;
	if (off + count > mallocsz) {
		left = mallocsz - off;
	}
	memcpy (buf, _io_malloc_buf (fd) + off, left);
	_io_malloc_set_off (fd, off + left);
	return count;
}

R_IPI bool io_memory_close(RIODesc *fd) {
	if (!fd || !fd->data) {
		return false;
	}
	RIOMalloc *riom = fd->data;
	R_FREE (riom->buf);
	R_FREE (fd->data);
	return true;
}

R_IPI ut64 io_memory_lseek(RIO* io, RIODesc *fd, ut64 offset, int whence) {
	R_RETURN_VAL_IF_FAIL (io && fd, offset);
	if (!fd || !fd->data) {
		return offset;
	}
	ut64 r_offset = offset;
	ut32 mallocsz = _io_malloc_sz (fd);
	switch (whence) {
	case SEEK_SET:
		r_offset = R_MIN (offset, mallocsz);
		break;
	case SEEK_CUR:
		{
			const ut64 pos = _io_malloc_off (fd) + offset;
			r_offset = R_MIN (pos, mallocsz);
		}
		break;
	case SEEK_END:
		r_offset = _io_malloc_sz (fd);
		break;
	}
	_io_malloc_set_off (fd, r_offset);
	return r_offset;
}
