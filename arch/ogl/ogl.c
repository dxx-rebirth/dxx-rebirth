/* $Id: ogl.c,v 1.1.1.1 2006/03/17 19:53:36 zicodxx Exp $ */
/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

//#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#include "internal.h"
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "3d.h"
#include "piggy.h"
#include "../../3d/globvars.h"
#include "error.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "mono.h"
#include "u_mem.h"
#ifdef HAVE_LIBPNG
#include "pngfile.h"
#endif

#include "segment.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "laser.h"
#include "player.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"

//change to 1 for lots of spew.
#if 0
#define glmprintf(a) mprintf(a)
#else
#define glmprintf(a)
#endif

#ifndef M_PI
#define M_PI 3.14159
#endif

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__)) || defined(__sun__) || defined(macintosh)
#define cosf(a) cos(a)
#define sinf(a) sin(a)
#endif

#if defined(_WIN32) // HACK - REMOVE ASAP
#undef GL_EXT_paletted_texture
#endif

unsigned char *ogl_pal=gr_palette;

int GL_texmagfilt=GL_NEAREST;
int GL_texminfilt=GL_NEAREST;
float GL_texanisofilt = 0;
int GL_needmipmaps=0;

int last_width=-1,last_height=-1;
int GL_TEXTURE_2D_enabled=-1;
int GL_texclamp_enabled=-1;

extern int gr_badtexture;
int r_texcount = 0, r_cachedtexcount = 0;
int ogl_alttexmerge=1;//merge textures by just printing the seperate textures?
int ogl_rgba_internalformat = GL_RGBA8;
int ogl_rgb_internalformat = GL_RGB8;
int ogl_intensity4_ok=1;
int ogl_luminance4_alpha4_ok=1;
int ogl_rgba2_ok=1;
int ogl_readpixels_ok=1;
int ogl_gettexlevelparam_ok=1;
#ifdef GL_ARB_multitexture
int ogl_arb_multitexture_ok=0;
#endif
#ifdef GL_SGIS_multitexture
int ogl_sgis_multitexture_ok=0;
#endif
int ogl_nv_texture_env_combine4_ok = 0;
#ifdef GL_NV_register_combiners
int ogl_nv_register_combiners_ok = 0;
#endif
int ogl_ext_texture_filter_anisotropic_ok = 0;
#ifdef GL_EXT_paletted_texture
int ogl_shared_palette_ok = 0;
int ogl_paletted_texture_ok = 0;
#endif

int sphereh=0;
int cross_lh[2]={0,0};
int primary_lh[3]={0,0,0};
int secondary_lh[5]={0,0,0,0,0};
int bNoDepthTest=0;
extern int glalpha_effects;
/*int lastbound=-1;

#define OGL_BINDTEXTURE(a) if(gr_badtexture>0) glBindTexture(GL_TEXTURE_2D, 0);\
	else if(a!=lastbound) {glBindTexture(GL_TEXTURE_2D, a);lastbound=a;}*/
#define OGL_BINDTEXTURE(a) if(gr_badtexture>0) glBindTexture(GL_TEXTURE_2D, 0);\
	else glBindTexture(GL_TEXTURE_2D, a);


ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];
int ogl_texture_list_cur;
int bLegacyZBuf = 0;

/* some function prototypes */

//#define OGLTEXBUFSIZE (1024*1024*4)
//#define OGLTEXBUFSIZE (4096*4096*4)
#define GL_TEXTURE0_ARB 0x84C0
extern GLubyte *pixels;
extern GLubyte *texbuf;
//void ogl_filltexbuf(unsigned char *data,GLubyte *texp,int width,int height,int  twidth,int theight);
void ogl_filltexbuf(unsigned char *data, GLubyte *texp, int truewidth, int width, int height, int dxo, int dyo, int twidth, int theight, int type, int bm_flags, int data_format);
void ogl_loadbmtexture(grs_bitmap *bm);
//void ogl_loadtexture(unsigned char * data, int width, int height,int dxo,intdyo , int *texid,float *u,float *v,char domipmap,float prio);
int ogl_loadtexture(unsigned char *data, int dxo, int dyo, ogl_texture *tex, int bm_flags, int data_format);
void ogl_freetexture(ogl_texture *gltexture);
void ogl_do_palfx(void);

void ogl_init_texture_stats(ogl_texture* t){
	t->prio=0.3;//default prio
	t->lastrend=0;
	t->numrend=0;
}
void ogl_init_texture(ogl_texture* t, int w, int h, int flags)
{
	t->handle = 0;
	if (flags & OGL_FLAG_NOCOLOR)
	{
		// use GL_INTENSITY instead of GL_RGB
		if (flags & OGL_FLAG_ALPHA)
		{
			if (ogl_intensity4_ok)
			{
				t->internalformat = GL_INTENSITY4;
				t->format = GL_LUMINANCE;
			}
			else if (ogl_luminance4_alpha4_ok)
			{
				t->internalformat = GL_LUMINANCE4_ALPHA4;
				t->format = GL_LUMINANCE_ALPHA;
			}
			else if (ogl_rgba2_ok)
			{
				t->internalformat = GL_RGBA2;
				t->format = GL_RGBA;
			}
			else
			{
				t->internalformat = ogl_rgba_internalformat;
				t->format = GL_RGBA;
			}
		}
		else
		{
			// there are certainly smaller formats we could use here, but nothing needs it ATM.
			t->internalformat = ogl_rgb_internalformat;
			t->format = GL_RGB;
		}
	}
	else
	{
		if (flags & OGL_FLAG_ALPHA)
		{
			t->internalformat = ogl_rgba_internalformat;
			t->format = GL_RGBA;
		}
		else
		{
			t->internalformat = ogl_rgb_internalformat;
			t->format = GL_RGB;
		}
	}
	t->wrapstate[0] = -1;
	t->wrapstate[1] = -1;
	t->lw = t->w = w;
	t->h = h;
	t->wantmip = flags & OGL_FLAG_MIPMAP;
	ogl_init_texture_stats(t);
}

void ogl_reset_texture(ogl_texture* t)
{
	ogl_init_texture(t, 0, 0, 0);
}

void ogl_reset_texture_stats_internal(void){
	int i;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++)
		if (ogl_texture_list[i].handle>0){
			ogl_init_texture_stats(&ogl_texture_list[i]);
		}
}
void ogl_init_texture_list_internal(void){
	int i;
	ogl_texture_list_cur=0;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++)
		ogl_reset_texture(&ogl_texture_list[i]);
}
void ogl_smash_texture_list_internal(void){
	int i;
	sphereh=0;
	memset(cross_lh,0,sizeof(cross_lh));
	memset(primary_lh,0,sizeof(primary_lh));
	memset(secondary_lh,0,sizeof(secondary_lh));
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		if (ogl_texture_list[i].handle>0){
			glDeleteTextures( 1, &ogl_texture_list[i].handle );
			ogl_texture_list[i].handle=0;
		}
		ogl_texture_list[i].wrapstate[0] = -1;
		ogl_texture_list[i].wrapstate[1] = -1;
	}
}
void ogl_vivify_texture_list_internal(void){
/*
   int i;
	ogl_texture* t;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		t=&ogl_texture_list[i];
		if (t->w>0){//erk, realised this can't be done since we'd need the texture bm_data too. hmmm.
			ogl_loadbmtexture(t);
	}
*/
}

ogl_texture* ogl_get_free_texture(void){
	int i;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		if (ogl_texture_list[ogl_texture_list_cur].handle<=0 && ogl_texture_list[ogl_texture_list_cur].w==0)
			return &ogl_texture_list[ogl_texture_list_cur];
		if (++ogl_texture_list_cur>=OGL_TEXTURE_LIST_SIZE)
			ogl_texture_list_cur=0;
	}
	Error("OGL: texture list full!\n");
