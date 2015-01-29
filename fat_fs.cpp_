#include "fat_fs.h"
#include <iostream>
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
	mem_set(buffer, 0xee, sector_size * count * sizeof(UINT8));
	//memset(buffer, 0xe, sector_size * count * sizeof(UINT8));

	DWORD dwReadLength = 0 ;
	ReadFile(hDisk , buffer , sector_size * count , &dwReadLength , NULL) ;

	CloseHandle(hDisk);
	return true;
}

bool write_disk(UINT8* buffer, int sector_size, int offset, int count)
{
	return false;
}

/**************************************************************/

UINT32 mem_set(void* buffer, UINT8 val, UINT32 length)
{
	for (; length > 0; --length) {
		((UINT8*)buffer)[length - 1] = val;
	}
	return length;
}

UINT32 mem_cpy(void* target, const void* source, UINT32 length)
{
	for (; length > 0; --length) {
		((UINT8*)target)[length - 1] = ((UINT8*)source)[length - 1];	
	}
	return length;
}

UINT32 str_len(const UINT8* str)
{
	UINT32 len = 0;

	while (*str++) {
		++len;
	}

	return len;
}

UINT32 str_find_c(const UINT8* str, UINT8 c, UINT32 offset)
{
	UINT32 len = str_len(str);

	for (; offset < len; ++offset) {
		if (str[offset] == c) {
			return offset;
		}
	}

	return 0xFFFFFFFF;
}

UINT32 str_cmp(UINT8* str1, const UINT8* str2)
{
	UINT32 result;
	UINT32 len = str_len(str1);
	if (len > str_len(str2)) {
		len = str_len(str2);
	}

	result = str1[len] - str2[len];

	for (UINT32 i = 0; i < len; ++i)
	{
		if (str1[i] != str2[i]) {
			return str1[i] - str2[i];
		}
	}

	return result;
}

/***************************************************************/

void init_volume(volume_info* volume, file_info* root)
{
	UINT8 DBR_buffer[SECTOR_SIZE];
	read_disk(DBR_buffer, SECTOR_SIZE, 0, 1);

	volume->sector_size = (UINT16)get_num(&DBR_buffer[OFFSET_SECTOR_SIZE], uint16);
	volume->clust_size = DBR_buffer[OFFSET_CLUST_SIZE];
	volume->clust_offset = volume->sector_size * volume->clust_size;

	volume->fat_offset = volume->sector_size * get_num(&DBR_buffer[OFFSET_REV_SECTOR], uint16);
	volume->root_clust = get_num(&DBR_buffer[OFFSET_ROOT_CLUST], uint32);
	volume->root_offset = volume->sector_size * 
					(get_num(&DBR_buffer[OFFSET_REV_SECTOR], uint16) + 
					 get_num(&DBR_buffer[OFFSET_FAT32_SECTOR], uint32) * 2 + 
					 (volume->root_clust - 2) * volume->clust_size);		//root'offset = sector_size * (rev_sector + 2 * fat_sector + (root_clust - 2) * clust_sector)

	read_disk(volume->fat, volume->sector_size, volume->fat_offset, FAT_SIZE / volume->sector_size);
	volume->fat_part_num = 0;
	volume->fat_size = get_num(&DBR_buffer[OFFSET_FAT32_SECTOR], uint32) * volume->sector_size;

	//Get volume name;
	mem_cpy(volume->name, &DBR_buffer[OFFSET_VOLUME_NAME], LEN_VOLUME_NAME);

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

	volume->root = root;

		//---------------------Read FSInfo sector------------------
	read_disk(DBR_buffer, volume->sector_size, get_num(&DBR_buffer[OFFSET_FSINFO_SECTOR], uint16) * volume->sector_size, 1);

	//Get free space 
	volume->free_clust = get_num(&DBR_buffer[OFFSET_FSINFO_FREE], uint32);
	volume->next_clust = get_num(&DBR_buffer[OFFSET_FSINFO_NEXT], uint32);
}

