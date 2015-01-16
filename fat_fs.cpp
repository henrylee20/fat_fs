#include "fat_fs.h"
#include <Windows.h>

bool read_disk(UINT8* buffer, int sector_size, int offset, int count)
{
	HANDLE hDisk = CreateFileW(L"\\\\.\\F:",
								GENERIC_READ|GENERIC_WRITE,
								FILE_SHARE_READ|FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								NULL,
								NULL) ; 
	SetFilePointer(hDisk , offset , 0 , FILE_BEGIN) ;
	memset(buffer, 0xee, sector_size * count * sizeof(BYTE));

	DWORD dwReadLength = 0 ;
	ReadFile(hDisk , buffer , sector_size * count , &dwReadLength , NULL) ;

	CloseHandle(hDisk);
	return true;
}

void init_volume(volume_info* volume, file_info* root)
{
	UINT8 DBR_buffer[SECTOR_SIZE];
	read_disk(DBR_buffer, SECTOR_SIZE, 0, 1);

	volume->sector_size = (UINT16)get_num(&DBR_buffer[OFFSET_SECTOR_SIZE], uint16);
	volume->clust_size = DBR_buffer[OFFSET_CLUST_SIZE];
	volume->clust_offset = volume->sector_size * volume->clust_size;

	volume->fat_offset = volume->sector_size * get_num(&DBR_buffer[OFFSET_REV_SECTOR], uint16);
	volume->root_offset = volume->sector_size * 
					(get_num(&DBR_buffer[OFFSET_REV_SECTOR], uint16) + 
					 get_num(&DBR_buffer[OFFSET_FAT32_SECTOR], uint32) * 2 + 
					 (get_num(&DBR_buffer[OFFSET_ROOT_CLUST], uint32) - 2) * volume->clust_size);		//root'offset = sector_size * (rev_sector + 2 * fat_sector + (root_clust - 2) * clust_sector)

	read_disk(volume->fat, volume->sector_size, volume->fat_offset, FAT_SIZE / volume->sector_size);
	volume->fat_part_num = 0;

	//---------------------Set root dir------------------------
	root->name[0] = 'r';
	root->name[1] = 'o';
	root->name[2] = 'o';
	root->name[3] = 't';
	root->name[4] = '\0';
	root->name_len = 4;
	root->attri = FILE_ATTRI_DIR | FILE_ATTRI_SYSTEM | FILE_ATTRI_HIDDEN;
	root->offset = volume->root_offset;
	root->first_clust = get_num(&DBR_buffer[OFFSET_ROOT_CLUST], uint32);
	root->fs = volume;
	root->size = volume->clust_offset * (get_last_clust_num(root->first_clust, *volume) + 1);
}

UINT32 get_num(UINT8* buffer, num_type type)
{
	switch(type) {
		case uint16:
			return (UINT16)(buffer[0] | (buffer[1] << 8));
			break;
		case uint32:
			return (UINT32)(buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24));
			break;
		default:
			return 0xFFFFFFFF;
			break;
	}
}