//	return NULL;
}
int ogl_texture_stats(void){
	int used = 0, usedother = 0, usedidx = 0, usedrgb = 0, usedrgba = 0;
	int databytes = 0, truebytes = 0, datatexel = 0, truetexel = 0, i;
	int prio0=0,prio1=0,prio2=0,prio3=0,prioh=0;
//	int grabbed=0;
	ogl_texture* t;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		t=&ogl_texture_list[i];
		if (t->handle>0){
			used++;
			datatexel+=t->w*t->h;
			truetexel+=t->tw*t->th;
			databytes+=t->bytesu;
			truebytes+=t->bytes;
			if (t->prio<0.299)prio0++;
			else if (t->prio<0.399)prio1++;
			else if (t->prio<0.499)prio2++;
			else if (t->prio<0.599)prio3++;
			else prioh++;
			if (t->format == GL_RGBA)
				usedrgba++;
			else if (t->format == GL_RGB)
				usedrgb++;
			else if (t->format == GL_COLOR_INDEX)
				usedidx++;
			else
				usedother++;
		}
//		else if(t->w!=0)
//			grabbed++;
	}
	if (gr_renderstats)
	{
		GLint idx, r, g, b, a, dbl, depth;
		int res, colorsize, depthsize;

		res = SWIDTH * SHEIGHT;
		glGetIntegerv(GL_INDEX_BITS, &idx);
		glGetIntegerv(GL_RED_BITS, &r);
		glGetIntegerv(GL_GREEN_BITS, &g);
		glGetIntegerv(GL_BLUE_BITS, &b);
		glGetIntegerv(GL_ALPHA_BITS, &a);
		glGetIntegerv(GL_DOUBLEBUFFER, &dbl);
		dbl += 1;
		glGetIntegerv(GL_DEPTH_BITS, &depth);
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(31,0,0),-1 );
		colorsize = (idx * res * dbl) / 8;
		depthsize = res * depth / 8;
		gr_printf(5, GAME_FONT->ft_h * 14 + 3 * 14, "%i(%i,%i,%i,%i) %iK(%iK wasted) (%i postcachedtex)", used, usedrgba, usedrgb, usedidx, usedother, truebytes / 1024, (truebytes - databytes) / 1024, r_texcount - r_cachedtexcount);
		gr_printf(5, GAME_FONT->ft_h * 15 + 3 * 15, "%ibpp(r%i,g%i,b%i,a%i)x%i=%iK depth%i=%iK", idx, r, g, b, a, dbl, colorsize / 1024, depth, depthsize / 1024);
		gr_printf(5, GAME_FONT->ft_h * 16 + 3 * 16, "total=%iK", (colorsize + depthsize + truebytes) / 1024);
	}
//	glmprintf((0,"ogl tex stats: %i(%i,%i|%i,%i,%i,%i,%i) %i(%i)b (%i(%i)wasted)\n",used,usedrgba,usedl4a4,prio0,prio1,prio2,prio3,prioh,truebytes,truetexel,truebytes-databytes,truetexel-datatexel));
	return truebytes;
}
int ogl_mem_target=-1;
void ogl_clean_texture_cache(void){
	ogl_texture* t;
	int i,bytes;
	int time=120;
	
	if (ogl_mem_target<0){
		if (gr_renderstats)
			ogl_texture_stats();
		return;
	}
	
	bytes=ogl_texture_stats();
	while (bytes>ogl_mem_target){
		for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
			t=&ogl_texture_list[i];
			if (t->handle>0){
				if (t->lastrend+f1_0*time<GameTime){
					ogl_freetexture(t);
					bytes-=t->bytes;
					if (bytes<ogl_mem_target)
						return;
				}
			}
		}
		if (time==0)
			Error("not enough mem?");
		time=time/2;
	}
	
}
void ogl_bindbmtex(grs_bitmap *bm){
	if (bm->gltexture==NULL || bm->gltexture->handle<=0)
		ogl_loadbmtexture(bm);
	OGL_BINDTEXTURE(bm->gltexture->handle);
	bm->gltexture->lastrend=GameTime;
	bm->gltexture->numrend++;
////	if (bm->gltexture->numrend==80 || bm->gltexture->numrend==4000 || bm->gltexture->numrend==80000){
//	if (bm->gltexture->numrend==100){
//		bm->gltexture->prio+=0.1;
////		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_PRIORITY,bm->gltexture->prio);
//		glPrioritizeTextures(1,&bm->gltexture->handle,&bm->gltexture->prio);
//	}
}
//gltexture MUST be bound first
void ogl_texwrap(ogl_texture *gltexture,int state)
{
	if (gltexture->wrapstate[active_texture_unit] != state || gltexture->numrend < 1)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
		gltexture->wrapstate[active_texture_unit] = state;
	}
}

//crude texture precaching
//handles: powerups, walls, weapons, polymodels, etc.
//it is done with the horrid do_special_effects kludge so that sides that have to be texmerged and have animated textures will be correctly cached.
//similarly, with the objects(esp weapons), we could just go through and cache em all instead, but that would get ones that might not even be on the level
//TODO: doors

void ogl_cache_polymodel_textures(int model_num)
{
	polymodel *po;
	int i;

	if (model_num < 0)
		return;
	po = &Polygon_models[model_num];
	for (i=0;i<po->n_textures;i++)  {
//		texture_list_index[i] = ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]];
		ogl_loadbmtexture(&GameBitmaps[ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index]);
	}
}
void ogl_cache_vclip_textures(vclip *vc){
	int i;
	for (i=0;i<vc->num_frames;i++){
		PIGGY_PAGE_IN(vc->frames[i]);
		ogl_loadbmtexture(&GameBitmaps[vc->frames[i].index]);
	}
}

void ogl_cache_vclipn_textures(int i)
{
	if (i >= 0 && i < VCLIP_MAXNUM)
		ogl_cache_vclip_textures(&Vclip[i]);
}

void ogl_cache_weapon_textures(int weapon_type)
{
	weapon_info *w;

	if (weapon_type < 0)
		return;
	w = &Weapon_info[weapon_type];
	ogl_cache_vclipn_textures(w->flash_vclip);
	ogl_cache_vclipn_textures(w->robot_hit_vclip);
	ogl_cache_vclipn_textures(w->wall_hit_vclip);
	if (w->render_type==WEAPON_RENDER_VCLIP)
		ogl_cache_vclipn_textures(w->weapon_vclip);
	else if (w->render_type == WEAPON_RENDER_POLYMODEL)
	{
		ogl_cache_polymodel_textures(w->model_num);
		ogl_cache_polymodel_textures(w->model_num_inner);
	}
}

void ogl_cache_level_textures(void)
{
	int seg,side,i;
	eclip *ec;
	short tmap1,tmap2;
	grs_bitmap *bm,*bm2;
	struct side *sidep;
	int max_efx=0,ef;
	
	ogl_reset_texture_stats_internal();//loading a new lev should reset textures
	
	for (i=0,ec=Effects;i<Num_effects;i++,ec++) {
		ogl_cache_vclipn_textures(Effects[i].dest_vclip);
		if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
			continue;
		if (ec->vc.num_frames>max_efx)
			max_efx=ec->vc.num_frames;
	}
	glmprintf((0,"max_efx:%i\n",max_efx));
	for (ef=0;ef<max_efx;ef++){
		for (i=0,ec=Effects;i<Num_effects;i++,ec++) {
			if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
				continue;
//			if (ec->vc.num_frames>max_efx)
//				max_efx=ec->vc.num_frames;
			ec->time_left=-1;
		}
		do_special_effects();

		for (seg=0;seg<Num_segments;seg++){
			for (side=0;side<MAX_SIDES_PER_SEGMENT;side++){
				sidep=&Segments[seg].sides[side];
				tmap1=sidep->tmap_num;
				tmap2=sidep->tmap_num2;
				if (tmap1<0 || tmap1>=NumTextures){
					glmprintf((0,"ogl_cache_level_textures %i %i %i %i\n",seg,side,tmap1,NumTextures));
					//				tmap1=0;
					continue;
				}
				PIGGY_PAGE_IN(Textures[tmap1]);
				bm = &GameBitmaps[Textures[tmap1].index];
				if (tmap2 != 0){
					PIGGY_PAGE_IN(Textures[tmap2&0x3FFF]);
					bm2 = &GameBitmaps[Textures[tmap2&0x3FFF].index];
					if (ogl_alttexmerge == 0 || (!OGL_SUPER_TRANSPARENT_OK && (bm2->bm_flags & BM_FLAG_SUPER_TRANSPARENT)))
						bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
					else {
						ogl_loadbmtexture(bm2);
					}
					//				glmprintf((0,"ogl_cache_level_textures seg %i side %i t1 %i t2 %x bm %p NT %i\n",seg,side,tmap1,tmap2,bm,NumTextures));
				}
				ogl_loadbmtexture(bm);
			}
		}
		glmprintf((0,"finished ef:%i\n",ef));
	}
	reset_special_effects();
	init_special_effects();
	{
//		int laserlev=1;

		// always have lasers, concs, flares.  Always shows player appearance, and at least concs are always available to disappear.
		ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[LASER_INDEX]);
		ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[CONCUSSION_INDEX]);
		ogl_cache_weapon_textures(FLARE_ID);
		ogl_cache_vclipn_textures(VCLIP_PLAYER_APPEARANCE);
		ogl_cache_vclipn_textures(VCLIP_POWERUP_DISAPPEARANCE);
		ogl_cache_polymodel_textures(Player_ship->model_num);
		ogl_cache_vclipn_textures(Player_ship->expl_vclip_num);

		for (i=0;i<Highest_object_index;i++){
			if(Objects[i].render_type==RT_POWERUP){
				ogl_cache_vclipn_textures(Objects[i].rtype.vclip_info.vclip_num);
				switch (Objects[i].id){
/*					case POW_LASER:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[LASER_INDEX]);
//						if (laserlev<4)
//							laserlev++;
						break;*/
					case POW_VULCAN_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[VULCAN_INDEX]);
						break;
					case POW_SPREADFIRE_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[SPREADFIRE_INDEX]);
						break;
					case POW_PLASMA_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[PLASMA_INDEX]);
						break;
					case POW_FUSION_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[FUSION_INDEX]);
						break;
