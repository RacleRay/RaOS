#include "streamer.h"
#include "../config.h"
#include "../memory/heap/kheap.h"



struct disk_stream* diskstreamer_new(int disk_id) {
    struct disk* disk = disk_get(disk_id);
    if (!disk) {
        return NULL;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos                = 0;
    streamer->disk               = disk;

    return streamer;
}


int diskstreamer_seek(struct disk_stream* stream, int pos) {
    stream->pos = pos;
    return 0;
}

/**
 * @brief Read to total bytes to out from disk stream.
 *        NOTE: there is a problem, the recursion may cause stack overflow.
 *
 * @param stream
 * @param out
 * @param total
 * @return int
 */
int diskstreamer_read(struct disk_stream* stream, void* out, int total) {
    int sector = stream->pos / RAOS_SECTOR_SIZE;
    int offset = stream->pos % RAOS_SECTOR_SIZE;

    char buf[RAOS_SECTOR_SIZE];
    int  res = disk_read_block(stream->disk, sector, 1, buf);
    if (res < 0) {
        goto out;
    }

    // read for small size data
    int total_to_read = total > RAOS_SECTOR_SIZE ? RAOS_SECTOR_SIZE : total;
    for (int i = 0; i < total_to_read; ++i) {
        *(char*)out++ = buf[offset + i];
    }

    // read for big size data
    stream->pos += total_to_read;
    if (total > RAOS_SECTOR_SIZE) {
        res = diskstreamer_read(stream, out, total - RAOS_SECTOR_SIZE);
    }
out:
    return res;
}


void diskstreamer_close(struct disk_stream* stream) { kfree(stream); }