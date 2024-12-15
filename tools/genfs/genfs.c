#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#define NAME_MAX 64
#define BRUXFS_INDEX 6

#pragma packed(1, push)

FILE * _img = NULL;
uint16_t _img_size = 0;
FILE * _bootloader = NULL;

struct bruxfs_date
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t centis;
} __attribute__((__packed__));

struct bruxfs_item
{
    char name[NAME_MAX];
    uint16_t zero;
    uint16_t user_id;
    uint16_t group_id;
    uint16_t creation_user_id;
    struct bruxfs_date creation_date;
    struct bruxfs_date modification_date;
    uint8_t reserved[500-NAME_MAX-8-8-8];
} __attribute__((__packed__));

struct bruxfs_block
{
    union
    {
        uint8_t raw[500];
        struct bruxfs_item item;
    };
    uint16_t id;
    uint16_t mode;
    uint16_t next;
    uint16_t child;
    uint16_t parent;
    uint16_t size;
} __attribute__((__packed__));

void block_write(struct bruxfs_block * block)
{
    fseek(_img, (int)block->id * 512, SEEK_SET);
    fwrite(block, 512, 1, _img);
    fflush(_img);
    printf("[W %d %o NEXT %d]\n", block->id, block->mode, block->next);
}

void block_read(struct bruxfs_block * block)
{
    fseek(_img, (int)block->id * 512, SEEK_SET);
    fread(block, 512, 1, _img);
    printf("[R %d %o NEXT %d]\n", block->id, block->mode, block->next);
}

void currtime(struct bruxfs_date * date)
{
    time_t t = time(NULL);
    struct tm * curr = localtime(&t);
    date->centis = 0;
    date->day = curr->tm_mday;
    date->hour = curr->tm_hour;
    date->minute = curr->tm_min;
    date->month = curr->tm_mon;
    date->second = curr->tm_sec;
    date->year = curr->tm_year + 1900;
}

void inject_bootloader()
{
    uint8_t buffer[BRUXFS_INDEX * 512];
    memset(buffer, 0, BRUXFS_INDEX * 512);
    fread(buffer, 1, BRUXFS_INDEX * 512, _bootloader);
    rewind(_img);
    fwrite(buffer, 1, BRUXFS_INDEX * 512, _img);
    fflush(_img);
}

void block_init(struct bruxfs_block * block, uint16_t id, uint16_t mode)
{
    memset(block, 0, 512);
    block->id = id;
    block->mode = mode;
}

void mkfs()
{
    struct bruxfs_block block;
    block_init(&block, BRUXFS_INDEX, __S_IFDIR | S_IRWXG | S_IRWXO | S_IRWXU);
    block.size = _img_size;
    block.next = block.id + 1;
    currtime(&block.item.creation_date);
    currtime(&block.item.modification_date);
    block_write(&block);
    block_init(&block, block.next, 0);
    block_write(&block);
}

void block_alloc(struct bruxfs_block * block)
{
    struct bruxfs_block index;
    struct bruxfs_block find;
    index.id = BRUXFS_INDEX;
    printf("ALLOC START\n");
    block_read(&index);
    int i = 0;
    find.id = index.id;
    find.next = index.next;
    do
    {
        printf("<%d %d>", find.id, find.next);
        index.id = find.id;
        find.id = find.next;
        block_read(&find);
        i++;
    } while(find.next);
    block_read(&index);
    printf("<%d OK>", find.id);
    block->id = find.id;
    if(i == 1)
        index.next++;
    else
        index.next = 0;
    block_write(&index);
    block_write(block);
    printf("ALLOC END\n");
}