/*					case POW_MISSILE_1:
					case POW_MISSILE_4:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[CONCUSSION_INDEX]);
						break;*/
					case POW_PROXIMITY_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[PROXIMITY_INDEX]);
						break;
					case POW_HOMING_AMMO_1:
					case POW_HOMING_AMMO_4:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[HOMING_INDEX]);
						break;
					case POW_SMARTBOMB_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[SMART_INDEX]);
						break;
					case POW_MEGA_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[MEGA_INDEX]);
						break;
				}
			}
			else if(Objects[i].render_type==RT_POLYOBJ){
				//printf("robot %i model %i rmodel %i\n", Objects[i].id, Objects[i].rtype.pobj_info.model_num, Robot_info[Objects[i].id].model_num);
				if (Objects[i].type == OBJ_ROBOT)
				{
					ogl_cache_vclipn_textures(Robot_info[Objects[i].id].exp1_vclip_num);
					ogl_cache_vclipn_textures(Robot_info[Objects[i].id].exp2_vclip_num);
					ogl_cache_weapon_textures(Robot_info[Objects[i].id].weapon_type);
				}
				if (Objects[i].rtype.pobj_info.tmap_override != -1)
					ogl_loadbmtexture(&GameBitmaps[Textures[Objects[i].rtype.pobj_info.tmap_override].index]);
				else
					ogl_cache_polymodel_textures(Objects[i].rtype.pobj_info.model_num);
			}
		}
	}
	glmprintf((0,"finished caching\n"));
	r_cachedtexcount = r_texcount;
}

int r_polyc,r_tpolyc,r_bitmapc,r_ubitmapc,r_ubitbltc,r_upixelc;
#define f2glf(x) (f2fl(x))

bool g3_draw_line(g3s_point *p0,g3s_point *p1)
{
	int c;
	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	glLineWidth(grd_curscreen->sc_w/640);
	glColor3f(PAL2Tr(c),PAL2Tg(c),PAL2Tb(c));
	glBegin(GL_LINES);
	glVertex3f(f2glf(p0->p3_vec.x),f2glf(p0->p3_vec.y),-f2glf(p0->p3_vec.z));
	glVertex3f(f2glf(p1->p3_vec.x),f2glf(p1->p3_vec.y),-f2glf(p1->p3_vec.z));
	glEnd();
	return 1;
}
void ogl_drawcircle2(int nsides,int type,float xsc,float xo,float ysc,float yo){
	int i;
	float ang;
	glBegin(type);
	for (i=0; i<nsides; i++) {
		ang = 2.0*M_PI*i/nsides;
		glVertex2f(cosf(ang)*xsc+xo,sinf(ang)*ysc+yo);
	}
	glEnd();
}
void ogl_drawcircle(int nsides,int type){
	int i;
	float ang;
	glBegin(type);
	for (i=0; i<nsides; i++) {
		ang = 2.0*M_PI*i/nsides;
		glVertex2f(cosf(ang),sinf(ang));
	}
	glEnd();
}
int circle_list_init(int nsides,int type,int mode) {
	int hand=glGenLists(1);
	glNewList(hand, mode);
	/* draw a unit radius circle in xy plane centered on origin */
	ogl_drawcircle(nsides,type);
	glEndList();
	return hand;
}
float bright_g[4]={	32.0/256,	252.0/256,	32.0/256};
float dark_g[4]={	32.0/256,	148.0/256,	32.0/256};
float darker_g[4]={	32.0/256,	128.0/256,	32.0/256};
void ogl_draw_reticle(int cross,int primary,int secondary){
	float scale=(float)Canvas_height/(float)grd_curscreen->sc_h;
	glPushMatrix();
//	glTranslatef(0.5,0.5,0);
	glTranslatef((grd_curcanv->cv_bitmap.bm_w/2+grd_curcanv->cv_bitmap.bm_x)/(float)last_width,1.0-(grd_curcanv->cv_bitmap.bm_h/2+grd_curcanv->cv_bitmap.bm_y)/(float)last_height,0);
	glScalef(scale/320.0,scale/200.0,scale);//the positions are based upon the standard reticle at 320x200 res.
	
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	if (!cross_lh[cross]){
		cross_lh[cross]=glGenLists(1);
		glNewList(cross_lh[cross], GL_COMPILE_AND_EXECUTE);
		glLineWidth(grd_curscreen->sc_w/640);
		glBegin(GL_LINES);
		//cross top left
		glColor3fv(darker_g);
		glVertex2f(-4.0,4.0);
		if (cross)
			glColor3fv(bright_g);
		else
			glColor3fv(dark_g);
		glVertex2f(-2.0,2.0);

		//cross bottom left
		glColor3fv(dark_g);
		glVertex2f(-3.0,-2.0);
		if (cross)
			glColor3fv(bright_g);
		glVertex2f(-2.0,-1.0);

		//cross top right
		glColor3fv(darker_g);
		glVertex2f(4.0,4.0);
		if (cross)
			glColor3fv(bright_g);
		else
			glColor3fv(dark_g);
		glVertex2f(2.0,2.0);

		//cross bottom right
		glColor3fv(dark_g);
		glVertex2f(3.0,-2.0);
		if (cross)
			glColor3fv(bright_g);
		glVertex2f(2.0,-1.0);

		glEnd();
		glEndList();
	}else
		glCallList(cross_lh[cross]);

//	if (Canvas_height>200)
//		glLineWidth(Canvas_height/(float)200);
	if (!primary_lh[primary]){
		primary_lh[primary]=glGenLists(1);
		glNewList(primary_lh[primary], GL_COMPILE_AND_EXECUTE);
		glColor3fv(dark_g);
		glLineWidth(grd_curscreen->sc_w/640);
		glBegin(GL_LINES);
		//left primary bar
		glVertex2f(-14.0,-8.0);
		glVertex2f(-8.0,-5.0);
		//right primary bar
		glVertex2f(14.0,-8.0);
		glVertex2f(8.0,-5.0);
		glEnd();
		if (primary==0)
			glColor3fv(dark_g);
		else
			glColor3fv(bright_g);
		//left upper
		ogl_drawcircle2(6,GL_POLYGON,1.5,-7.0,1.5,-5.0);
		//right upper
		ogl_drawcircle2(6,GL_POLYGON,1.5,7.0,1.5,-5.0);
		if (primary!=2)
			glColor3fv(dark_g);
		else
			glColor3fv(bright_g);
		//left lower
		ogl_drawcircle2(4,GL_POLYGON,1.0,-14.0,1.0,-8.0);
		//right lower
		ogl_drawcircle2(4,GL_POLYGON,1.0,14.0,1.0,-8.0);

		glEndList();
	}else
		glCallList(primary_lh[primary]);
//	if (Canvas_height>200)
//		glLineWidth(1);

	if (!secondary_lh[secondary]){
		secondary_lh[secondary]=glGenLists(1);
		glNewList(secondary_lh[secondary], GL_COMPILE_AND_EXECUTE);
		if (secondary<=2){
			//left secondary
			if (secondary!=1)
				glColor3fv(darker_g);
			else
				glColor3fv(bright_g);
			ogl_drawcircle2(8,GL_LINE_LOOP,2.0,-10.0,2.0,-1.0);
			//right secondary
			if (secondary!=2)
				glColor3fv(darker_g);
			else
				glColor3fv(bright_g);
			ogl_drawcircle2(8,GL_LINE_LOOP,2.0,10.0,2.0,-1.0);
		}else{
			//bottom/middle secondary
			if (secondary!=4)
				glColor3fv(darker_g);
			else
				glColor3fv(bright_g);
			ogl_drawcircle2(8,GL_LINE_LOOP,2.0,0.0,2.0,-7.0);
		}
		glEndList();
	}else
		glCallList(secondary_lh[secondary]);

	glPopMatrix();
}
int g3_draw_sphere(g3s_point *pnt,fix rad){
	int c;
	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE); // ZICO - to draw sheres on automap correctly
