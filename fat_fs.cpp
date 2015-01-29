#include "fat_fs.h"
#include <Windows.h>

//Basic disk IO function
bool readDisk(UINT8* buffer, int sector_size, int offset, int count)
{
	HANDLE hDisk = CreateFileW(L"\\\\.\\F:",
								GENERIC_READ|GENERIC_WRITE,
								FILE_SHARE_READ|FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								NULL,
								NULL) ; 
	SetFilePointer(hDisk , offset , 0 , FILE_BEGIN) ;
	memSet(buffer, 0xee, sector_size * count * sizeof(UINT8));

	DWORD dwReadLength = 0 ;
	ReadFile(hDisk , buffer , sector_size * count , &dwReadLength , NULL) ;

	CloseHandle(hDisk);
	return true;
}

bool writeDisk(UINT8* buffer, int sector_size, int offset, int count)
{
	return false;
}

//Help func
UINT32 memSet(void* buffer, UINT8 val, UINT32 length)
{
	for (; length > 0; --length) {
		((UINT8*)buffer)[length - 1] = val;
	}
	return length;
}

UINT32 memCpy(void* target, const void* source, UINT32 length)
{
	for (; length > 0; --length) {
		((UINT8*)target)[length - 1] = ((UINT8*)source)[length - 1];	
	}
	return length;
}

UINT32 strLen(const UINT8* str)
{
	UINT32 len = 0;

	while (*str++) {
		++len;
	}

	return len;
}

UINT32 strCmp(UINT8* str1, const UINT8* str2)
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

UINT32 strFindChar(const UINT8* str, UINT8 c, UINT32 offset)
{
	UINT32 len = strLen(str);

	for (; offset < len; ++offset) {
		if (str[offset] == c) {
			return offset;
		}
	}

	return 0xFFFFFFFF;
}

//init function
void initVolume(VolumeInfo* volume)
{

	UINT8 DBR_buffer[SECTOR_SIZE];
	readDisk(DBR_buffer, SECTOR_SIZE, 0, 1);

	volume->sector_size = (UINT16)getNum(&DBR_buffer[OFFSET_SECTOR_SIZE], uint16);
	volume->clust_size = DBR_buffer[OFFSET_CLUST_SIZE];
	volume->clust_offset = volume->sector_size * volume->clust_size;

	volume->fat_offset = volume->sector_size * getNum(&DBR_buffer[OFFSET_REV_SECTOR], uint16);
	volume->root_clust = getNum(&DBR_buffer[OFFSET_ROOT_CLUST], uint32);
	volume->root_offset = volume->sector_size * 
					(getNum(&DBR_buffer[OFFSET_REV_SECTOR], uint16) + 
					 getNum(&DBR_buffer[OFFSET_FAT32_SECTOR], uint32) * 2 + 
					 (volume->root_clust - 2) * volume->clust_size);		//root'offset = sector_size * (rev_sector + 2 * fat_sector + (root_clust - 2) * clust_sector)

	readDisk(volume->fat, volume->sector_size, volume->fat_offset, FAT_SIZE / volume->sector_size);
	volume->fat_part_num = 0;
	volume->fat_size = getNum(&DBR_buffer[OFFSET_FAT32_SECTOR], uint32) * volume->sector_size;

	//Get volume name;
	memCpy(volume->name, &DBR_buffer[OFFSET_VOLUME_NAME], LEN_VOLUME_NAME);

	//---------------------Set root dir------------------------
	root->name[0] = '\\';
	root->name[1] = '\0';
	root->name_len = strLen(root->name);
	root->attri = FILE_ATTRI_DIR | FILE_ATTRI_SYSTEM | FILE_ATTRI_HIDDEN;
	root->offset = volume->root_offset;
	root->first_clust = getNum(&DBR_buffer[OFFSET_ROOT_CLUST], uint32);
	root->fs = volume;
	root->size = volume->clust_offset * (getLastClustNum(*volume, root->first_clust) + 1);

	volume->root = root;

	//---------------------Read FSInfo sector------------------
	readDisk(DBR_buffer, volume->sector_size, getNum(&DBR_buffer[OFFSET_FSINFO_SECTOR], uint16) * volume->sector_size, 1);

	//Get free space 
	volume->free_clust = getNum(&DBR_buffer[OFFSET_FSINFO_FREE], uint32);
	volume->next_clust = getNum(&DBR_buffer[OFFSET_FSINFO_NEXT], uint32);}

