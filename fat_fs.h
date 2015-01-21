#ifndef _FAT_FS_H_
#define _FAT_FS_H_

#define SECTOR_SIZE 512
#define CLUST_SIZE	0x2000
#define DIR_LIST_LEN	10

#define OFFSET_JUMP			0x00
#define OFFSET_OEM_NAME		0x03
#define OFFSET_SECTOR_SIZE	0x0B
#define OFFSET_CLUST_SIZE	0x0D
#define OFFSET_REV_SECTOR	0x0E
#define OFFSET_FAT_NUM		0x10
#define OFFSET_MAX_ROOT		0x11
#define OFFSET_SECTOR_NUM1	0x13
#define OFFSET_DISK_TYPE	0x15	
#define OFFSET_FAT16_SECTOR	0x16
#define OFFSET_TRACK_SECTOR	0x18
#define OFFSET_HEAD_NUM		0x1A
#define OFFSET_HIDE_SECTOR	0x1C
#define OFFSET_SECTOR_NUM2	0x20
#define OFFSET_FAT32_SECTOR	0x24
#define OFFSET_FAT16_DRIVE_NUM	0x24
#define OFFSET_FAT16_NOW_HEAD	0x25
#define OFFSET_FAT16_SIGN	0x26
#define OFFSET_FAT16_ID		0x27
#define OFFSET_FLAGS		0x28
#define OFFSET_VER			0x2A
#define OFFSET_ROOT_CLUST	0x2C
#define OFFSET_OTHER_FS_DISK_NAME	0x2B
#define OFFSET_FSINFO_SECTOR	0x30
#define OFFSET_START_BCK	0x32
#define OFFSET_REV			0x34
#define OFFSET_FS_NAME		0x36
#define OFFSET_OS_BOOT_CODE	0x3E
#define OFFSET_BIOS_ID		0x40
#define OFFSET_UNUSE		0x41
#define OFFSET_MARK			0x42
#define OFFSET_VOLUME_ID	0x43
#define OFFSET_VOLUME_NAME	0x47
#define OFFSET_FATFS_TYPE	0x52
#define OFFSET_EOF			0x1FE
#define LEN_VOLUME_NAME		11

#define OFFSET_FSINFO_FREE	0x1E8
#define OFFSET_FSINFO_NEXT	0x1EC

#define DIR_OFFSET_NAME		0x00
#define DIR_OFFSET_EX_NAME	0x08
#define DIR_OFFSET_ATTRI	0x0B
#define DIR_OFFSET_NT_REV	0x0C
#define DIR_OFFSET_CREATE_MS	0x0D
#define DIR_OFFSET_CREATE_TIME	0x0E
#define DIR_OFFSET_CREATE_DATE	0x10
#define DIR_OFFSET_RESENT_DATE	0x12
#define DIR_OFFSET_F_CLUST_HIGH	0x14
#define DIR_OFFSET_CHANGE_TIME	0x16
#define DIR_OFFSET_CHANGE_DATE	0x18
#define DIR_OFFSET_F_CLUST_LOW	0x1A
#define DIR_OFFSET_FILE_SIZE	0x1C
#define DIR_VAL_EMPTY			0x00
#define DIR_VAL_DELETED			0xE5
#define DIR_VAL_LAST_NAME_BLOCK	0x40
#define DIR_VAL_BLOCK_NAME_LEN	0x0D
#define DIR_VAL_NAME_BLOCK_MASK	0x2F

#define CLUST_EMPTY			0x00000000
#define CLUST_REV1			0x00000001
#define CLUST_REV2			0x0FFFFFF0
#define CLUST_BAD			0x0FFFFFF7
#define CLUST_LAST			0x0FFFFFF8

#define FILE_ATTRI_READONLY	0x01
#define FILE_ATTRI_HIDDEN	0x02
#define FILE_ATTRI_SYSTEM	0x04
#define FILE_ATTRI_VOLUME	0x08
#define FILE_ATTRI_DIR		0x10
#define FILE_ATTRI_DOC		0x20
#define FILE_ATTRI_DEVICE	0x40
#define FILE_ATTRI_UNUSE	0x80
#define FILE_ATTRI_LONGNAME	0x0F

