/*
 * Load custom textures & robot data
 */

#include <string.h>
#include "gr.h"
#include "pstypes.h"
#include "piggy.h"
#include "textures.h"
#include "polyobj.h"
#include "weapon.h"
#include "digi.h"
#include "hash.h"
#include "u_mem.h"
#include "error.h"
#ifdef BITMAP_SELECTOR
#include "u_dpmi.h"
#endif
#include "custom.h"

//#define D2TMAP_CONV // used for testing

extern hashtable AllBitmapsNames;
extern hashtable AllDigiSndNames;

struct snd_info
{
	unsigned int length;
	ubyte *data;
};

typedef struct DiskBitmapHeader2 
{
	char name[8];
	ubyte dflags;
	ubyte width;
	ubyte height;
	ubyte hi_wh;
	ubyte flags;
	ubyte avg_color;
	int offset;
} __pack__ DiskBitmapHeader2;

typedef struct DiskBitmapHeader
{
	char name[8];
	ubyte dflags;
	ubyte width;
	ubyte height;
	ubyte flags;
	ubyte avg_color;
	int offset;
} __pack__ DiskBitmapHeader;

typedef struct DiskSoundHeader
{
	char name[8];
	int length;
	int data_length;
	int offset;
} __pack__ DiskSoundHeader;

struct custom_info
{
	int offset;
	int repl_idx; // 0x80000000 -> sound, -1 -> n/a
	unsigned int flags;
	int width, height;
};

grs_bitmap BitmapOriginal[MAX_BITMAP_FILES];
struct snd_info SoundOriginal[MAX_SOUND_FILES];

static void change_ext(char *filename, const char *newext, int filename_size)
{
	char *p;
	int len, extlen;
	len = strlen(filename);
	extlen = strlen(newext);

	if ((p = strrchr(filename, '.')))
	{
		len = p - filename;
		*p = 0;
	}

	if (len + extlen + 1 < filename_size)
	{
		strcat(filename, ".");
		strcat(filename, newext);
	}
}

int load_pig1(PHYSFS_file *f, int num_bitmaps, int num_sounds, int *num_custom, struct custom_info **ci)
{
	int data_ofs;
	int i;
	struct custom_info *cip;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	char name[15];

	*num_custom = 0;

	if ((unsigned int)num_bitmaps <= MAX_BITMAP_FILES) // <v1.4 pig?
	{
		PHYSFSX_fseek(f, 8, SEEK_SET);
		data_ofs = 8;
	}
	else if (num_bitmaps > 0 && num_bitmaps < PHYSFS_fileLength(f)) // >=v1.4 pig?
	{
		PHYSFSX_fseek(f, num_bitmaps, SEEK_SET);
		data_ofs = num_bitmaps + 8;
		num_bitmaps = PHYSFSX_readInt(f);
		num_sounds = PHYSFSX_readInt(f);
	}
	else
		return -1; // invalid pig file

	if ((unsigned int)num_bitmaps >= MAX_BITMAP_FILES || (unsigned int)num_sounds >= MAX_SOUND_FILES)
		return -1; // invalid pig file

	if (!(*ci = cip = d_malloc((num_bitmaps + num_sounds) * sizeof(struct custom_info))))
		return -1; // out of memory

	data_ofs += num_bitmaps * sizeof(DiskBitmapHeader) + num_sounds * sizeof(DiskSoundHeader);
	i = num_bitmaps;

	while (i--)
	{
		if (PHYSFS_read(f, &bmh, sizeof(DiskBitmapHeader), 1) < 1)
		{
			d_free(*ci);
			return -1;
		}

		memcpy( name, bmh.name, 8 );
		name[8] = 0;

		if (bmh.dflags & DBM_FLAG_ABM)
			sprintf(strchr(name, 0), "#%d", bmh.dflags & 63);

		cip->offset = bmh.offset + data_ofs;
		cip->repl_idx = hashtable_search(&AllBitmapsNames, name);
		cip->flags = bmh.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE);
		cip->width = bmh.width + ((bmh.dflags & DBM_FLAG_LARGE) ? 256 : 0);
		cip->height = bmh.height;
		cip++;
	}

	i = num_sounds;

	while (i--)
	{
		if (PHYSFS_read(f, &sndh, sizeof(DiskSoundHeader), 1) < 1)
		{
			d_free(*ci);
			return -1;
		}

		memcpy(name, sndh.name, 8);
		name[8] = 0;
		cip->offset = sndh.offset + data_ofs;
		cip->repl_idx = hashtable_search(&AllDigiSndNames, name) | 0x80000000;
		cip->width = sndh.length;
		cip++;
	}

	*num_custom = num_bitmaps + num_sounds;

	return 0;
}

