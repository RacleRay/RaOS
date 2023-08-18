#ifndef _DISK_H
#define _DISK_H

int disk_read_sector(int lba, int total, void* buf);

#endif