#define FILE_VAL_ALL		0xFFFFFFFF

#define RESULT_OK			0x00
#define RESULT_WRONG_NAME	0xFC
#define RESULT_NOT_FOUND	0xFD
#define RESULT_WRONG_PATH	0xFE

#define NAME_MAX_LEN	255
#define FAT_SIZE		512

#define TRUE	1
#define FALSE	0

typedef unsigned __int64 UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

enum num_type
{
	uint32,
	uint16
};

struct time_type
{
	UINT16	ms;
	UINT8	s;
	UINT8	min;
	UINT8	hour;

	UINT8	day;
	UINT8	mon;
	UINT16	year;
};

struct volume_info {
	UINT8 name[LEN_VOLUME_NAME];

	UINT32 fat_offset;
	UINT32 root_offset;
	UINT32 root_clust;
	void* root;

	UINT16 sector_size;
	UINT8 clust_size;

	UINT32 clust_offset;
	
	UINT8 fat[FAT_SIZE];
	UINT32 fat_part_num;
	UINT64 fat_size;

	UINT32 free_clust; 
	UINT32 next_clust;
};

struct file_info
{
	volume_info*	fs;
	UINT8		name[NAME_MAX_LEN];
	UINT32		name_len;
	UINT8		name_chk;
	UINT8		attri;
	time_type	create_time;
	time_type	resent_time;
	time_type	change_time;
	UINT32	size;
	UINT32	first_clust;
	UINT32	offset;

	file_info* dir;
};

//Basic disk IO function
bool read_disk(UINT8* buffer, int sector_size = 512, int offset = 0, int count = 1);
bool write_disk(UINT8* buffer, int sector_size, int offset, int count);

//Help func
UINT32 mem_set(void* buffer, UINT8 val, UINT32 length); 
UINT32 mem_cpy(void* target, const void* source, UINT32 length);
UINT32 str_len(const UINT8* str);
UINT32 str_cmp(UINT8* str1, const UINT8* str2);
UINT32 str_find_c(const UINT8* str, UINT8 c, UINT32 offset);

//Get volume basic information
void init_volume(volume_info* volume, file_info* root);
void update_volume_info(volume_info& volume);

//Get dir and its file list
file_info f_open(file_info& now_dir, const UINT8* file_name);
UINT8 f_open(file_info* file, file_info& now_dir, const UINT8* file_name);

UINT8 get_file_dir(file_info* target, file_info& start, const UINT8* path);
UINT8 get_file_info(file_info* file, file_info& dir, const UINT8* file_name);

UINT32 get_file_list(file_info& dir, file_info* file_list, UINT32 start = 0, UINT32 max = 0xFFFFFFFF);
UINT32 get_file_num(file_info& dir);
void get_file_info_base(file_info* info, UINT8* buffer);
void file_get_time(time_type* time_info, UINT16 time_date);
void file_get_date(time_type* time_info, UINT16 date_date);

file_info create_new_file(file_info& dir, const UINT8* file_name);

UINT8 set_file_info_base(file_info&, UINT8*buffer);
//UINT8 file_get_short_name(UINT8* short_name, const UINT8* name);
UINT8 file_is_long_name(const UINT8* name);
UINT16 file_set_time(time_type& time);
UINT16 file_set_date(time_type& date);

UINT8 file_chk_name(const UINT8* name);

//Read data of a file
UINT32 f_read(file_info &file, UINT8* buffer, UINT32 size = FILE_VAL_ALL, UINT32 offset = 0);
void read_clust(volume_info &volume, UINT8* buffer, UINT32 clust_num);

//Write data of a file
UINT32 f_write(file_info &file, UINT8* buffer, UINT32 size, UINT32 offset = 0);
void write_clust(volume_info &volume, UINT8* buffer, UINT32 clust_num);

//Get data of next clust 
UINT32 get_next_clust(UINT32 now_clust, volume_info& fs);
UINT32 get_last_clust_num(UINT32 now_clust, volume_info& fs);
UINT32 get_next_empty_clust(volume_info& fs);

//Win32 number convet
UINT32 get_num(UINT8* buffer, num_type type);
void set_num(UINT32 val, UINT8* buffer, num_type type);

#endif