int load_pog(PHYSFS_file *f, int pog_sig, int pog_ver, int *num_custom, struct custom_info **ci)
{
	int data_ofs;
	int num_bitmaps;
	int no_repl = 0;
	int i;
	struct custom_info *cip;
	DiskBitmapHeader2 bmh;

#ifdef D2TMAP_CONV
	int x, j, N_d2tmap;
	int *d2tmap = NULL;
	PHYSFS_file *f2 = NULL;
	if ((f2 = PHYSFSX_openReadBuffered("d2tmap.bin")))
	{
		N_d2tmap = PHYSFSX_readInt(f2);
		if ((d2tmap = d_malloc(N_d2tmap * sizeof(d2tmap[0]))))
			for (i = 0; i < N_d2tmap; i++)
				d2tmap[i] = PHYSFSX_readShort(f2);
		PHYSFS_close(f2);
	}
#endif

	*num_custom = 0;

	if (pog_sig == 0x47495050 && pog_ver == 2) /* PPIG */
		no_repl = 1;
	else if (pog_sig != 0x474f5044 || pog_ver != 1) /* DPOG */
		return -1; // no pig2/pog file/unknown version

	num_bitmaps = PHYSFSX_readInt(f);

	if (!(*ci = cip = d_malloc(num_bitmaps * sizeof(struct custom_info))))
		return -1; // out of memory

	data_ofs = 12 + num_bitmaps * sizeof(DiskBitmapHeader2);

	if (!no_repl)
	{
		i = num_bitmaps;

		while (i--)
			(cip++)->repl_idx = PHYSFSX_readShort(f);

		cip = *ci;
		data_ofs += num_bitmaps * 2;
	}

#ifdef D2TMAP_CONV
	if (d2tmap)
		for (i = 0; i < num_bitmaps; i++)
		{
			x = cip[i].repl_idx;
			cip[i].repl_idx = -1;

			for (j = 0; j < N_d2tmap; j++)
				if (x == d2tmap[j])
				{
					cip[i].repl_idx = Textures[j % NumTextures].index;
					break;
				}
		}
#endif

	i = num_bitmaps;

	while (i--)
	{
		if (PHYSFS_read(f, &bmh, sizeof(DiskBitmapHeader2), 1) < 1)
		{
			d_free(*ci);
			return -1;
		}

		cip->offset = bmh.offset + data_ofs;
		cip->flags = bmh.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE);
		cip->width = bmh.width + ((bmh.hi_wh & 15) << 8);
		cip->height = bmh.height + ((bmh.hi_wh >> 4) << 8);
		cip++;
	}

	*num_custom = num_bitmaps;

	return 0;
}