//	glPointSize(f2glf(rad));
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
//	glBegin(GL_POINTS);
//	glVertex3f(f2glf(pnt->p3_vec.x),f2glf(pnt->p3_vec.y),-f2glf(pnt->p3_vec.z));
//	glEnd();
	glPushMatrix();
	glTranslatef(f2glf(pnt->p3_vec.x),f2glf(pnt->p3_vec.y),-f2glf(pnt->p3_vec.z));
	rad/=((float)grd_curcanv->cv_bitmap.bm_w/grd_curcanv->cv_bitmap.bm_h);
	glScalef(f2glf(rad),f2glf(rad*((float)grd_curcanv->cv_bitmap.bm_w/grd_curcanv->cv_bitmap.bm_h)),f2glf(rad));
	if (!sphereh) sphereh=circle_list_init(20,GL_POLYGON,GL_COMPILE_AND_EXECUTE);
	else glCallList(sphereh);
	glPopMatrix();
	return 0;

}
int gr_ucircle(fix xc1, fix yc1, fix r1)
{
	int c;
	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glPushMatrix();
	glTranslatef(
	             (f2fl(xc1) + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width,
	             1.0 - (f2fl(yc1) + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height,0);
	glScalef(f2fl(r1) / last_width, f2fl(r1) / last_height, 1.0);
	ogl_drawcircle(10 + 2 * (int)(M_PI * f2fl(r1) / 19), GL_LINE_LOOP);
	glPopMatrix();
	return 0;
}
int gr_circle(fix xc1,fix yc1,fix r1){
	return gr_ucircle(xc1,yc1,r1);
}

bool g3_draw_poly(int nv,g3s_point **pointlist)
{
	int c;
	r_polyc++;
	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS) {
		glColor3f(PAL2Tr(c), PAL2Tg(c), PAL2Tb(c));
		glDisable(GL_DEPTH_TEST); // ZICO - disable to show lasers correctly
	}else{
		glColor4f(PAL2Tr(c), PAL2Tg(c), PAL2Tb(c), 1.0 - (float)Gr_scanline_darkening_level / ((float)GR_FADE_LEVELS - 1.0));
	}
	glBegin(GL_TRIANGLE_FAN);
	for (c=0;c<nv;c++){
		glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
	}
	glEnd();
	glEnable(GL_DEPTH_TEST); // ZICO - enable it again to stay correct
	return 0;
}

void gr_upoly_tmap(int nverts, int *vert ){
		mprintf((0,"gr_upoly_tmap: unhandled\n"));//should never get called
}
void draw_tmap_flat(grs_bitmap *bm,int nv,g3s_point **vertlist){
		mprintf((0,"draw_tmap_flat: unhandled\n"));//should never get called
}
extern void (*tmap_drawer_ptr)(grs_bitmap *bm,int nv,g3s_point **vertlist);
bool g3_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bm)
{
	int c;
	float l;
	if (tmap_drawer_ptr==draw_tmap_flat){
		OGL_DISABLE(TEXTURE_2D);
		glColor4f(0,0,0,1.0-(Gr_scanline_darkening_level/(float)NUM_LIGHTING_LEVELS));
		glBegin(GL_TRIANGLE_FAN);
		for (c=0;c<nv;c++){
			glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
		}
		glEnd();
	}else if (tmap_drawer_ptr==draw_tmap){
		r_tpolyc++;
		OGL_ENABLE(TEXTURE_2D);
		ogl_bindbmtex(bm);
		ogl_texwrap(bm->gltexture,GL_REPEAT);
		glBegin(GL_TRIANGLE_FAN);
		for (c=0;c<nv;c++){
			if (bm->bm_flags&BM_FLAG_NO_LIGHTING){
				l=1.0;
			}else{
				l=f2fl(uvl_list[c].l);
			}
			glColor3f(l,l,l);
			glTexCoord2f(f2glf(uvl_list[c].u),f2glf(uvl_list[c].v));
			glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
		}
		glEnd();
	}else{
		mprintf((0,"g3_draw_tmap: unhandled tmap_drawer %p\n",tmap_drawer_ptr));
	}
	if (!bNoDepthTest)
		glEnable(GL_DEPTH_TEST); // ZICO - we need this so we do not run into any problems if lasers or sprites show up

	return 0;
}

int active_texture_unit = 0;

void ogl_setActiveTexture(int t)
{
	if (ogl_arb_multitexture_ok)
	{
#ifdef GL_ARB_multitexture
		if (t == 0)
			glActiveTextureARB(GL_TEXTURE0_ARB);
		else
			glActiveTextureARB(GL_TEXTURE1_ARB);
#endif
	}
	else if (ogl_sgis_multitexture_ok)
	{
#ifdef GL_SGIS_multitexture
		if (t == 0)
			glSelectTextureSGIS(GL_TEXTURE0_SGIS);
		else
			glSelectTextureSGIS(GL_TEXTURE1_SGIS);
#endif
	}
	active_texture_unit = t;
}

void ogl_MultiTexCoord2f(int t, float u, float v)
{
	if (ogl_arb_multitexture_ok)
	{
#ifdef GL_ARB_multitexture
		if (t == 0)
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u, v);
		else
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, u, v);
#endif
	}
	else if (ogl_sgis_multitexture_ok)
	{
#ifdef GL_SGIS_multitexture
		if (t == 0)
			glMultiTexCoord2fSGIS(GL_TEXTURE0_SGIS, u, v);
		else
			glMultiTexCoord2fSGIS(GL_TEXTURE1_SGIS, u, v);
#endif
	}
}

bool g3_draw_tmap_2(int nv, g3s_point **pointlist, g3s_uvl *uvl_list, grs_bitmap *bmbot, grs_bitmap *bm, int orient)
{
#if ((defined(GL_NV_register_combiners) || defined(GL_NV_texture_env_combine4)) && (defined(GL_ARB_multitexture) || defined(GL_SGIS_multitexture)))
	if ((/*ogl_nv_register_combiners_ok ||*/ ogl_nv_texture_env_combine4_ok) && (ogl_arb_multitexture_ok || ogl_sgis_multitexture_ok))
	{
		int c;
		float l, u1, v1;

		if (tmap_drawer_ptr != draw_tmap)
			Error("WTFF\n");

		r_tpolyc+=2;

		//ogl_setActiveTexture(0);
		OGL_ENABLE(TEXTURE_2D);
		ogl_bindbmtex(bmbot);
		ogl_texwrap(bmbot->gltexture, GL_REPEAT);
		// GL_MODULATE is fine for texture 0

		ogl_setActiveTexture(1);
		glEnable(GL_TEXTURE_2D);
		ogl_bindbmtex(bm);
		ogl_texwrap(bm->gltexture,GL_REPEAT);

#ifdef GL_NV_register_combiners
		if (ogl_nv_register_combiners_ok)
		{
			glEnable(GL_REGISTER_COMBINERS_NV);
			glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);
			// spare0 = tex0 * (1-alpha1) + tex1 * alpha1
			glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
			glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
			glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
			glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
			glCombinerOutputNV(
			                   GL_COMBINER0_NV,   //GLenum stage
			                   GL_RGB,            //GLenum portion,
			                   GL_DISCARD_NV,     //GLenum abOutput,
			                   GL_DISCARD_NV,     //GLenum cdOutput,
			                   GL_SPARE0_NV,      //GLenum sumOutput,
			                   GL_NONE,           //GLenum scale,
			                   GL_NONE,           //GLenum bias,
			                   GL_FALSE,          //GLboolean abDotProduct,
			                   GL_FALSE,          //GLboolean cdDotProduct,
			                   GL_FALSE           //GLboolean muxSum
			                   );
			// out = spare0 * color
			// ( out = AB + (1-A)C + D )
			glFinalCombinerInputNV(GL_VARIABLE_A_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
			glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
			glFinalCombinerInputNV(GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
			glFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

			if (bm->bm_flags & BM_FLAG_SUPER_TRANSPARENT)
			{
				// out = alpha0*(1-tex1) + alpha1
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_INVERT_NV, GL_BLUE);
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
			}
			else
			{
				// out = alpha0 + alpha1
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
				glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
			}
			glCombinerOutputNV(
			                   GL_COMBINER0_NV,   //GLenum stage
			                   GL_ALPHA,          //GLenum portion,
			                   GL_DISCARD_NV,     //GLenum abOutput,
			                   GL_DISCARD_NV,     //GLenum cdOutput,
			                   GL_SPARE0_NV,      //GLenum sumOutput,
			                   GL_NONE,           //GLenum scale,
			                   GL_NONE,           //GLenum bias,
			                   GL_FALSE,          //GLboolean abDotProduct,
			                   GL_FALSE,          //GLboolean cdDotProduct,
			                   GL_FALSE           //GLboolean muxSum
			                   );
			glFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
		}
		else
#endif
		{
			//http://oss.sgi.com/projects/ogl-sample/registry/NV/texture_env_combine4.txt
			//only GL_NV_texture_env_combine4 lets us do what we need:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV);

			//multiply top texture by color(vertex lighting) and add bottom texture(where alpha says to)
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, GL_PREVIOUS_EXT);

			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_ONE_MINUS_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, GL_SRC_COLOR);

			//add up alpha channels
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_ADD);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_ZERO);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_PREVIOUS_EXT);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, GL_ZERO);

			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_EXT, GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_ONE_MINUS_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_EXT, GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, GL_ONE_MINUS_SRC_ALPHA);
		}

		// GL_DECAL works sorta ok but the top texture is fullbright.
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		// GL_ARB_texture_env_combine comes close, but doesn't quite make it.
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		//// this gives effect like GL_DECAL:
		//glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);


		// this properly shades the top texture, but the bottom texture doesn't get through.
		//glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);


		// add up alpha
		//glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_ADD);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		//glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);

		glBegin(GL_TRIANGLE_FAN);
		for (c=0;c<nv;c++){
			switch(orient){
				case 1:
					u1=1.0-f2glf(uvl_list[c].v);
					v1=f2glf(uvl_list[c].u);
					break;
				case 2:
					u1=1.0-f2glf(uvl_list[c].u);
					v1=1.0-f2glf(uvl_list[c].v);
					break;
				case 3:
					u1=f2glf(uvl_list[c].v);
					v1=1.0-f2glf(uvl_list[c].u);
					break;
				default:
					u1=f2glf(uvl_list[c].u);
					v1=f2glf(uvl_list[c].v);
					break;
			}
			if (bm->bm_flags&BM_FLAG_NO_LIGHTING){
				l=1.0;
			}else{
				l=f2fl(uvl_list[c].l);
			}
			glColor3f(l,l,l);
			ogl_MultiTexCoord2f(0, f2glf(uvl_list[c].u), f2glf(uvl_list[c].v));
			ogl_MultiTexCoord2f(1, u1, v1);
			//glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),f2glf(pointlist[c]->p3_vec.z));
			//glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),f2glf(pointlist[c]->p3_vec.z));
			glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
		}
		glEnd();
		//ogl_setActiveTexture(1); // still the active texture