UINT32 get_file_list(file_info& dir, file_info* file_list)
{
	UINT8 buffer[CLUST_SIZE] = {0};
	UINT8* p;			//Point to a block
	UINT8 i = 0;		//Counter
	file_info* file = file_list;	//file iter
	UINT8 block_num = 0;	//Long file name block number
	UINT32 write_pos = 0;	//Write position of 'name'
	UINT32 clust_num = 0;
	UINT32 now_clust = dir.first_clust;
	char name[NAME_MAX_LEN] = {0};	//Temp buffer of long file name

	if (!(dir.attri & FILE_ATTRI_DIR)) {	//this file is not a dir
		return 0;
	}

	clust_num = get_last_clust_num(dir.first_clust, *dir.fs);		//Get number of clusts
	
	for(UINT32 j = 0; j <= clust_num; ++j)  {
		read_clust(*dir.fs, buffer, now_clust);

		for (p = buffer; *p != DIR_VAL_EMPTY; p += 0x20) {
			if (p[0] == DIR_VAL_DELETED) {
				continue;
			}

			if (p[DIR_OFFSET_ATTRI] == FILE_ATTRI_LONGNAME) {		//Is long file name

				block_num = p[0] & DIR_VAL_NAME_BLOCK_MASK;		//Get block num

				if (p[0] & DIR_VAL_LAST_NAME_BLOCK) {			//Is last block
					file->name_len = block_num * DIR_VAL_BLOCK_NAME_LEN;			//Calc other name length
				}

				i = 0x01;
				write_pos = file->name_len - (block_num - 1) * DIR_VAL_BLOCK_NAME_LEN;	//Calc write position of 'name'
				while ((p[i] || p[i+1]) && i < 0x20) {			//Read name
					name[write_pos--] = p[i];

					if (i == 0x1A - 2) {					//Jump to next aera
						i = 0x1C;
					} else if (i == 0x0B - 2) {
						i = 0x0E;
					} else {
						i += 2;
					}
				}
			} else {								// Normal file block
				file->fs = dir.fs;
				get_file_info_base(file, p);		//Get other infomation	
				if (p[6] == '~' && p[7] >='1' && p[7] <= '9') {		//Use long file name
					i = 0;
					while (name[file->name_len]) {
						file->name[i++] = name[file->name_len--];
					}
					file->name[i] = 0;
					file->name_len = i;
				}
				++file;												//Point to next file_info
				memset(name, 0, NAME_MAX_LEN);						//Clear memories
			}
		}

		if (j != clust_num) {
			now_clust = get_next_clust(now_clust, *dir.fs);			//get next clust
		}
	}

	return file - file_list;
}

UINT32 get_file_num(file_info& dir)
{
	UINT8 buffer[CLUST_SIZE] = {0};
	UINT8* p = buffer;
	UINT32 clust_num;
	UINT32 result = 0;
	UINT32 now_clust = dir.first_clust;

	if (!(dir.attri & FILE_ATTRI_DIR)) {	//this file is not a dir
		return 0;
	}

	clust_num = get_last_clust_num(dir.first_clust, *dir.fs);		//Get number of clusts
	
	for(UINT32 i = 0; i <= clust_num; ++i)  {
		read_clust(*dir.fs, buffer, now_clust);

		for (p = buffer; *p != DIR_VAL_EMPTY; p += 0x20) {			//scan this clust
			if (p[0] != DIR_VAL_DELETED && p[DIR_OFFSET_ATTRI] != FILE_ATTRI_LONGNAME) {
				++result;
			}
		}

		if (i != clust_num) {
			now_clust = get_next_clust(now_clust, *dir.fs);			//get next clust
		}
	}

	return result;
}

void get_file_info_base(file_info* info, UINT8* buffer)
{
	if (info == NULL) {
		return;
	}
	if (info->name == NULL) {
		return;
	}

	//Get short file name
	char* p = info->name;
	for (int i = 0; i < 8; ++i) {
		*p = buffer[DIR_OFFSET_NAME + i];
		if (buffer[DIR_OFFSET_NT_REV] & 0x10 && *p >= 'A' && *p <= 'Z') {
			*p -= ('A' - 'a');
		}
		++p;
	}

	p--;
	while (*p == 0x20) {	//Remove blank
		p--;
	}
	p++;
	*(p++) = '.';

	for (int i = 0; i < 3; ++i) {
		*p = buffer[DIR_OFFSET_EX_NAME + i];
		if (buffer[DIR_OFFSET_NT_REV] & 0x08 && *p >= 'A' && *p <= 'Z') {
			*p -= ('A' - 'a');
		}
		++p;
	}
	p--;
	while (*p == 0x20) {	//Remove blank
		p--;
	}
	*(p+1) = 0x00;

	//Get file attribution
	info->attri = buffer[DIR_OFFSET_ATTRI];

	//Get file time info
	UINT16 time_data;

	time_data = get_num(&buffer[DIR_OFFSET_CREATE_TIME], uint16);
	file_get_time(&info->create_time, time_data);
	time_data = get_num(&buffer[DIR_OFFSET_CREATE_DATE], uint16);
	file_get_date(&info->create_time, time_data);
	info->create_time.ms = (buffer[DIR_OFFSET_CREATE_MS] % 100) * 10;
	info->create_time.s += buffer[DIR_OFFSET_CREATE_MS] / 100;

	time_data = get_num(&buffer[DIR_OFFSET_RESENT_DATE], uint16);
	file_get_date(&info->resent_time, time_data);

	time_data = get_num(&buffer[DIR_OFFSET_CHANGE_TIME], uint16);
	file_get_time(&info->change_time, time_data);
	time_data = get_num(&buffer[DIR_OFFSET_CHANGE_DATE], uint16);
	file_get_date(&info->change_time, time_data);

	//Get file size
	if (info->attri & FILE_ATTRI_DIR) {
		info->size = info->fs->clust_offset * (get_last_clust_num(info->first_clust, *info->fs) + 1);
	} else {
		info->size = get_num(&buffer[DIR_OFFSET_FILE_SIZE], uint32);
	}

	//Get first clust number
	info->first_clust = (UINT16)get_num(&buffer[DIR_OFFSET_F_CLUST_LOW], uint16) | 
						((UINT16)get_num(&buffer[DIR_OFFSET_F_CLUST_HIGH], uint16) << 16);

	info->offset = info->fs->root_offset + (info->first_clust - 2) * info->fs->clust_offset;
}