// load custom textures/sounds from pog/pig file
// returns 0 if ok, <0 on error
int load_pigpog(char *pogname)
{
	int num_custom;
	grs_bitmap *bmp;
	digi_sound *snd;
	ubyte *p;
	PHYSFS_file *f;
	struct custom_info *custom_info, *cip;
	int i, j, rc = -1;
	unsigned int x = 0;

	if (!(f = PHYSFSX_openReadBuffered((char *)pogname)))
		return -1; // pog file doesn't exist

	i = PHYSFSX_readInt(f);
	x = PHYSFSX_readInt(f);

	if (load_pog(f, i, x, &num_custom, &custom_info) && load_pig1(f, i, x, &num_custom, &custom_info))
	{
		if (num_custom)
			d_free(custom_info);
		PHYSFS_close(f);
		return rc;
	}

	cip = custom_info;
	i = num_custom;

	while (i--)
	{
		x = cip->repl_idx;
		if (cip->repl_idx >= 0)
		{
			PHYSFSX_fseek( f, cip->offset, SEEK_SET );

			if ( cip->flags & BM_FLAG_RLE )
				j = PHYSFSX_readInt(f);
			else
				j = cip->width * cip->height;

			if (!(p = d_malloc(j)))
			{
				if (num_custom)
					d_free(custom_info);
				PHYSFS_close(f);
				return rc;
			}

			bmp = &GameBitmaps[x];

			if (BitmapOriginal[x].bm_flags & 0x80) // already customized?
				gr_free_bitmap_data(bmp);
			else
			{
				// save original bitmap info
				BitmapOriginal[x] = *bmp;
				BitmapOriginal[x].bm_flags |= 0x80;
				if (GameBitmapOffset[x]) // from pig?
				{
					BitmapOriginal[x].bm_flags |= BM_FLAG_PAGED_OUT;
					BitmapOriginal[x].bm_data = (ubyte *)(size_t)GameBitmapOffset[x];
				}
			}

			GameBitmapOffset[x] = 0; // not in pig
			memset(bmp, 0, sizeof(*bmp));
			gr_init_bitmap (bmp, 0, 0, 0, cip->width, cip->height, cip->width, p);
			gr_set_bitmap_flags(bmp, cip->flags & 255);
			bmp->avg_color = cip->flags >> 8;

#ifdef BITMAP_SELECTOR
			if ( bmp->bm_selector )
				if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
					Error( "Error modifying selector base in custom.c\n" );
#endif
			if ( cip->flags & BM_FLAG_RLE )
			{
				int *ip = (int *)p;
				*ip = j;
				p += 4;
				j -= 4;
			}

			if (PHYSFS_read(f, p, 1, j) < 1)
			{
				if (num_custom)
					d_free(custom_info);
				PHYSFS_close(f);
				return rc;
			}

		}
		else if ((cip->repl_idx + 1) < 0)
		{
			PHYSFSX_fseek( f, cip->offset, SEEK_SET );
			snd = &GameSounds[x & 0x7fffffff];

			if (!(p = d_malloc(j = cip->width)))
			{
				if (num_custom)
					d_free(custom_info);
				PHYSFS_close(f);
				return rc;
			}

			if (SoundOriginal[x & 0x7fffffff].length & 0x80000000)  // already customized?
				d_free(snd->data);
			else
			{
#ifdef ALLEGRO
				SoundOriginal[x & 0x7fffffff].length = snd->len | 0x80000000;
#else
				SoundOriginal[x & 0x7fffffff].length = snd->length | 0x80000000;
#endif
				SoundOriginal[x & 0x7fffffff].data = snd->data;
			}

#ifdef ALLEGRO
				snd->loop_end = snd->len = j;
#else
				snd->length = j;
#endif
			snd->data = p;

			if (PHYSFS_read(f, p, j, 1) < 1)
			{
				if (num_custom)
					d_free(custom_info);
				PHYSFS_close(f);
				return rc;
			}
		}
		cip++;
	}
	rc = 0;

	if (num_custom)
		d_free(custom_info);

	PHYSFS_close(f);

	return rc;
}