#ifdef GL_NV_register_combiners
		if (ogl_nv_register_combiners_ok)
		{
			glDisable(GL_REGISTER_COMBINERS_NV);
		}
		else
#endif
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
		glDisable(GL_TEXTURE_2D);
		ogl_setActiveTexture(0);
	}
	else
#endif
	{
		int c;
		float l,u1,v1;

		g3_draw_tmap(nv,pointlist,uvl_list,bmbot);//draw the bottom texture first.. could be optimized with multitexturing..

		r_tpolyc++;
		/*	if (bm->bm_w !=64||bm->bm_h!=64)
			printf("g3_draw_tmap w %i h %i\n",bm->bm_w,bm->bm_h);*/
		OGL_ENABLE(TEXTURE_2D);
		ogl_bindbmtex(bm);
		ogl_texwrap(bm->gltexture,GL_REPEAT);
		glBegin(GL_TRIANGLE_FAN);
		for (c=0;c<nv;c++){
			switch(orient){
				case 1:
					u1=1.0-f2glf(uvl_list[c].v);
					v1=f2glf(uvl_list[c].u);
					break;
				case 2:
					u1=1.0-f2glf(uvl_list[c].u);
					v1=1.0-f2glf(uvl_list[c].v);
					break;
				case 3:
					u1=f2glf(uvl_list[c].v);
					v1=1.0-f2glf(uvl_list[c].u);
					break;
				default:
					u1=f2glf(uvl_list[c].u);
					v1=f2glf(uvl_list[c].v);
					break;
			}
			if (bm->bm_flags&BM_FLAG_NO_LIGHTING){
				l=1.0;
			}else{
				//l=f2fl(uvl_list[c].l)+gr_palette_gamma/63.0;
				l=f2fl(uvl_list[c].l);
			}
			glColor3f(l,l,l);
			glTexCoord2f(u1,v1);
			//glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),f2glf(pointlist[c]->p3_vec.z));
			//glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),f2glf(pointlist[c]->p3_vec.z));
			glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
		}
		glEnd();
	}
	return 0;
}

bool g3_draw_bitmap(vms_vector *pos,fix width,fix height,grs_bitmap *bm, int orientation, object *obj)
{
	vms_vector pv,v1;//,v2;
	int i;
	r_bitmapc++;
	v1.z=0;

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP);

	glDisable(GL_DEPTH_TEST); // ZICO - disable to prevent sprites get cutted by polygons

	glBegin(GL_QUADS);
	// Define alpha by looking for object TYPE or ID. We do this here so we have it seperated from the rest of the code.
	if (glalpha_effects && (obj->type==OBJ_FIREBALL || obj->type==OBJ_WEAPON || obj->id==POW_EXTRA_LIFE || obj->id==POW_ENERGY || obj->id==POW_SHIELD_BOOST || obj->id==POW_HOARD_ORB || obj->id==POW_CLOAK || obj->id==POW_INVULNERABILITY))
		glColor4f(1.0,1.0,1.0,0.6);
	else
		glColor3f(1.0,1.0,1.0);
	width = fixmul(width,Matrix_scale.x);	
	height = fixmul(height,Matrix_scale.y);	
	for (i=0;i<4;i++){
		vm_vec_sub(&v1,pos,&View_position);
		vm_vec_rotate(&pv,&v1,&View_matrix);
		switch (i){
			case 0:
				glTexCoord2f(0.0, 0.0);
				pv.x+=-width;
				pv.y+=height;
				break;
			case 1:
				glTexCoord2f(bm->gltexture->u, 0.0);
				pv.x+=width;
				pv.y+=height;
				break;
			case 2:
				glTexCoord2f(bm->gltexture->u, bm->gltexture->v);
				pv.x+=width;
				pv.y+=-height;
				break;
			case 3:
				glTexCoord2f(0.0, bm->gltexture->v);
				pv.x+=-width;
				pv.y+=-height;
				break;
		}
		glVertex3f(f2glf(pv.x),f2glf(pv.y),-f2glf(pv.z));
	}
	glEnd();
	if (!bNoDepthTest) // ZICO - need this for escape seq
		glEnable(GL_DEPTH_TEST);
	return 0;
}

bool ogl_ubitmapm_c(int x, int y,grs_bitmap *bm,int c)
{
	GLfloat xo,yo,xf,yf;
	GLfloat u1,u2,v1,v2;
	r_ubitmapc++;
	x+=grd_curcanv->cv_bitmap.bm_x;
	y+=grd_curcanv->cv_bitmap.bm_y;
	xo=x/(float)last_width;
	xf=(bm->bm_w+x)/(float)last_width;
	yo=1.0-y/(float)last_height;
	yf=1.0-(bm->bm_h+y)/(float)last_height;

//	printf("g3_draw_bitmap: %f,%f,%f - ",f2glf(pos->x),f2glf(pos->y),-f2glf(pos->z));
//	printf("(%f,%f,%f) ",f2glf(View_position.x),f2glf(View_position.y),-f2glf(View_position.z));

/*		glEnABLE(ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.0);*/

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP);
	
	if (bm->bm_x==0){
		u1=0;
		if (bm->bm_w==bm->gltexture->w)
			u2=bm->gltexture->u;
		else
			u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}else {
		u1=bm->bm_x/(float)bm->gltexture->tw;
		u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}
	if (bm->bm_y==0){
		v1=0;
		if (bm->bm_h==bm->gltexture->h)
			v2=bm->gltexture->v;
		else
			v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}else{
		v1=bm->bm_y/(float)bm->gltexture->th;
		v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}

	glBegin(GL_QUADS);
	if (c<0)
		glColor3f(1.0,1.0,1.0);
	else
		glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glTexCoord2f(u1, v1); glVertex2f(xo, yo);
	glTexCoord2f(u2, v1); glVertex2f(xf, yo);
	glTexCoord2f(u2, v2); glVertex2f(xf, yf);
	glTexCoord2f(u1, v2); glVertex2f(xo, yf);
	glEnd();
//	glDisABLE(ALPHA_TEST);
	
	return 0;
}
bool ogl_ubitmapm(int x, int y,grs_bitmap *bm){
	return ogl_ubitmapm_c(x,y,bm,-1);
//	return ogl_ubitblt(bm->bm_w,bm->bm_h,x,y,0,0,bm,NULL);
}
#if 0
//also upsidedown, currently.
bool ogl_ubitblt(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	GLfloat xo,yo;//,xs,ys;
	glmprintf((0,"ogl_ubitblt(w=%i,h=%i,dx=%i,dy=%i,sx=%i,sy=%i,src=%p,dest=%p\n",w,h, dx, dy,sx, sy,  src,dest));
	
	dx+=dest->bm_x;
	dy+=dest->bm_y;
	
	xo=dx/(float)last_width;
//	xo=dx/(float)grd_curscreen->sc_w;
//	xs=w/(float)last_width;
	//yo=1.0-dy/(float)last_height;
	yo=1.0-(dy+h)/(float)last_height;
//	ys=h/(float)last_height;
	
//	OGL_ENABLE(TEXTURE_2D);
	
	OGL_DISABLE(TEXTURE_2D);
	glRasterPos2f(xo,yo);
	ogl_filltexbuf(src->bm_data,texbuf,src->bm_w,w,h,sx,sy,w,h,GL_RGBA);
	glDrawPixels(w,h,GL_RGBA,GL_UNSIGNED_BYTE,texbuf);
	glRasterPos2f(0,0);
	
	return 0;
}
#else

