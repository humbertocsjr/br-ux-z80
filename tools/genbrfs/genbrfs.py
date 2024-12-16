import sys
import struct
import datetime
from pathlib import Path

BLOCK_SIZE = 512
DIRECTORY_MODE = 0x41ff
REGULAR_FILE_MODE = 0x81ff
DATA_BLOCK_MODE = 0x0000

class DateTime:
    def __init__(self, year: int, month: int, day: int, hour: int, minute: int, second: int, centisecond: int):
        self.year        = year        # 2 bytes
        self.month       = month       # 1 byte
        self.day         = day         # 1 byte
        self.hour        = hour        # 1 byte
        self.minute      = minute      # 1 byte
        self.second      = second      # 1 byte
        self.centisecond = centisecond # 1 byte

class ItemBlock:
    def __init__(self, item_name: str, always_zero: int, user_id: int, group_id: int, creation_user_id: int, creation_date_time: DateTime, modification_date_time: DateTime, reserved: str, block_id: int, mode: int, next_id: int, first_child_id: int, parent_id: int, data_size: int):
        self.item_name              = item_name                 #  64 bytes
        self.always_zero            = always_zero               #   2 bytes
        self.user_id                = user_id                   #   2 bytes
        self.group_id               = group_id                  #   2 bytes
        self.creation_user_id       = creation_user_id          #   2 bytes
        self.creation_date_time     = creation_date_time        #   8 bytes
        self.modification_date_time = modification_date_time    #   8 bytes
        self.reserved               = reserved                  # 412 bytes
        self.block_id               = block_id                  #   2 bytes
        self.mode                   = mode                      #   2 bytes
        self.next_id                = next_id                   #   2 bytes
        self.first_child_id         = first_child_id            #   2 bytes
        self.parent_id              = parent_id                 #   2 bytes
        self.data_size              = data_size                 #   2 bytes

class DataBlock:
    def __init__(self, raw_data: str, block_id: int, mode, next_id: int, first_child_id: int, parent_id: int, data_size: int):
        self.raw_data       = raw_data       # 500 bytes
        self.block_id       = block_id       #   2 bytes
        self.mode           = mode           #   2 bytes
        self.next_id        = next_id        #   2 bytes
        self.first_child_id = first_child_id #   2 bytes
        self.parent_id      = parent_id      #   2 bytes
        self.data_size      = data_size      #   2 bytes

def get_cur_datetime():
    now = datetime.datetime.now()
    return DateTime(now.year, now.month, now.day, now.hour, now.minute, now.second, int(now.microsecond/10000))


def write_bootloader(image_name, bootloader_name):
    bootloader_file = open(bootloader_name, mode='rb')
    image_file = open(image_name, mode='rb+')

    data = bootloader_file.read(6*BLOCK_SIZE)
    image_file.write(data)

    bootloader_file.close()
    image_file.close()

def write_item_block(image_file_name, item_block : ItemBlock):
    creation_date_time = item_block.creation_date_time
    modification_date_time = item_block.modification_date_time

    content = struct.pack("<64s5H6BH6B412s6H", 
                          item_block.item_name.encode('utf-8'),
                          item_block.always_zero,
                          item_block.user_id,
                          item_block.group_id,
                          item_block.creation_user_id,
                          creation_date_time.year,
                          creation_date_time.month,
                          creation_date_time.day,
                          creation_date_time.hour,
                          creation_date_time.minute,
                          creation_date_time.second,
                          creation_date_time.centisecond,
                          modification_date_time.year,
                          modification_date_time.month,
                          modification_date_time.day,
                          modification_date_time.hour,
                          modification_date_time.minute,
                          modification_date_time.second,
                          modification_date_time.centisecond,
                          item_block.reserved.encode('utf-8'),
                          item_block.block_id,
                          item_block.mode,
                          item_block.next_id,
                          item_block.first_child_id,
                          item_block.parent_id,
                          item_block.data_size
                          )
    
    image_file = open(image_file_name, mode='rb+')
    image_file.seek(item_block.block_id*BLOCK_SIZE)
    image_file.write(content)
    image_file.close()

def write_data_block(image_file_name, data_block: DataBlock):
    content = struct.pack("<500s6H", 
                          data_block.raw_data,
                          data_block.block_id,
                          data_block.mode,
                          data_block.next_id,
                          data_block.first_child_id,
                          data_block.parent_id,
                          data_block.data_size
                          )
    image_file = open(image_file_name, mode='rb+')
    image_file.seek(data_block.block_id*BLOCK_SIZE)
    image_file.write(content)
    image_file.close()