void updateVolume(VolumeInfo& volume)
{
	UINT8 buffer[SECTOR_SIZE];
	UINT32 v_info_offset;

	readDisk(buffer, SECTOR_SIZE, 0, 1);
	v_info_offset = getNum(&buffer[OFFSET_FSINFO_SECTOR], uint16) * volume.sector_size;
	readDisk(buffer, volume.sector_size, v_info_offset, 1);

	setNum(volume.free_clust, &buffer[OFFSET_FSINFO_FREE], uint32);
	setNum(volume.next_clust, &buffer[OFFSET_FSINFO_NEXT], uint32);

	writeDisk(buffer, volume.sector_size, v_info_offset, 1);
}

//interface function
FileInfo f_open(const CHAR* file_name, UINT8 mode)
{
	FileInfo result;
	f_open(*result, file_name, mode);
	return result;
}

UINT8 f_open(FileInfo* file, const CHAR* file_name, UINT8 mode)
{

}

UINT8 f_read(FileInfo& file, UINT8* buffer, UINT32 offset, UINT32 size)
{

}

UINT8 f_write(FileInfo& file, UINT8* buffer, UINT32 offset, UINT32 size)
{

}

//dir read functions
UINT8 getNextFile(DirInfo& dir);
UINT8 setDirPoint(DirInfo* dir, UINT32 pos);
UINT8 getFileCount(DirInfo& dir);
UINT8 followPath(DirInfo begin_dir, const CHAR* path, FileInfo* target)
{
	CHAR* p = path;
	CHAR path_part[NAME_MAX_LEN] = {0};
	UINT8 i = 0;

	if (*p == '/' || *p == '\\') {		//start from root
		begin_dir = *(DirInfo*)begin_dir.base_info.fs->root;
		++p;
	}

	while (*p) {
		i = 0;
		while (*p != '/' && *p != '\\' && *p) {
			path_part[i++] = *p++;
		}

		p++;	//jump '/'

		readFile(begin_dir, *target, path_part);
	}
}

UINT8 file2dir(DirInfo* dir, FileInfo& file)
{

}

UINT8 readFile(DirInfo& dir, FileInfo* file, const CHAR* filename)
{

}

//clust operate
bool readClust(VolumeInfo& volume, UINT32 clust, UINT8* buffer, UINT32 size);
bool writeClust(VolumeInfo& volume, UINT32 clust, UINT8* buffer, UINT32 size);
UINT32 getNextClust(VolumeInfo& volume, UINT32 now_clust);
UINT32 getLastClustNum(VolumeInfo& volume, UINT32 now_clust);
UINT32 getEmptyClust(VolumeInfo& volume);

//Win32 number convet
UINT32 getNum(UINT8* buffer, UINT8 num_bit_len)
{
	switch(num_bit_len) {
		case 16:
			return (UINT16)(buffer[0] | (buffer[1] << 8));
			break;
		case 32:
			return (UINT32)(buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24));
			break;
		default:
			return 0xFFFFFFFF;
			break;
	}
}

void setNum(UINT32 val, UINT8* buffer, UINT8 num_bit_len)
{
	switch(num_bit_len) {
		case 16:
			buffer[0] =  val & 0x00FF;
			buffer[1] = (val & 0xFF00) >> 8;
			break;
		case 32:
			buffer[0] =  val & 0x000000FF;
			buffer[1] = (val & 0x0000FF00) >> 8;
			buffer[2] = (val & 0x00FF0000) >> 16;
			buffer[3] = (val & 0xFF000000) >> 24;
			break;
		default:
			break;
	}
}