bool ogl_ubitblt_i(int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int mipmap)
{
	GLfloat xo,yo,xs,ys;
	GLfloat u1,v1;
	ogl_texture tex;
	r_ubitbltc++;

	if (mipmap) {
		ogl_init_texture(&tex, sw, sh, OGL_FLAG_MIPMAP);
	} else {
		ogl_init_texture(&tex, sw, sh, OGL_FLAG_ALPHA);
	}
	tex.prio = 0.0;
	tex.lw=src->bm_rowsize;

	u1=v1=0;
	
	dx+=dest->bm_x;
	dy+=dest->bm_y;
	xo=dx/(float)last_width;
	xs=dw/(float)last_width;
	yo=1.0-dy/(float)last_height;
	ys=dh/(float)last_height;
	
	OGL_ENABLE(TEXTURE_2D);
	
	ogl_pal=gr_current_pal;
	ogl_loadtexture(src->bm_data, sx, sy, &tex, src->bm_flags, 0);
	ogl_pal=gr_palette;
	OGL_BINDTEXTURE(tex.handle);
	
	ogl_texwrap(&tex,GL_CLAMP);

	glBegin(GL_QUADS);
	glColor3f(1.0,1.0,1.0);
	glTexCoord2f(u1, v1); glVertex2f(xo, yo);
	glTexCoord2f(tex.u, v1); glVertex2f(xo+xs, yo);
	glTexCoord2f(tex.u, tex.v); glVertex2f(xo+xs, yo-ys);
	glTexCoord2f(u1, tex.v); glVertex2f(xo, yo-ys);
	glEnd();
	ogl_freetexture(&tex);
	return 0;
}
bool ogl_ubitblt(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest){
	return ogl_ubitblt_i(w,h,dx,dy,w,h,sx,sy,src,dest,0);
}
#endif
bool ogl_ubitblt_tolinear(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest){
#if 1
	unsigned char *d,*s;
	int i,j;
	int w1,h1;
//	w1=w;h1=h;
	w1=grd_curscreen->sc_w;h1=grd_curscreen->sc_h;
//	if (w1*h1*3>OGLTEXBUFSIZE)
//		Error("ogl_ubitblt_tolinear: screen res larger than OGLTEXBUFSIZE\n");

	if (ogl_readpixels_ok>0){
		OGL_DISABLE(TEXTURE_2D);
		glReadBuffer(GL_FRONT);
		glReadPixels(0,0,w1,h1,GL_RGB,GL_UNSIGNED_BYTE,texbuf);
//		glReadPixels(sx,grd_curscreen->sc_h-(sy+h),w,h,GL_RGB,GL_UNSIGNED_BYTE,texbuf);
//		glReadPixels(sx,sy,w+sx,h+sy,GL_RGB,GL_UNSIGNED_BYTE,texbuf);
	}else
		memset(texbuf,0,w1*h1*3);
	sx+=src->bm_x;
	sy+=src->bm_y;
	for (i=0;i<h;i++){
		d=dest->bm_data+dx+(dy+i)*dest->bm_rowsize;
		s=texbuf+((h1-(i+sy+1))*w1+sx)*3;
		for (j=0;j<w;j++){
			*d=gr_find_closest_color(s[0]/4,s[1]/4,s[2]/4);
			s+=3;
			d++;
		}
	}
#else
	int i,j,c=0;
	unsigned char *d,*s,*e;
	if (w*h*3>OGLTEXBUFSIZE)
		Error("ogl_ubitblt_tolinear: size larger than OGLTEXBUFSIZE\n");
	sx+=src->bm_x;
	sy+=src->bm_y;
#if 1//also seems to cause a mess.  need to look into it a bit more..
	if (ogl_readpixels_ok>0){
		OGL_DISABLE(TEXTURE_2D);
		glReadBuffer(GL_FRONT);
//		glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,texbuf);
		glReadPixels(sx,grd_curscreen->sc_h-(sy+h),w,h,GL_RGB,GL_UNSIGNED_BYTE,texbuf);
	}else
#endif
		memset(texbuf,0,w*h*3);
//	d=dest->bm_data+dx+(dy+i)*dest->bm_rowsize;
	d=dest->bm_data+dx+dy*dest->bm_rowsize;
	for (i=0;i<h;i++){
		s=texbuf+w*(h-(i+1))*3;
//		s=texbuf+w*i*3;
		if (s<texbuf){Error("blah1\n");}
		if (d<dest->bm_data){Error("blah3\n");}
//		d=dest->bm_data+(i*dest->bm_rowsize);

		e=d;
		for (j=0;j<w;j++){
			if (s>texbuf+w*h*3-3){Error("blah2\n");}
			if (d>dest->bm_data+dest->bm_rowsize*(h+dy)+dx	){Error("blah4\n");}
			*d=gr_find_closest_color(s[0]/4,s[1]/4,s[2]/4);
			s+=3;
			d++;
			c++;
		}
		d=e;
		d+=dest->bm_rowsize;
	}
	glmprintf((0,"c=%i w*h=%i\n",c,w*h));
#endif
	return 0;
}

bool ogl_ubitblt_copy(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest){
#if 0 //just seems to cause a mess.
	GLfloat xo,yo;//,xs,ys;
	
	dx+=dest->bm_x;
	dy+=dest->bm_y;
	
//	xo=dx/(float)last_width;
	xo=dx/(float)grd_curscreen->sc_w;
//	yo=1.0-(dy+h)/(float)last_height;
	yo=1.0-(dy+h)/(float)grd_curscreen->sc_h;
	sx+=src->bm_x;
	sy+=src->bm_y;
	OGL_DISABLE(TEXTURE_2D);
	glReadBuffer(GL_FRONT);
	glRasterPos2f(xo,yo);
//	glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,texbuf);
	glCopyPixels(sx,grd_curscreen->sc_h-(sy+h),w,h,GL_COLOR);
	glRasterPos2f(0,0);
#endif
	return 0;
}

grs_canvas *offscreen_save_canv = NULL, *offscreen_canv = NULL;
//float pixels [OGLTEXBUFSIZE];
GLubyte *pixels = NULL;