def read_item_block(image_file_name, block_id):
    image_file = open(image_file_name, 'rb')
    image_file.seek(block_id*BLOCK_SIZE)
    content = image_file.read(BLOCK_SIZE)
    image_file.close()
    data = struct.unpack("<64s5H6BH6B412s6H", content)
    return ItemBlock(data[0].decode("utf-8"), data[1], data[2], data[3], data[4], DateTime(data[5], data[6], data[7], data[8], data[9], data[10], data[11]), DateTime(data[12], data[13], data[14], data[15], data[16], data[17], data[18]), data[19].decode("utf-8"), data[20], data[21], data[22], data[23], data[24], data[25])

def read_data_block(image_file_name, block_id):
    image_file = open(image_file_name, 'rb')
    image_file.seek(block_id*BLOCK_SIZE)
    content = image_file.read(BLOCK_SIZE)
    image_file.close()
    data = struct.unpack("<500s6H", content)
    return DataBlock(data[0], data[1], data[2], data[3], data[4], data[5], data[6])


def write_root(image_file_name, disk_size):
    cur_datetime = get_cur_datetime()
    root = ItemBlock("", 0, 0, 0, 0, cur_datetime, cur_datetime, "", 6, DIRECTORY_MODE, 7, 0, 0, 2*disk_size)
    write_item_block(image_file_name, root)

def alloc_block(image_file_name):
    root = read_item_block(image_file_name, 6)
    root.next_id += 1
    write_item_block(image_file_name, root)
    return root.next_id-1

def import_file(image_file_name, source, destination):
    directories = destination.split('/')
    cur_dir = read_item_block(image_file_name, 6)
    
    #Checking if path exists, and creates it if it doesn't
    for i in range(1, len(directories)-1):
        if(cur_dir.first_child_id == 0):
            id = alloc_block(image_file_name)
            cur_datetime = get_cur_datetime()
            write_item_block(image_file_name, ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
            cur_dir = read_item_block(image_file_name, cur_dir.block_id)
            cur_dir.first_child_id = id
            write_item_block(image_file_name, cur_dir)
            cur_dir = read_item_block(image_file_name, id)
        else:
            cur_item = read_item_block(image_file_name, cur_dir.first_child_id)
            found = False
            while True:
                if(cur_item.item_name == directories[i]):
                    found = True
                    break
                if(cur_item.next_id == 0):
                    break
                cur_item = read_item_block(image_file_name, cur_item.next_id)
            if(not found):
                id = alloc_block(image_file_name)
                cur_datetime = get_cur_datetime()
                write_item_block(image_file_name, ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                cur_item = read_item_block(image_file_name, cur_item.block_id)
                cur_item.next_id = id
                write_item_block(image_file_name, cur_item)
                cur_item = read_item_block(image_file_name, id)
            cur_dir = cur_item
    
    id = alloc_block(image_file_name)
    cur_dir = read_item_block(image_file_name, cur_dir.block_id)
    cur_dir.first_child_id = id

    write_item_block(image_file_name, cur_dir)
    cur_datetime = get_cur_datetime()
    cur_file = ItemBlock(directories[-1], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, REGULAR_FILE_MODE, 0, 0, cur_dir.block_id, 0)

    source_file = open(source, 'rb')
    first = True
    prev_id = 0
    while True:
        data = source_file.read(500)
        id = alloc_block(image_file_name)
        write_data_block(image_file_name, DataBlock(data, id, DATA_BLOCK_MODE, 0, 0, cur_file.block_id, len(data)))
        if(first):
            cur_file.first_child_id = id
            write_item_block(image_file_name, cur_file)
            first = False
        else:
            prev_data_block = read_data_block(image_file_name, prev_id)
            prev_data_block.next_id = id
            write_data_block(image_file_name, prev_data_block)
        if(len(data) < 500):
            break
        prev_id = id
    
    source_file.close()

args = sys.argv[1:]
image_file_name = args[0]
bootloader_file_name = args[1]
disk_size = int(args[2])
files_pair = [] #(source path, destination path)
for i in range (3, len(args), 2):
    files_pair.append((args[i], args[i+1]))

write_bootloader(image_file_name, bootloader_file_name)
write_root(image_file_name, disk_size)

for files in files_pair:
    import_file(image_file_name, files[0], files[1])