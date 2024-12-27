import sys
import stat
import struct
import datetime

BLOCK_SIZE = 512
DIRECTORY_MODE    = stat.S_IFDIR|stat.S_IRUSR|stat.S_IWUSR|stat.S_IRGRP|stat.S_IWGRP|stat.S_IROTH|stat.S_IWOTH
REGULAR_FILE_MODE = stat.S_IFREG|stat.S_IRUSR|stat.S_IWUSR|stat.S_IRGRP|stat.S_IWGRP|stat.S_IROTH|stat.S_IWOTH
DATA_BLOCK_MODE   = 0x0000
CHAR_DEV_MODE     = stat.S_IFCHR|stat.S_IRUSR|stat.S_IWUSR|stat.S_IRGRP|stat.S_IWGRP|stat.S_IROTH|stat.S_IWOTH
BLOCK_DEV_MODE    = stat.S_IFBLK|stat.S_IRUSR|stat.S_IWUSR|stat.S_IRGRP|stat.S_IWGRP|stat.S_IROTH|stat.S_IWOTH
MODE_MASK = 0o777

class DateTime:
    def __init__(self, year: int, month: int, day: int, hour: int, minute: int, second: int, centisecond: int):
        self.year        = year        # 2 bytes
        self.month       = month       # 1 byte
        self.day         = day         # 1 byte
        self.hour        = hour        # 1 byte
        self.minute      = minute      # 1 byte
        self.second      = second      # 1 byte
        self.centisecond = centisecond # 1 byte
    
    def get_cur_datetime():
        now = datetime.datetime.now()
        return DateTime(now.year, now.month, now.day, now.hour, now.minute, now.second, int(now.microsecond/10000))


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