void ogl_start_offscreen_render(int x, int y, int w, int h) {
	int y2;

	if (offscreen_canv) Error("ogl_start_offscreen_render: offscreen_canv!=NULL");
	offscreen_save_canv = grd_curcanv;
	glDrawBuffer(GL_BACK);
	offscreen_canv = gr_create_sub_canvas(grd_curcanv, x, y, w, h);
	gr_set_current_canvas(offscreen_canv);
	y2 = last_height - offscreen_canv->cv_bitmap.bm_y - offscreen_canv->cv_bitmap.bm_h;
	glReadPixels(offscreen_canv->cv_bitmap.bm_x,y2,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
}
void ogl_end_offscreen_render(void) {
	int y;

	if (!offscreen_canv) Error("ogl_end_offscreen_render: no offscreen_canv");
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_BACK);
	OGL_DISABLE(TEXTURE_2D);
	y = last_height - offscreen_canv->cv_bitmap.bm_y - offscreen_canv->cv_bitmap.bm_h;
	glRasterPos2f(offscreen_canv->cv_bitmap.bm_x/(float)last_width, y/(float)last_height);
	glDrawPixels(offscreen_canv->cv_bitmap.bm_w, offscreen_canv->cv_bitmap.bm_h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
	gr_free_sub_canvas(offscreen_canv);
	gr_set_current_canvas(offscreen_save_canv);
	offscreen_canv=NULL;
}

void ogl_start_frame(void){
	r_polyc=0;r_tpolyc=0;r_bitmapc=0;r_ubitmapc=0;r_ubitbltc=0;r_upixelc=0;

	OGL_VIEWPORT(grd_curcanv->cv_bitmap.bm_x,grd_curcanv->cv_bitmap.bm_y,Canvas_width,Canvas_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL,0.02);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glShadeModel(GL_SMOOTH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();//clear matrix
	gluPerspective(90.0,1.0,0.01,1000000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
}

#ifndef NMONO
void merge_textures_stats(void);
#endif
void ogl_end_frame(void){
	OGL_VIEWPORT(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();//clear matrix
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}
void ogl_swap_buffers(void){
	ogl_clean_texture_cache();
	if (gr_renderstats){
		gr_printf(5,GAME_FONT->ft_h*13+3*13,"%i flat %i tex %i sprites %i bitmaps",r_polyc,r_tpolyc,r_bitmapc,r_ubitmapc);
//	glmprintf((0,"ogl_end_frame: %i polys, %i tmaps, %i sprites, %i bitmaps, %i bitblts, %i pixels\n",r_polyc,r_tpolyc,r_bitmapc,r_ubitmapc,r_ubitbltc,r_upixelc));//we need to do it here because some things get drawn after end_frame
	}
	ogl_do_palfx();
	ogl_swap_buffers_internal();
	glClear(GL_COLOR_BUFFER_BIT);
}

void ogl_init_shared_palette(void)
{
#ifdef GL_EXT_paletted_texture
	if (ogl_shared_palette_ok)
	{
		int i;

		glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
		//glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, ogl_pal);

		for (i = 0; i < 256; i++)
		{
			if (i == 254)
			{
				texbuf[i * 4] = 255;
				texbuf[i * 4 + 1] = 255;
				texbuf[i * 4 + 2] = 255;
				texbuf[i * 4 + 3] = 0;
			}
			else if (i == 255)
			{
				texbuf[i * 4] = 0;
				texbuf[i * 4 + 1] = 0;
				texbuf[i * 4 + 2] = 0;
				texbuf[i * 4 + 3] = 0;
			}
			else
			{
				texbuf[i * 4] = gr_current_pal[i * 3] * 4;
				texbuf[i * 4 + 1] = gr_current_pal[i * 3 + 1] * 4;
				texbuf[i * 4 + 2] = gr_current_pal[i * 3 + 2] * 4;
				texbuf[i * 4 + 3] = 255;
			}
		}
		glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, texbuf);
	}
#endif
}

int tex_format_supported(int iformat,int format){
	switch (iformat){
		case GL_INTENSITY4:
			if (!ogl_intensity4_ok) return 0; break;
		case GL_LUMINANCE4_ALPHA4:
			if (!ogl_luminance4_alpha4_ok) return 0; break;
		case GL_RGBA2:
			if (!ogl_rgba2_ok) return 0; break;
	}
	if (ogl_gettexlevelparam_ok){
		GLint internalFormat;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, iformat, 64, 64, 0,
				format, GL_UNSIGNED_BYTE, texbuf);//NULL?
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0,
				GL_TEXTURE_INTERNAL_FORMAT,
				&internalFormat);
		return (internalFormat==iformat);
	}else
		return 1;
}

//little hack to find the nearest bigger power of 2 for a given number
int pow2ize(int x){
	int i;
	for (i=2; i<x; i*=2) {}; //corrected by MD2211: was previously limited to 4096
	return i;
}



//GLubyte texbuf[512*512*4];
//GLubyte texbuf[OGLTEXBUFSIZE];
GLubyte *texbuf = NULL;

// Allocate the pixel buffers 'pixels' and 'texbuf' based on current screen resolution
void ogl_init_pixel_buffers(int w, int h)
{
	w = pow2ize(w);	// convert to OpenGL texture size
	h = pow2ize(h);

	if (pixels)
		d_free(pixels);
	pixels = d_malloc(w*h*4);

	if (texbuf)
		d_free(texbuf);
	texbuf = d_malloc(max(w, 1024)*max(h, 256)*4);	// must also fit big font texture

	if ((pixels == NULL) || (texbuf == NULL))
		Error("Not enough memory for current resolution");
}

void ogl_close_pixel_buffers(void)
{
	d_free(pixels);
	d_free(texbuf);
}

void ogl_filltexbuf(unsigned char *data, GLubyte *texp, int truewidth, int width, int height, int dxo, int dyo, int twidth, int theight, int type, int bm_flags, int data_format)
{
//	GLushort *tex=(GLushort *)texp;
	int x,y,c,i;
//	if (twidth*theight*4>sizeof(texbuf))//shouldn't happen, descent never uses textures that big.
	if ((width > max(grd_curscreen->sc_w, 1024)) || (height > max(grd_curscreen->sc_h, 256)))
		Error("Texture is too big: %ix%i", width, height);

	i=0;
	for (y=0;y<theight;y++){
		i=dxo+truewidth*(y+dyo);
		for (x=0;x<twidth;x++){
			if (x<width && y<height)
				if (data_format)
				{
					int j;

					for (j = 0; j < data_format; ++j)
						(*(texp++)) = data[i * data_format + j];
					i++;
					continue;
				}
				else
					c = data[i++];
			else
				c = 256; // fill the pad space with transparency (or blackness)
			if (c == 254 && (bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			{
				switch (type)
				{
				case GL_LUMINANCE_ALPHA:
					(*(texp++)) = 255;
					(*(texp++)) = 0;
					break;
				case GL_RGBA:
					(*(texp++)) = 255;
					(*(texp++)) = 255;
					(*(texp++)) = 255;
					(*(texp++)) = 0; // transparent pixel
					break;
				case GL_COLOR_INDEX:
					(*(texp++)) = c;
					break;
				default:
					Error("ogl_filltexbuf unhandled super-transparent texformat\n");
					break;
				}
			}
			else if ((c == 255 && (bm_flags & BM_FLAG_TRANSPARENT)) || c == 256)
			{
				switch (type){
					case GL_LUMINANCE:
						(*(texp++))=0;
						break;
					case GL_LUMINANCE_ALPHA:
						(*(texp++))=0;
						(*(texp++))=0;
						break;
					case GL_RGB:
						(*(texp++)) = 0;
						(*(texp++)) = 0;
						(*(texp++)) = 0;
						break;
					case GL_RGBA:
						(*(texp++))=0;
						(*(texp++))=0;
						(*(texp++))=0;
						(*(texp++))=0;//transparent pixel
						break;
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
					default:
						Error("ogl_filltexbuf unknown texformat\n");
						break;
				}
			}else{
				switch (type){
					case GL_LUMINANCE://these could prolly be done to make the intensity based upon the intensity of the resulting color, but its not needed for anything (yet?) so no point. :)
						(*(texp++))=255;
						break;
					case GL_LUMINANCE_ALPHA:
						(*(texp++))=255;
						(*(texp++))=255;
						break;
					case GL_RGB:
						(*(texp++)) = ogl_pal[c * 3] * 4;
						(*(texp++)) = ogl_pal[c * 3 + 1] * 4;
						(*(texp++)) = ogl_pal[c * 3 + 2] * 4;
						break;
					case GL_RGBA:
						//(*(texp++))=gr_palette[c*3]*4;
						//(*(texp++))=gr_palette[c*3+1]*4;
						//(*(texp++))=gr_palette[c*3+2]*4;
						(*(texp++))=ogl_pal[c*3]*4;
						(*(texp++))=ogl_pal[c*3+1]*4;
						(*(texp++))=ogl_pal[c*3+2]*4;
						(*(texp++))=255;//not transparent
						//				(*(tex++))=(ogl_pal[c*3]>>1) + ((ogl_pal[c*3+1]>>1)<<5) + ((ogl_pal[c*3+2]>>1)<<10) + (1<<15);
						break;
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
					default:
						Error("ogl_filltexbuf unknown texformat\n");
						break;
				}
			}
		}
	}
}

int tex_format_verify(ogl_texture *tex){
	while (!tex_format_supported(tex->internalformat,tex->format)){
		glmprintf((0,"tex format %x not supported",tex->internalformat));
		switch (tex->internalformat){
			case GL_INTENSITY4:
				if (ogl_luminance4_alpha4_ok){
					tex->internalformat=GL_LUMINANCE4_ALPHA4;
					tex->format=GL_LUMINANCE_ALPHA;
					break;
				}//note how it will fall through here if the statement is false
			case GL_LUMINANCE4_ALPHA4:
				if (ogl_rgba2_ok){
					tex->internalformat=GL_RGBA2;
					tex->format=GL_RGBA;
					break;
				}//note how it will fall through here if the statement is false
			case GL_RGBA2:
#if defined(__APPLE__) && defined(__MACH__)
			case GL_RGB8:	// Quartz doesn't support RGB only
#endif
				tex->internalformat = ogl_rgba_internalformat;
				tex->format=GL_RGBA;
				break;
			default:
				mprintf((0,"...no tex format to fall back on\n"));
				return 1;
		}
		glmprintf((0,"...falling back to %x\n",tex->internalformat));
	}
	return 0;
}
void tex_set_size1(ogl_texture *tex,int dbits,int bits,int w, int h){
	int u;
	if (tex->tw!=w || tex->th!=h){
		u=(tex->w/(float)tex->tw*w) * (tex->h/(float)tex->th*h);
		glmprintf((0,"shrunken texture?\n"));
	}else
		u=tex->w*tex->h;
	if (bits<=0){//the beta nvidia GLX server. doesn't ever return any bit sizes, so just use some assumptions.
		tex->bytes=((float)w*h*dbits)/8.0;
		tex->bytesu=((float)u*dbits)/8.0;
	}else{
		tex->bytes=((float)w*h*bits)/8.0;
		tex->bytesu=((float)u*bits)/8.0;
	}
	glmprintf((0,"tex_set_size1: %ix%i, %ib(%i) %iB\n",w,h,bits,dbits,tex->bytes));
}
void tex_set_size(ogl_texture *tex){
	GLint w,h;
	int bi=16,a=0;
	if (ogl_gettexlevelparam_ok){
		GLint t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_LUMINANCE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTENSITY_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_RED_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_GREEN_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_BLUE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_ALPHA_SIZE,&t);a+=t;
#ifdef GL_EXT_paletted_texture
		if (ogl_paletted_texture_ok)
		{
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INDEX_SIZE_EXT, &t);
			a += t;
		}
#endif
	}else{
		w=tex->tw;
		h=tex->th;
	}
	switch (tex->format){
		case GL_LUMINANCE:
			bi=8;
			break;
		case GL_LUMINANCE_ALPHA:
			bi=8;
			break;
		case GL_RGB:
		case GL_RGBA:
			bi=16;
			break;
		case GL_COLOR_INDEX:
			bi = 8;
			break;
		default:
			Error("tex_set_size unknown texformat\n");
			break;
	}
	tex_set_size1(tex,bi,a,w,h);
}
//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
int ogl_loadtexture (unsigned char *data, int dxo, int dyo, ogl_texture *tex, int bm_flags, int data_format)
{
	GLubyte	*bufP = texbuf;
	tex->tw = pow2ize (tex->w);
	tex->th = pow2ize (tex->h);//calculate smallest texture size that can accomodate us (must be multiples of 2)

	if (gr_badtexture > 0) 
		return 1;

#ifndef MACINTOSH
	if (tex_format_verify (tex))
		return 1;
#endif

	//calculate u/v values that would make the resulting texture correctly sized
	tex->u = (float) ((double) tex->w / (double) tex->tw);
	tex->v = (float) ((double) tex->h / (double) tex->th);

	if (data) {
		if (bm_flags >= 0)
			ogl_filltexbuf (data, texbuf, tex->lw, tex->w, tex->h, dxo, dyo, tex->tw, tex->th, 
								 tex->format, bm_flags, data_format);
		else {
#if 0
			memcpy (bufP = texbuf, data, tex->w * tex->h * 4);
#else
			if (!dxo && !dyo && (tex->w == tex->tw) && (tex->h == tex->th))
				bufP = data;
			else {
				int h, w, tw;
				
				h = tex->lw / tex->w;
				w = (tex->w - dxo) * h;
				data += tex->lw * dyo + h * dxo;
				bufP = texbuf;
				tw = tex->tw * h;
				h = tw - w;
				for (; dyo < tex->h; dyo++, data += tex->lw) {
					memcpy (bufP, data, w);
					bufP += w;
					memset (bufP, 0, h);
					bufP += h;
				}
				memset (bufP, 0, tex->th * tw - (bufP - texbuf));
				bufP = texbuf;
				}
#endif
			}
	}
	// Generate OpenGL texture IDs.
	glGenTextures (1, &tex->handle);
	//set priority
	glPrioritizeTextures (1, &tex->handle, &tex->prio);
	// Give our data to OpenGL.
	OGL_BINDTEXTURE(tex->handle);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (tex->wantmip){
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_texmagfilt);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_texminfilt);
		}
	else{
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	if (tex->wantmip && GL_needmipmaps)
		gluBuild2DMipmaps (
				GL_TEXTURE_2D, tex->internalformat, 
				tex->tw, tex->th, tex->format, 
				GL_UNSIGNED_BYTE, 
				bufP);
	else
		glTexImage2D (
			GL_TEXTURE_2D, 0, tex->internalformat,
			tex->tw, tex->th, 0, tex->format, // RGBA textures.
			GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
			bufP);
	tex_set_size (tex);
	r_texcount++; 
	return 0;
}

