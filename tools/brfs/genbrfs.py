import sys
from brfslib import BRFS

args = sys.argv[1:]
image_file_name = args[0]
bootloader_file_name = args[1]
disk_size = int(args[2])
files_pair = [] #(source path, destination path)
for i in range (3, len(args), 2):
    files_pair.append((args[i], args[i+1]))

brfs = BRFS(image_file_name)

brfs.write_bootloader(bootloader_file_name)
brfs.write_root(disk_size)

for files in files_pair:
    brfs.import_file(files[0], files[1])

del brfs