int read_d2_robot_info(PHYSFS_file *fp, robot_info *ri)
{
	int j, k;

	ri->model_num = PHYSFSX_readInt(fp);

	for (j = 0; j < MAX_GUNS; j++)
		PHYSFSX_readVector(&ri->gun_points[j], fp);
	for (j = 0; j < MAX_GUNS; j++)
		ri->gun_submodels[j] = PHYSFSX_readByte(fp);
	ri->exp1_vclip_num = PHYSFSX_readShort(fp);
	ri->exp1_sound_num = PHYSFSX_readShort(fp);
	ri->exp2_vclip_num = PHYSFSX_readShort(fp);
	ri->exp2_sound_num = PHYSFSX_readShort(fp);
	ri->weapon_type = PHYSFSX_readByte(fp);
	if (ri->weapon_type >= N_weapon_types)
	    ri->weapon_type = 0;
	/*ri->weapon_type2 =*/ PHYSFSX_readByte(fp);
	ri->n_guns = PHYSFSX_readByte(fp);
	ri->contains_id = PHYSFSX_readByte(fp);
	ri->contains_count = PHYSFSX_readByte(fp);
	ri->contains_prob = PHYSFSX_readByte(fp);
	ri->contains_type = PHYSFSX_readByte(fp);
	/*ri->kamikaze =*/ PHYSFSX_readByte(fp);
	ri->score_value = PHYSFSX_readShort(fp);
	/*ri->badass =*/ PHYSFSX_readByte(fp);
	/*ri->energy_drain =*/ PHYSFSX_readByte(fp);
	ri->lighting = PHYSFSX_readFix(fp);
	ri->strength = PHYSFSX_readFix(fp);
	ri->mass = PHYSFSX_readFix(fp);
	ri->drag = PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		ri->field_of_view[j] = PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		ri->firing_wait[j] = PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		/*ri->firing_wait2[j] =*/ PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		ri->turn_time[j] = PHYSFSX_readFix(fp);
#if 0 // not used in d1, removed in d2
	for (j = 0; j < NDL; j++)
		ri->fire_power[j] = PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		ri->shield[j] = PHYSFSX_readFix(fp);
#endif
	for (j = 0; j < NDL; j++)
		ri->max_speed[j] = PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		ri->circle_distance[j] = PHYSFSX_readFix(fp);
	for (j = 0; j < NDL; j++)
		ri->rapidfire_count[j] = PHYSFSX_readByte(fp);
	for (j = 0; j < NDL; j++)
		ri->evade_speed[j] = PHYSFSX_readByte(fp);
	ri->cloak_type = PHYSFSX_readByte(fp);
	ri->attack_type = PHYSFSX_readByte(fp);
	ri->see_sound = PHYSFSX_readByte(fp);
	ri->attack_sound = PHYSFSX_readByte(fp);
	ri->claw_sound = PHYSFSX_readByte(fp);
	/*ri->taunt_sound =*/ PHYSFSX_readByte(fp);
	ri->boss_flag = PHYSFSX_readByte(fp);
	/*ri->companion =*/ PHYSFSX_readByte(fp);
	/*ri->smart_blobs =*/ PHYSFSX_readByte(fp);
	/*ri->energy_blobs =*/ PHYSFSX_readByte(fp);
	/*ri->thief =*/ PHYSFSX_readByte(fp);
	/*ri->pursuit =*/ PHYSFSX_readByte(fp);
	/*ri->lightcast =*/ PHYSFSX_readByte(fp);
	/*ri->death_roll =*/ PHYSFSX_readByte(fp);
	/*ri->flags =*/ PHYSFSX_readByte(fp);
	/*ri->pad[0] =*/ PHYSFSX_readByte(fp);
	/*ri->pad[1] =*/ PHYSFSX_readByte(fp);
	/*ri->pad[2] =*/ PHYSFSX_readByte(fp);
	/*ri->deathroll_sound =*/ PHYSFSX_readByte(fp);
	/*ri->glow =*/ PHYSFSX_readByte(fp);
	/*ri->behavior =*/ PHYSFSX_readByte(fp);
	/*ri->aim =*/ PHYSFSX_readByte(fp);

	for (j = 0; j < MAX_GUNS + 1; j++)
	{
		for (k = 0; k < N_ANIM_STATES; k++)
		{
			ri->anim_states[j][k].n_joints = PHYSFSX_readShort(fp);
			ri->anim_states[j][k].offset = PHYSFSX_readShort(fp);
		}
	}
	ri->always_0xabcd = PHYSFSX_readInt(fp);

	return 1;
}