void add_file(char * unx_fname, char * img_fname)
{
    struct bruxfs_block dir;
    struct bruxfs_block file;
    struct bruxfs_block data;
    char part[NAME_MAX + 1];
    char part_i = 0;
    char *ptr = img_fname;
    strcpy(part, "");
    dir.id = BRUXFS_INDEX;
    block_read(&dir);
    do
    {
        if(*ptr == '/' || *ptr == 0)
        {
            if(*ptr != 0)
            {
                ptr++;
                if(*ptr == 0)
                {
                    fprintf(stderr, "error: invalid file address: %s\n", img_fname);
                    exit(-EINVAL);
                }
            }
            part_i = 0;
            if(strlen(part))
            {
                if(!(dir.mode & __S_IFDIR))
                {
                    fprintf(stderr, "error: invalid file path: %s [%s != %s %o]\n", img_fname, dir.item.name, part, dir.mode);
                    exit(-EINVAL);
                }
                if(dir.child == 0)
                {
                    printf("---- CHILD 0\n");
                    if(*ptr) // Directory
                        block_init(&file, 0, __S_IFDIR | S_IRWXG | S_IRWXO | S_IRWXG);
                    else // File
                        block_init(&file, 0, __S_IFREG | S_IRWXG | S_IRWXO | S_IRWXG);
                    strcpy(file.item.name, part);
                    currtime(&file.item.creation_date);
                    currtime(&file.item.modification_date);
                    currtime(&dir.item.modification_date);
                    file.parent = dir.id;
                    block_alloc(&file);
                    block_read(&dir);
                    dir.child = file.id;
                    block_write(&dir);
                    memcpy(&dir, &file, sizeof(struct bruxfs_block));
                }
                else
                {
                    bool found = false;
                    file.next = dir.child;
                    do
                    {
                        file.id = file.next;
                        block_read(&file);
                        if(!strcmp(part, file.item.name))
                        {
                            found = true;
                            break;
                        }
                    } while(file.next);
                    if(!found)
                    {
                    printf("---- NOT FOUND\n");
                        dir.id = file.id;
                        block_read(&dir);
                        if(*ptr) // Directory
                            block_init(&file, 0, __S_IFDIR | S_IRWXG | S_IRWXO | S_IRWXG);
                        else // File
                            block_init(&file, 0, __S_IFREG | S_IRWXG | S_IRWXO | S_IRWXG);
                        strcpy(file.item.name, part);
                        currtime(&file.item.creation_date);
                        currtime(&file.item.modification_date);
                        currtime(&dir.item.modification_date);
                        file.parent = dir.parent;
                        block_alloc(&file);
                        block_read(&dir);
                        dir.next = file.id;
                        block_write(&dir);
                    }
                    memcpy(&dir, &file, sizeof(struct bruxfs_block));
                }
            }
        }
        else
        {
            part[part_i++] = *ptr++;
            part[part_i] = 0;
        }
    } while(*ptr);
    FILE * unx = fopen(unx_fname, "rb");
    if(!unx)
    {
        fprintf(stderr, "error: source file not found: %s\n", unx_fname);
        exit(-EINVAL);
    }
    //dir = parent file
    //file = prev block;
    //data = curr block;
    file.id = 0;
    do
    {
        block_init(&data, 0, 0);
        data.size = fread(data.raw, 1, 500, unx);
        data.parent = dir.id;
        block_alloc(&data);
        if(file.id)
        {
            file.next = data.id;
            block_write(&file);
        }
        else
        {
            dir.child = data.id;
            block_write(&dir);
        }
        memcpy(&file, &data, sizeof(struct bruxfs_block));
    } while(data.size == 500);
}

int main(int argc, char **argv)
{
    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-i"))
        {
            i++;
            if(i < argc) _img = fopen(argv[i], "rb+");
        }
        else if(!strcmp(argv[i], "-b"))
        {
            i++;
            if(i < argc) _bootloader = fopen(argv[i], "rb");
        }
        else if(!strcmp(argv[i], "-s"))
        {
            i++;
            if(i < argc) _img_size = atoi(argv[i]);
        }
    }
    if(_img == NULL || _img_size == 0)
    {
        fprintf(stderr, "error: invalid image name or size.\n");
        return -EINVAL;
    }

    if(_bootloader) inject_bootloader();
    mkfs();
    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-a"))
        {
            i+=2;
            if(i < argc) add_file(argv[i-1], argv[i]);
        }
    }

    fclose(_img);
    return 0;
}

#pragma packed(pop)