unsigned char decodebuf[1024*1024];

void ogl_loadbmtexture_f(grs_bitmap *bm, int flags)
{
	unsigned char *buf;
#ifdef HAVE_LIBPNG
	char *bitmapname;
#endif

	while (bm->bm_parent)
		bm=bm->bm_parent;
	if (bm->gltexture && bm->gltexture->handle > 0)
		return;
	buf=bm->bm_data;
#ifdef HAVE_LIBPNG
	if ((bitmapname = piggy_game_bitmap_name(bm)))
	{
		char filename[64];
		png_data pdata;

		sprintf(filename, "textures/%s.png", bitmapname);
		if (read_png(filename, &pdata))
		{
			printf("%s: %ux%ux%i p=%i(%i) c=%i a=%i chans=%i\n", filename, pdata.width, pdata.height, pdata.depth, pdata.paletted, pdata.num_palette, pdata.color, pdata.alpha, pdata.channels);
			if (pdata.depth == 8 && pdata.color)
			{
				if (bm->gltexture == NULL)
					ogl_init_texture(bm->gltexture = ogl_get_free_texture(), pdata.width, pdata.height, flags | ((pdata.alpha || bm->bm_flags & BM_FLAG_TRANSPARENT) ? OGL_FLAG_ALPHA : 0));
				ogl_loadtexture(pdata.data, 0, 0, bm->gltexture, bm->bm_flags, pdata.paletted ? 0 : pdata.channels);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
				return;
			}
			else
			{
				printf("%s: unsupported texture format: must be rgb, rgba, or paletted, and depth 8\n", filename);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
			}
		}
	}
#endif
	if (bm->gltexture == NULL){
 		ogl_init_texture(bm->gltexture = ogl_get_free_texture(), bm->bm_w, bm->bm_h, flags | ((bm->bm_flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))? OGL_FLAG_ALPHA : 0));
	}
	else {
		if (bm->gltexture->handle>0)
			return;
		if (bm->gltexture->w==0){
			bm->gltexture->lw=bm->bm_w;
			bm->gltexture->w=bm->bm_w;
			bm->gltexture->h=bm->bm_h;
		}
	}
	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = decodebuf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
		buf=decodebuf;
	}
	ogl_loadtexture(buf, 0, 0, bm->gltexture, bm->bm_flags, 0);
}

void ogl_loadbmtexture(grs_bitmap *bm)
{
	ogl_loadbmtexture_f(bm, OGL_FLAG_MIPMAP);
}

void ogl_freetexture(ogl_texture *gltexture)
{
	if (gltexture->handle>0) {
		r_texcount--;
		glmprintf((0,"ogl_freetexture(%p):%i (last rend %is) (%i left)\n",gltexture,gltexture->handle,(GameTime-gltexture->lastrend)/f1_0,r_texcount));
		glDeleteTextures( 1, &gltexture->handle );
//		gltexture->handle=0;
		ogl_reset_texture(gltexture);
	}
}
void ogl_freebmtexture(grs_bitmap *bm){
	if (bm->gltexture){
		ogl_freetexture(bm->gltexture);
		bm->gltexture=NULL;
//		r_texcount--;
//		glmprintf((0,"ogl_freebmtexture(%p,%p):%i (%i left)\n",bm->bm_data,&bm->gltexture,bm->gltexture,r_texcount));
//		glDeleteTextures( 1, &bm->gltexture );
//		bm->gltexture=-1;
	}
}

bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale) // to scale bitmaps
{
	GLfloat xo,yo,xf,yf;
	GLfloat u1,u2,v1,v2;
	GLdouble h,a;
	r_ubitmapc++;
	x+=grd_curcanv->cv_bitmap.bm_x;
	y+=grd_curcanv->cv_bitmap.bm_y;
	xo=x/(float)last_width;
	xf=(bm->bm_w+x)/(float)last_width;
	yo=1.0-y/(float)last_height;
	yf=1.0-(bm->bm_h+y)/(float)last_height;

	if (dw < 0)
		dw = grd_curcanv->cv_bitmap.bm_w;
	else if (dw == 0)
		dw = bm->bm_w;
	if (dh < 0)
		dh = grd_curcanv->cv_bitmap.bm_h;
	else if (dh == 0)
		dh = bm->bm_h;
	r_ubitmapc++;

	a = (double) grd_curscreen->sc_w / (double) grd_curscreen->sc_h;
	h = (double) scale / (double) F1_0;

	xo = x / ((double) last_width * h);
	xf = (dw + x) / ((double) last_width * h);
	yo = 1.0 - y / ((double) last_height * h);
	yf = 1.0 - (dh + y) / ((double) last_height * h);

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP);
	
	if (bm->bm_x==0){
		u1=0;
		if (bm->bm_w==bm->gltexture->w)
			u2=bm->gltexture->u;
		else
			u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}else {
		u1=bm->bm_x/(float)bm->gltexture->tw;
		u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}
	if (bm->bm_y==0){
		v1=0;
		if (bm->bm_h==bm->gltexture->h)
			v2=bm->gltexture->v;
		else
			v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}else{
		v1=bm->bm_y/(float)bm->gltexture->th;
		v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}

	glBegin(GL_QUADS);
	if (c<0)
		glColor3f(1.0,1.0,1.0);
	else
		glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glTexCoord2f(u1, v1); glVertex2f(xo, yo);
	glTexCoord2f(u2, v1); glVertex2f(xf, yo);
	glTexCoord2f(u2, v2); glVertex2f(xf, yf);
	glTexCoord2f(u1, v2); glVertex2f(xo, yf);
	glEnd();
	
	return 0;
}