class BRFS:

    def __init__(self, image_file_name):
        self.image_file = open(image_file_name, mode='rb+')


    def write_bootloader(self, bootloader_name):
        bootloader_file = open(bootloader_name, mode='rb')

        data = bootloader_file.read(6*BLOCK_SIZE)
        self.image_file.seek(0)
        self.image_file.write(data)

        bootloader_file.close()
    
    def write_root(self, disk_size):
        cur_datetime = DateTime.get_cur_datetime()
        root = ItemBlock("", 0, 0, 0, 0, cur_datetime, cur_datetime, "", 6, DIRECTORY_MODE, 7, 0, 0, 2*disk_size)
        self.write_item_block(root)
        eod  = DataBlock("".encode(), 7, DATA_BLOCK_MODE, 0, 0, 0, 0)
        self.write_data_block(eod)

    def write_item_block(self, item_block : ItemBlock):
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
        
        self.image_file.seek(item_block.block_id*BLOCK_SIZE)
        self.image_file.write(content)
    
    def read_item_block(self, block_id):
        self.image_file.seek(block_id*BLOCK_SIZE)
        content = self.image_file.read(BLOCK_SIZE)
        data = struct.unpack("<64s5H6BH6B412s6H", content)
        return ItemBlock(data[0].decode("utf-8"), data[1], data[2], data[3], data[4], DateTime(data[5], data[6], data[7], data[8], data[9], data[10], data[11]), DateTime(data[12], data[13], data[14], data[15], data[16], data[17], data[18]), data[19].decode("utf-8"), data[20], data[21], data[22], data[23], data[24], data[25])


    def write_data_block(self, data_block: DataBlock):
        content = struct.pack("<500s6H", 
                            data_block.raw_data,
                            data_block.block_id,
                            data_block.mode,
                            data_block.next_id,
                            data_block.first_child_id,
                            data_block.parent_id,
                            data_block.data_size
                            )
        
        self.image_file.seek(data_block.block_id*BLOCK_SIZE)
        self.image_file.write(content)
    
    def read_data_block(self, block_id):
        self.image_file.seek(block_id*BLOCK_SIZE)
        content = self.image_file.read(BLOCK_SIZE)
        data = struct.unpack("<500s6H", content)
        return DataBlock(data[0], data[1], data[2], data[3], data[4], data[5], data[6])

    def alloc_block(self):
        root = self.read_item_block(6)
        eod  = self.read_item_block(root.next_id)
        if(eod.next_id == 0):
            alloc_id = root.next_id
            root.next_id += 1
            eod.block_id += 1
            self.write_item_block(root)
        else:
            deleted_block = self.read_data_block(eod.next_id)
            alloc_id = deleted_block.block_id
            eod.next_id = deleted_block.next_id
        self.write_item_block(eod)
        return alloc_id
    
    def free_block(self, block : DataBlock|ItemBlock):
        self.free_child_block(block, False)

    def free_child_block(self, block : DataBlock|ItemBlock, free_next_block = True):
        if(block.first_child_id):
            child_block = self.read_item_block(block.first_child_id)
            self.free_child_block(child_block)
        if(block.next_id and free_next_block):
            next_block = self.read_item_block(block.next_id)
            self.free_child_block(next_block)
        root = self.read_item_block(6)
        eod  = self.read_item_block(root.next_id)
        deleted_block = DataBlock("".encode(), block.block_id, DATA_BLOCK_MODE, eod.next_id, 0, 0, 0)
        eod.next_id = deleted_block.block_id
        self.write_item_block(eod)
        self.write_data_block(deleted_block)
    
    def import_file(self, source, destination):
        directories = destination.split('/')
        cur_dir = self.read_item_block(6)
        
        #Checking if path exists, and creates it if it doesn't
        for i in range(1, len(directories)-1):
            if(cur_dir.first_child_id == 0):
                id = self.alloc_block()
                cur_datetime = DateTime.get_cur_datetime()
                self.write_item_block(ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                cur_dir = self.read_item_block(cur_dir.block_id)
                cur_dir.first_child_id = id
                self.write_item_block(cur_dir)
                cur_dir = self.read_item_block(id)
            else:
                cur_item = self.read_item_block(cur_dir.first_child_id)
                found = False
                while True:
                    if(cur_item.item_name == directories[i]):
                        found = True
                        break
                    if(cur_item.next_id == 0):
                        break
                    cur_item = self.read_item_block(cur_item.next_id)
                if(not found):
                    id = self.alloc_block()
                    cur_datetime = DateTime.get_cur_datetime()
                    self.write_item_block(ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                    cur_item = self.read_item_block(cur_item.block_id)
                    cur_item.next_id = id
                    self.write_item_block(cur_item)
                    cur_item = self.read_item_block(id)
                cur_dir = cur_item
        
        id = self.alloc_block()
        cur_dir = self.read_item_block(cur_dir.block_id)
        cur_dir.first_child_id = id

        self.write_item_block(cur_dir)
        cur_datetime = DateTime.get_cur_datetime()
        cur_file = ItemBlock(directories[-1], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, REGULAR_FILE_MODE, 0, 0, cur_dir.block_id, 0)

        source_file = open(source, 'rb')
        first = True
        prev_id = 0
        while True:
            data = source_file.read(500)
            id = self.alloc_block()
            self.write_data_block(DataBlock(data, id, DATA_BLOCK_MODE, 0, 0, cur_file.block_id, len(data)))
            if(first):
                cur_file.first_child_id = id
                self.write_item_block(cur_file)
                first = False
            else:
                prev_data_block = self.read_data_block(prev_id)
                prev_data_block.next_id = id
                self.write_data_block(prev_data_block)
            if(len(data) < 500):
                break
            prev_id = id
        
        source_file.close()
    
    def export_file(self, source, destination):
        directories = source.split('/')
        cur_dir = self.read_item_block(6)
        
        #Searching for file
        for i in range(1, len(directories)):
            if(cur_dir.first_child_id == 0):
                return False
            else:
                cur_item = self.read_item_block(cur_dir.first_child_id)
                found = False
                while True:
                    if(cur_item.item_name == directories[i]):
                        found = True
                        break
                    if(cur_item.next_id == 0):
                        break
                    cur_item = self.read_item_block(cur_item.next_id)
                if(not found):
                    return False
                cur_dir = cur_item
        
        destination_file = open(destination, 'rb')
        
        data = self.read_data_block(cur_item.first_child_id)
        while True:
            raw_data = data.raw_data[:data.data_size]
            destination_file.write(raw_data)

            if(data.next_id == 0):
                break

            data = self.read_data_block(data.next_id)

        destination_file.close()
        return True
    
    def make_dir(self, path):
        directories = path.split('/')
        cur_dir = self.read_item_block(6)
        
        #Checking if directories exists, and creates it if it doesn't
        for i in range(1, len(directories)):
            if(cur_dir.first_child_id == 0):
                id = self.alloc_block()
                cur_datetime = DateTime.get_cur_datetime()
                self.write_item_block(ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                cur_dir = self.read_item_block(cur_dir.block_id)
                cur_dir.first_child_id = id
                self.write_item_block(cur_dir)
                cur_dir = self.read_item_block(id)
            else:
                cur_item = self.read_item_block(cur_dir.first_child_id)
                found = False
                while True:
                    if(cur_item.item_name == directories[i]):
                        found = True
                        break
                    if(cur_item.next_id == 0):
                        break
                    cur_item = self.read_item_block(cur_item.next_id)
                if(not found):
                    id = self.alloc_block()
                    cur_datetime = DateTime.get_cur_datetime()
                    self.write_item_block(ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                    cur_item = self.read_item_block(cur_item.block_id)
                    cur_item.next_id = id
                    self.write_item_block(cur_item)
                    cur_item = self.read_item_block(id)
                cur_dir = cur_item
        return True
    
    def delete_item(self, path):
        directories = path.split('/')
        cur_dir = self.read_item_block(6)
        prev_item = None
        
        #Searching for file
        for i in range(1, len(directories)):
            if(cur_dir.first_child_id == 0):
                return False
            else:
                cur_item = self.read_item_block(cur_dir.first_child_id)
                found = False
                while True:
                    if(cur_item.item_name == directories[i]):
                        found = True
                        break
                    if(cur_item.next_id == 0):
                        break
                    prev_item = cur_item
                    cur_item = self.read_item_block(cur_item.next_id)
                if(not found):
                    return False
                cur_dir = cur_item
        
        parent_block = self.read_item_block(cur_item.parent_id)
        if(parent_block.first_child_id == cur_item.block_id):
            parent_block.first_child_id = cur_item.next_id
            self.write_item_block(parent_block)
        else:
            prev_item.block_id = cur_item.next_id
            self.write_item_block(prev_item)
        self.free_block(cur_item)
        
        return True
        
    def change_mode(self, path, mode):
        directories = path.split('/')
        cur_dir = self.read_item_block(6)
        mode = mode & MODE_MASK
        
        #Searching for item
        for i in range(1, len(directories)):
            if(cur_dir.first_child_id == 0):
                return False
            else:
                cur_item = self.read_item_block(cur_dir.first_child_id)
                found = False
                while True:
                    if(cur_item.item_name == directories[i]):
                        found = True
                        break
                    if(cur_item.next_id == 0):
                        break
                    cur_item = self.read_item_block(cur_item.next_id)
                if(not found):
                    return False
                cur_dir = cur_item

        cur_item.mode = (cur_item.mode & ~MODE_MASK) | mode
        self.write_item_block(cur_item)
        return True

    def make_device(self, path, is_char_device):
        directories = path.split('/')
        cur_dir = self.read_item_block(6)
        
        #Checking if directories exists, and creates it if it doesn't
        for i in range(1, len(directories)-1):
            if(cur_dir.first_child_id == 0):
                id = self.alloc_block()
                cur_datetime = DateTime.get_cur_datetime()
                self.write_item_block(ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                cur_dir = self.read_item_block(cur_dir.block_id)
                cur_dir.first_child_id = id
                self.write_item_block(cur_dir)
                cur_dir = self.read_item_block(id)
            else:
                cur_item = self.read_item_block(cur_dir.first_child_id)
                found = False
                while True:
                    if(cur_item.item_name == directories[i]):
                        found = True
                        break
                    if(cur_item.next_id == 0):
                        break
                    cur_item = self.read_item_block(cur_item.next_id)
                if(not found):
                    id = self.alloc_block()
                    cur_datetime = DateTime.get_cur_datetime()
                    self.write_item_block(ItemBlock(directories[i], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, DIRECTORY_MODE, 0, 0, cur_dir.block_id, 0))
                    cur_item = self.read_item_block(cur_item.block_id)
                    cur_item.next_id = id
                    self.write_item_block(cur_item)
                    cur_item = self.read_item_block(id)
                cur_dir = cur_item
        
        id = self.alloc_block()
        cur_dir = self.read_item_block(cur_dir.block_id)
        cur_dir.first_child_id = id

        self.write_item_block(cur_dir)
        cur_datetime = DateTime.get_cur_datetime()
        cur_file = ItemBlock(directories[-1], 0, 0, 0, 0, cur_datetime, cur_datetime, "", id, (CHAR_DEV_MODE if is_char_device else BLOCK_DEV_MODE), 0, 0, cur_dir.block_id, 0)
        self.write_item_block(cur_file)
       
    def __del__(self):
        self.image_file.close()