#include <iostream>
#include "fat_fs.h"

using namespace std;

int main()
{
	cout << "hello world" << endl;

	//--------------Test basic info---------------
	volume_info volume;
	file_info root;

	init_volume(&volume, &root);
	printf("Sector size: \t0x%x\n", volume.sector_size);
	printf("Clust size: \t0x%x\n", volume.clust_size);
	printf("Clust offset: \t0x%x\n", volume.clust_offset);
	printf("FAT postion: \t0x%x\n", volume.fat_offset);
	printf("Root postion: \t0x%x\n", volume.root_offset);
	
	printf("Sector size: \t0x%x\n", root.fs->sector_size);
	printf("Clust size: \t0x%x\n", root.fs->clust_size);
	printf("Clust offset: \t0x%x\n", root.fs->clust_offset);
	printf("FAT postion: \t0x%x\n", root.fs->fat_offset);
	printf("Root postion: \t0x%x\n", root.fs->root_offset);
	//system("pause");

	//----------------Test dir read----------------

	file_info info;
	file_info *file_list = new file_info[get_file_num(root)];

	int file_num = get_file_list(root, file_list);
	printf("There is %d files:\n", file_num);
	for (int i = 0; i < file_num; ++i) {
		info = file_list[i];
		printf("File name:\t%s\t%x\n", info.name, info.attri);
		printf("Create time:\t%d-%d-%d %d:%d:%d.%d\n", info.create_time.year, info.create_time.mon, info.create_time.day, info.create_time.hour, info.create_time.min, info.create_time.s, info.create_time.ms);
		printf("File size:\t%d\n", info.size);
		printf("Clust number:\t%d\n", info.first_clust);
		printf("Offset:\t0x%x\n", info.offset);
		printf("------------------------\n");
	}

	//----------------Test file read--------------------
	UINT8 file_buffer[0x4000] = {0};
	UINT32 read_size;
	cin >> file_num;
	read_size = read_file(file_list[file_num], file_buffer);

	for (UINT8* p = file_buffer; *p; p += 16) {
		printf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}

	printf("Get %d bytes data\n", read_size);

	system("pause");
	return 0;
}