void update_volume_info(volume_info& volume)
{
	UINT8 buffer[SECTOR_SIZE];
	UINT32 v_info_offset;

	read_disk(buffer, SECTOR_SIZE, 0, 1);
	v_info_offset = get_num(&buffer[OFFSET_FSINFO_SECTOR], uint16) * volume.sector_size;
	read_disk(buffer, volume.sector_size, v_info_offset, 1);

	set_num(volume.free_clust, &buffer[OFFSET_FSINFO_FREE], uint32);
	set_num(volume.next_clust, &buffer[OFFSET_FSINFO_NEXT], uint32);

	write_disk(buffer, volume.sector_size, v_info_offset, 1);
}

/***************************************************************/

file_info f_open(file_info& now_dir, const UINT8* file_name)
{
	file_info file;
	if (f_open(&file, now_dir, file_name) == RESULT_NOT_FOUND) {

	}
	return file;
}

UINT8 f_open(file_info* file, file_info& now_dir, const UINT8* file_name)
{
	file_info dir;
	UINT8 file_name_pos;

	//mem_set(&file, 0, sizeof(file_info));
	file_name_pos = get_file_dir(&dir, now_dir, file_name);
	if (file_name_pos == RESULT_WRONG_PATH) {
		return RESULT_WRONG_PATH;
	}

	if (get_file_info(file, dir, &file_name[file_name_pos]) == RESULT_NOT_FOUND) {
		return RESULT_NOT_FOUND;
	}

	return RESULT_OK;
}

UINT8 get_file_dir(file_info* target, file_info& start, const UINT8* path)
{
	//file_info target = start;
	UINT8 i;
	UINT8 last_pos;
	UINT32 step = 0;
	UINT8 path_part[NAME_MAX_LEN] = {0};

	*target = start;

	last_pos = str_len(path) - 1;
	while (path[last_pos] != '/' && path[last_pos] != '\\' && last_pos != 0xFF) {	//find last '/'
		--last_pos;
	}
	++last_pos;

	if (last_pos == 0xFF) {
		return last_pos;
	}

	if (*path == '/' || *path == '\\') {
		*target = *(file_info*)start.fs->root;
		++path;
		++step;
	}

	while (*path && step != last_pos) {
		for (i = 0; *path != '/' && *path != '\\' && *path; ++i, ++path, ++step) {
			path_part[i] = *path;
		}
		path_part[i] = '\0';
		++path;			//Jump '/'
		++step;

		if (path_part[0] == '.' && path_part[1] == '\0') {
			continue;
		}

		if(get_file_info(target, *target, path_part) == RESULT_NOT_FOUND) {
			return RESULT_WRONG_PATH;
		}
	}
	return last_pos;
}

UINT8 get_file_info(file_info* file, file_info& dir, const UINT8* file_name)
{
	file_info files[DIR_LIST_LEN];
	UINT32 file_num = get_file_num(dir);	//sum of files
	UINT32 got_file_num;					//count of file gotten
	UINT16 part_num = 0;

	while (file_num) {
		got_file_num = get_file_list(dir, files, part_num++, DIR_LIST_LEN);
		file_num -= got_file_num;
		for (; got_file_num > 0; --got_file_num) {
			if(!str_cmp(files[got_file_num - 1].name, file_name)) {
				*file = files[got_file_num - 1];
				return RESULT_OK;
			}
		}
	}
	return RESULT_NOT_FOUND;
}