void file_get_time(time_type* time_info, UINT16 time_data)
{
	time_info->s = 2 * (time_data & 0x001F);
	time_info->min = (time_data & 0x07E0) >> 5;
	time_info->hour = (time_data & 0xF800) >> 11;
}

void file_get_date(time_type* time_info, UINT16 date_data)
{
	time_info->day = date_data & 0x001F;
	time_info->mon = (date_data & 0x01E0) >> 5;
	time_info->year = ((date_data & 0xFE00) >> 9) + 1980;
};

UINT32 read_file(file_info &file, UINT8* buffer, UINT32 size, UINT32 offset)
{
	UINT32 next_clust = file.first_clust;
	UINT32 last_size;
	UINT32 i = 0;
	UINT8 buf[CLUST_SIZE] = {0};
	UINT8* p = buffer;

	UINT32 root_offset = file.fs->root_offset;
	UINT32 clust_offset = file.fs->clust_offset;

	if (offset > file.size) {
		return 0;
	}

	if (size + offset > file.size) {	//Out of range
		size = file.size - offset;
	}
	last_size = size;

	if (!next_clust) {		//Empty
		return 0;
	}

	//Jump to offset clust
	for (i = 0; i < offset / clust_offset; ++i) {
		next_clust = get_next_clust(next_clust, *file.fs);
	}

	//Get offset in the clust
	if (offset > clust_offset) {
		offset = offset % clust_offset;
	}

	do {
		read_clust(*file.fs, buf, next_clust);	//Read clust
		
		for (i = offset; i < clust_offset && last_size; ++i) {		//Copy data
			*(p++) = buf[i];
			--last_size;
		}
		offset = 0;				//only first clust needs offset

		if(!last_size) {		//Finished
			break;
		} else {
			next_clust = get_next_clust(next_clust, *file.fs);	//Get next clust;
		}
	} while (next_clust > CLUST_REV1 && next_clust < CLUST_REV2);

	return size;
}

void read_clust(volume_info &volume, UINT8* buffer, UINT32 clust_num)
{
	read_disk(buffer, volume.sector_size, volume.root_offset + (clust_num - 2) * volume.clust_offset, volume.clust_size);
}

UINT32 get_next_clust(UINT32 now_clust, volume_info& fs)
{
	UINT32 part = now_clust / (FAT_SIZE / 4);
	if (part != fs.fat_part_num) {
		read_disk(fs.fat, fs.sector_size, fs.fat_offset + part * FAT_SIZE, FAT_SIZE / fs.sector_size);
		fs.fat_part_num = part;
	}
	return get_num(&fs.fat[(now_clust % (FAT_SIZE / 4)) * 4], uint32);
}

UINT32 get_last_clust_num(UINT32 now_clust, volume_info& fs)
{
	UINT32 result = 0;
	now_clust = get_next_clust(now_clust, fs);
	while (now_clust > CLUST_REV1 && now_clust < CLUST_REV2) {
		++result;
		now_clust = get_next_clust(now_clust, fs);
	}
	return result;
}