void load_hxm(char *hxmname)
{
	unsigned int repl_num;
	int i;
	PHYSFS_file *f;
	int n_items;

	if (!(f = PHYSFSX_openReadBuffered((char *)hxmname)))
		return; // hxm file doesn't exist

	if (PHYSFSX_readInt(f) != 0x21584d48) /* HMX! */
	{
		PHYSFS_close(f); // invalid hxm file
		return;
	}

	if (PHYSFSX_readInt(f) != 1)
	{
		PHYSFS_close(f); // unknown version
		return;
	}

	// read robot info
	if ((n_items = PHYSFSX_readInt(f)) != 0)
	{
		for (i = 0; i < n_items; i++)
		{
			repl_num = PHYSFSX_readInt(f);

			if (repl_num >= MAX_ROBOT_TYPES)
			{
				PHYSFSX_fseek(f, 480, SEEK_CUR); /* sizeof d2_robot_info */
			}
			else
			{
				if (!(read_d2_robot_info(f, &Robot_info[repl_num])))
				{
					PHYSFS_close(f);
					return;
				}
			}
		}
	}

	// read joint positions
	if ((n_items = PHYSFSX_readInt(f)) != 0)
	{
		for (i = 0; i < n_items; i++)
		{
			repl_num = PHYSFSX_readInt(f);

			if (repl_num >= MAX_ROBOT_JOINTS)
				PHYSFSX_fseek(f, sizeof(jointpos), SEEK_CUR);
			else
			{
				if (PHYSFS_read(f, &Robot_joints[repl_num], sizeof(jointpos), 1) < 1)
				{
					PHYSFS_close(f);
					return;
				}
			}
		}
	}

	// read polygon models
	if ((n_items = PHYSFSX_readInt(f)) != 0)
	{
		for (i = 0; i < n_items; i++)
		{
			polymodel *pm;

			repl_num = PHYSFSX_readInt(f);
			if (repl_num >= MAX_POLYGON_MODELS)
			{
				PHYSFSX_readInt(f); // skip n_models
				PHYSFSX_fseek(f, 734 - 8 + PHYSFSX_readInt(f) + 8, SEEK_CUR);
			}
			else
			{
				pm = &Polygon_models[repl_num];
				if (pm->model_data)
					d_free(pm->model_data);

				if (PHYSFS_read(f, pm, sizeof(polymodel), 1) < 1)
				{
					pm->model_data = NULL;
					PHYSFS_close(f);
					return;
				}

				if (!(pm->model_data = d_malloc(pm->model_data_size)))
				{
					PHYSFS_close(f);
					return;
				}

				if (PHYSFS_read(f, pm->model_data, pm->model_data_size, 1) < 1)
				{
					pm->model_data = NULL;
					PHYSFS_close(f);
					return;
				}

				Dying_modelnums[repl_num] = PHYSFSX_readInt(f);
				Dead_modelnums[repl_num] = PHYSFSX_readInt(f);
			}
		}
	}

	// read object bitmaps
	if ((n_items = PHYSFSX_readInt(f)) != 0)
	{
		for (i = 0; i < n_items; i++)
		{
			repl_num = PHYSFSX_readInt(f);
			if (repl_num >= MAX_OBJ_BITMAPS)
				PHYSFSX_fseek(f, 2, SEEK_CUR);
			else
				ObjBitmaps[repl_num].index = PHYSFSX_readShort(f);
		}
	}

	PHYSFS_close(f);
}

// undo customized items
void custom_remove()
{
	int i;
	grs_bitmap *bmo = BitmapOriginal;
	grs_bitmap *bmp = GameBitmaps;

	for (i = 0; i < MAX_BITMAP_FILES; bmo++, bmp++, i++)
		if (bmo->bm_flags & 0x80)
		{
			gr_free_bitmap_data(bmp);
			*bmp = *bmo;

			if (bmo->bm_flags & BM_FLAG_PAGED_OUT)
			{
				GameBitmapOffset[i] = (int)(size_t)BitmapOriginal[i].bm_data;
				gr_set_bitmap_flags(bmp, BM_FLAG_PAGED_OUT);
				gr_set_bitmap_data(bmp, Piggy_bitmap_cache_data);
			}
			else
			{
				gr_set_bitmap_flags(bmp, bmo->bm_flags & 0x7f);
#ifdef BITMAP_SELECTOR
				if ( bmp->bm_selector )
					if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
						Error( "Error modifying selector base in custom.c\n" );
#endif

			}
			bmo->bm_flags = 0;
		}
	for (i = 0; i < MAX_SOUND_FILES; i++)
		if (SoundOriginal[i].length & 0x80000000)
		{
			d_free(GameSounds[i].data);
			GameSounds[i].data = SoundOriginal[i].data;
#ifdef ALLEGRO
			GameSounds[i].len = SoundOriginal[i].length & 0x7fffffff;
#else
			GameSounds[i].length = SoundOriginal[i].length & 0x7fffffff;
#endif
			SoundOriginal[i].length = 0;
		}
}

void load_custom_data(char *level_name)
{
	char custom_file[64];

	custom_remove();
	strncpy(custom_file, level_name, 63);
	custom_file[63] = 0;
	change_ext(custom_file, "pg1", sizeof(custom_file));
	load_pigpog(custom_file);
	change_ext(custom_file, "dtx", sizeof(custom_file));
	load_pigpog(custom_file);
	change_ext(custom_file, "hx1", sizeof(custom_file));
	load_hxm(custom_file);
}

void custom_close()
{
	custom_remove();
}