UINT32 get_file_list(file_info& dir, file_info* file_list, UINT32 start, UINT32 max)
{
	UINT8 buffer[CLUST_SIZE] = {0};
	UINT8* p;			//iter of block
	UINT8 i = 0;		//Counter
	file_info* file = file_list;	//file iter
	UINT8 block_num = 0;	//Long file name block number
	UINT32 write_pos = 0;	//Write position of 'name'
	UINT32 clust_num = 0;
	UINT32 now_clust = dir.first_clust;
	UINT8 chk_sum = 0;
	char name[NAME_MAX_LEN] = {0};	//Temp buffer of long file name

	if (!(dir.attri & FILE_ATTRI_DIR)) {	//this file is not a dir
		return 0;
	}

	clust_num = get_last_clust_num(dir.first_clust, *dir.fs);		//Get number of clusts
	
	for(UINT32 j = 0; j <= clust_num; ++j)  {		//Scan each clust
		read_clust(*dir.fs, buffer, now_clust);

		for (p = buffer; *p != DIR_VAL_EMPTY; p += 0x20) {		//Scan each dir record
			if (p[0] == DIR_VAL_DELETED) {
				continue;
			}

			if (p[DIR_OFFSET_ATTRI] == FILE_ATTRI_LONGNAME) {		//Is long file name

				block_num = p[0] & DIR_VAL_NAME_BLOCK_MASK;		//Get block num
				

				if (p[0] & DIR_VAL_LAST_NAME_BLOCK) {			//Is last block
					file->name_len = block_num * DIR_VAL_BLOCK_NAME_LEN;			//Calc other name length
					chk_sum = p[0x0D];
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
			} else {					// Normal file block
				if (start) {			//Jump this file		
					--start;
				} else {				//Finish jump
					file->fs = dir.fs;
					get_file_info_base(file, p);		//Get other infomation	

					//if (p[6] == '~' && p[7] >='1' && p[7] <= '9') {		//Use long file name
					if (chk_sum == file->name_chk) {
						i = 0;
						while (name[file->name_len]) {
							file->name[i++] = name[file->name_len--];
						}
						file->name[i] = '\0';
						file->name_len = i;
					} else {
						file->name_len = str_len(file->name);
					}

					++file;							//Point to next file_info
					mem_set(name, 0, NAME_MAX_LEN);	//Clear memories

					--max;				
					if (!max) {				//Got all user want
						return file - file_list;
					}
				}
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
	UINT8 raw_name[12] = {0};

	if (info == NULL) {
		return;
	}
	if (info->name == NULL) {
		return;
	}
	if (buffer[DIR_OFFSET_ATTRI] == FILE_ATTRI_LONGNAME) {
		return;
	}

	//Get file attribution
	info->attri = buffer[DIR_OFFSET_ATTRI];

	//Get short file name
	UINT8* p = info->name;
	for (int i = 0; i < 8; ++i) {
		*p = buffer[DIR_OFFSET_NAME + i];
		raw_name[i] = *p;
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

	if (!(info->attri & FILE_ATTRI_DIR)) {	//If it's a file, add a '.' to split the ex-name and name
		*(p++) = '.';
	}

	for (int i = 0; i < 3; ++i) {
		*p = buffer[DIR_OFFSET_EX_NAME + i];
		raw_name[i + 8] = *p;
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

	info->name_chk = file_chk_name(raw_name);

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

	if (!info->first_clust) {			//0 means root
		info->first_clust = info->fs->root_clust;
	}

	info->offset = info->fs->root_offset + (info->first_clust - 2) * info->fs->clust_offset;
}

void file_get_time(time_type* time_info, UINT16 time_date)
{
	time_info->s = 2 * (time_date & 0x001F);
	time_info->min = (time_date & 0x07E0) >> 5;
	time_info->hour = (time_date & 0xF800) >> 11;
}

void file_get_date(time_type* time_info, UINT16 date_date)
{
	time_info->day = date_date & 0x001F;
	time_info->mon = (date_date & 0x01E0) >> 5;
	time_info->year = ((date_date & 0xFE00) >> 9) + 1980;
};

/*****************************************************************/

file_info create_new_file(file_info& dir, const UINT8* file_name)
{
	file_info result;
	UINT8 full_name_len = str_len(file_name);
	UINT8 name_start = full_name_len;

	while (file_name[name_start] != '/' && file_name[name_start] != '\\' && name_start != 0xFF) {
		--name_start;
	}
	++name_start;

	result.attri = FILE_ATTRI_DOC;
	//result.change_time;
	//result.change_time;
	//result.change_time;
	result.first_clust = 0;
	result.fs = dir.fs;
	result.name_len = full_name_len - name_start;
	mem_cpy(result.name, &file_name[name_start], result.name_len);

	result.name_chk = file_chk_name(file_name);
	result.offset = dir.fs->root_offset + result.first_clust * dir.fs->clust_offset;
	result.dir = &dir;
	return result;
}

UINT8 set_file_info_base(file_info& info, UINT8* buffer)
{
	if (info.name[0] == '\0') {
		return RESULT_WRONG_NAME;
	}

	//Set file attribution
	buffer[DIR_OFFSET_ATTRI] = info.attri;

	//Set short file name ( Cannot use long file name )
	if(file_is_long_name(info.name)) {
		return RESULT_WRONG_NAME;
	}
	UINT8 short_name[8] = {0};
	UINT8 ex_name[3] = {0};
	UINT8 dot_pos = info.name_len;
	UINT8 i;

	while (info.name[dot_pos] != '.' && dot_pos != 0xFF) {	//Split name
		--dot_pos;
	}
	for (i = 0; i < dot_pos && i < 8; ++i) {
		short_name[i] = info.name[i];
		if (short_name[i] >= 'a' && short_name[i] <= 'z') {
			buffer[DIR_OFFSET_NT_REV] |= 0x10;
			short_name[i] += 'A' - 'a';
		}
	}
	for (i = 0; i < info.name_len - dot_pos - 1 && i < 3; ++i) {
		ex_name[i] = info.name[dot_pos + 1 + i];
		if (ex_name[i] >= 'a' && ex_name[i] <= 'z') {
			buffer[DIR_OFFSET_NT_REV] |= 0x08;
			ex_name[i] += 'A' - 'a';
		}
	}

	for (i = 0; i < 8; ++i) {
		if (short_name[i]) {
			buffer[DIR_OFFSET_NAME + i] = short_name[i];
		} else {
			buffer[DIR_OFFSET_NAME + i] = 0x20;
		}
	}
	for (i = 0; i < 3; ++i) {
		if (ex_name[i]) {
			buffer[DIR_OFFSET_EX_NAME + i] = ex_name[i];
		} else {
			buffer[DIR_OFFSET_EX_NAME + i] = 0x20;
		}
	}

	//Set file time info
	UINT8 ms = info.create_time.ms / 10 + (~info.create_time.s & 1) * 100;
	info.create_time.s /= 2;
	set_num(ms, &buffer[DIR_OFFSET_CREATE_MS], uint16);
	set_num(file_set_time(info.create_time), &buffer[DIR_OFFSET_CREATE_TIME], uint16);
	set_num(file_set_date(info.create_time), &buffer[DIR_OFFSET_CREATE_DATE], uint16);
	info.create_time.s *= 2;
	info.create_time.s += (~(ms / 100)) & 1;

	set_num(file_set_date(info.resent_time), &buffer[DIR_OFFSET_RESENT_DATE] ,uint16);
	set_num(file_set_time(info.change_time), &buffer[DIR_OFFSET_CHANGE_TIME] ,uint16);
	set_num(file_set_date(info.change_time), &buffer[DIR_OFFSET_CHANGE_DATE] ,uint16);

	//Set file size
	if (info.attri & FILE_ATTRI_DIR) {
		set_num(0x00000000, &buffer[DIR_OFFSET_FILE_SIZE], uint32);
	} else {
		set_num(info.size, &buffer[DIR_OFFSET_FILE_SIZE], uint32);
	}

	//Set first clust number
	if (info.first_clust == info.fs->root_clust) {
		set_num(0x0000, &buffer[DIR_OFFSET_F_CLUST_LOW], uint16);
		set_num(0x0000, &buffer[DIR_OFFSET_F_CLUST_HIGH], uint16);
	} else {
		set_num(info.first_clust & 0x0000FFFF, &buffer[DIR_OFFSET_F_CLUST_LOW], uint16);
		set_num((info.first_clust & 0xFFFF0000) >> 16, &buffer[DIR_OFFSET_F_CLUST_HIGH], uint16);
	}

	return RESULT_OK;
}

//UINT8 file_get_short_name(UINT8* short_name, const UINT8* name)
//{
//
//}

UINT8 file_is_long_name(const UINT8* name)
{
	UINT8 dot_pos = str_len(name);
	UINT8 name_len = dot_pos;
	UINT8 i;

	if (name_len > 11) {
		return TRUE;
	}

	while (name[dot_pos] != '.' && dot_pos != 0xFF) {	//Split name
		--dot_pos;
	}

	if (dot_pos >= 8 || name_len - dot_pos > 3) {
		return TRUE;
	}

	for (i = 0; i < dot_pos; ++i) {
		if (name[i] == '.' || name[i] == '+' || name[i] == ',' || name[i] == ';' || name[i] == '=' || name[i] == '[' || name[i] == ']') {
			return TRUE;
		}
	}
	for (i = 0; i < name_len - dot_pos - 1; ++i) {
		if (name[i] == '+' || name[i] == ',' || name[i] == ';' || name[i] == '=' || name[i] == '[' || name[i] == ']') {
			return TRUE;
		}
	}

	return FALSE;
}

UINT16 file_set_time(time_type& time)
{
	return (time.hour << 11) | (time.min << 5) | time.s;
}

UINT16 file_set_date(time_type& date)
{
	return ((date.year - 1980) << 9) | (date.mon << 5) | date.day;
}

UINT8 file_chk_name(const UINT8* name)
{
	int i;
	unsigned char sum=0;
 
	for (i=11; i; i--)
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *name++;
	return sum;
}

/*****************************************************************/

UINT32 f_read(file_info &file, UINT8* buffer, UINT32 size, UINT32 offset)
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

/********************************************************************/

UINT32 f_write(file_info &file, UINT8* buffer, UINT32 size, UINT32 offset)
{
	UINT32 clust_len = file.fs->clust_offset;
	UINT32 clust = file.first_clust;
	UINT32 clust_num = offset / clust_len;
	UINT32 blank_offset = file.size;
	UINT32 part_len;
	UINT8 applyed = 0;
	UINT8 w_buffer[CLUST_SIZE] = {0};
	UINT32 i;

	if (offset > file.size) {	//append blank
		while (blank_offset != offset) {
			part_len = offset - blank_offset;
			if (part_len > clust_len) {
				part_len = clust_len;
			}
			f_write(file, w_buffer, part_len, blank_offset);
			blank_offset += part_len;
		}
	}

	for (i = 0; i < clust_num; ++i) {
		clust = get_next_clust(clust, *file.fs);
		offset -= clust_len;
	}
	read_clust(*file.fs, w_buffer, clust);	//Get file last clust data
	while (size) {
		part_len = clust_len - offset;
		if (part_len > size) {
			part_len = size;
		}

		mem_cpy(&w_buffer[offset], buffer, part_len);
		write_clust(*file.fs, w_buffer, clust);
		
		if (applyed) {
			--file.fs->free_clust;
		}
		clust = get_next_empty_clust(*file.fs);
		applyed = 1;
		offset = 0;
		size -= part_len;
		buffer += part_len;
	}

	file.fs->next_clust = clust;

	update_volume_info(*file.fs);

	return 0;
}

void write_clust(volume_info &volume, UINT8* buffer, UINT32 clust_num)
{
	write_disk(buffer, volume.sector_size, volume.root_offset + (clust_num - 2) * volume.clust_offset, volume.clust_size);
}

/***********************************************************************/

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

UINT32 get_next_empty_clust(volume_info& fs)
{
	UINT32 result;
	UINT32 point = 0;

	do {
		result = get_num(&fs.fat[point], uint32);
		point += 4;
		if (point >= fs.fat_size) {
			return 0xFFFFFFFF;
		}
	} while (result != CLUST_EMPTY);

	return result;
}

/*************************************************************/

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

void set_num(UINT32 val, UINT8* buffer, num_type type)
{
	switch(type) {
		case uint16:
			buffer[0] =  val & 0x00FF;
			buffer[1] = (val & 0xFF00) >> 8;
			break;
		case uint32:
			buffer[0] =  val & 0x000000FF;
			buffer[1] = (val & 0x0000FF00) >> 8;
			buffer[2] = (val & 0x00FF0000) >> 16;
			buffer[3] = (val & 0xFF000000) >> 24;
			break;
		default:
			break;
	}
}
