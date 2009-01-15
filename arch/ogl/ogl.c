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
#include "console.h"
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
#include "endlevel.h"
#include "internal.h"
#include "gauges.h"
#include "playsave.h"

//change to 1 for lots of spew.
#if 0
#define glmprintf(0,a) con_printf(CON_DEBUG, a)
#else
#define glmprintf(a)
#endif

#ifndef M_PI
#define M_PI 3.14159
#endif

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__)) || defined(__sun__) || defined(macintosh)
#ifndef _MSC_VER
#define cosf(a) cos(a)
#define sinf(a) sin(a)
#endif
#endif

unsigned char *ogl_pal=gr_palette;

int GL_needmipmaps=0;
float OglTexMagFilt=GL_NEAREST;
float OglTexMinFilt=GL_NEAREST;
int active_texture_unit=0;

int last_width=-1,last_height=-1;
int GL_TEXTURE_2D_enabled=-1;
int GL_texclamp_enabled=-1;

int r_texcount = 0, r_cachedtexcount = 0;
int ogl_rgba_internalformat = GL_RGBA8;
int ogl_rgb_internalformat = GL_RGB8;
int sphereh=0;
int cross_lh[2]={0,0};
int primary_lh[3]={0,0,0};
int secondary_lh[5]={0,0,0,0,0};
int r_polyc,r_tpolyc,r_bitmapc,r_ubitbltc,r_upixelc;
extern int linedotscale;
#define f2glf(x) (f2fl(x))

#define OGL_BINDTEXTURE(a) glBindTexture(GL_TEXTURE_2D, a);


ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];
int ogl_texture_list_cur;

/* some function prototypes */

#define GL_TEXTURE0_ARB 0x84C0
extern GLubyte *pixels;
extern GLubyte *texbuf;
void ogl_filltexbuf(unsigned char *data, GLubyte *texp, int truewidth, int width, int height, int dxo, int dyo, int twidth, int theight, int type, int bm_flags, int data_format);
void ogl_loadbmtexture(grs_bitmap *bm);
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
			if (GameArg.DbgGlIntensity4Ok)
			{
				t->internalformat = GL_INTENSITY4;
				t->format = GL_LUMINANCE;
			}
			else if (GameArg.DbgGlLuminance4Alpha4Ok)
			{
				t->internalformat = GL_LUMINANCE4_ALPHA4;
				t->format = GL_LUMINANCE_ALPHA;
			}
			else if (GameArg.DbgGlRGBA2Ok)
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
	t->wrapstate = -1;
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
		ogl_texture_list[i].wrapstate = -1;
	}
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
}

void ogl_texture_stats(void)
{
	int used = 0, usedother = 0, usedidx = 0, usedrgb = 0, usedrgba = 0;
	int databytes = 0, truebytes = 0, datatexel = 0, truetexel = 0, i;
	int prio0=0,prio1=0,prio2=0,prio3=0,prioh=0;
	GLint idx, r, g, b, a, dbl, depth;
	int res, colorsize, depthsize;
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
	}

	res = SWIDTH * SHEIGHT;
	glGetIntegerv(GL_INDEX_BITS, &idx);
	glGetIntegerv(GL_RED_BITS, &r);
	glGetIntegerv(GL_GREEN_BITS, &g);
	glGetIntegerv(GL_BLUE_BITS, &b);
	glGetIntegerv(GL_ALPHA_BITS, &a);
	glGetIntegerv(GL_DOUBLEBUFFER, &dbl);
	dbl += 1;
	glGetIntegerv(GL_DEPTH_BITS, &depth);
	gr_set_current_canvas(NULL);
	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor( BM_XRGB(255,255,255),-1 );
	colorsize = (idx * res * dbl) / 8;
	depthsize = res * depth / 8;
	gr_printf(FSPACX(2),FSPACY(1),"%i flat %i tex %i sprites %i bitmaps",r_polyc,r_tpolyc,r_bitmapc);
	gr_printf(FSPACX(2), FSPACY(1)+LINE_SPACING, "%i(%i,%i,%i,%i) %iK(%iK wasted) (%i postcachedtex)", used, usedrgba, usedrgb, usedidx, usedother, truebytes / 1024, (truebytes - databytes) / 1024, r_texcount - r_cachedtexcount);
	gr_printf(FSPACX(2), FSPACY(1)+(LINE_SPACING*2), "%ibpp(r%i,g%i,b%i,a%i)x%i=%iK depth%i=%iK", idx, r, g, b, a, dbl, colorsize / 1024, depth, depthsize / 1024);
	gr_printf(FSPACX(2), FSPACY(1)+(LINE_SPACING*3), "total=%iK", (colorsize + depthsize + truebytes) / 1024);
}

void ogl_bindbmtex(grs_bitmap *bm){
	if (bm->gltexture==NULL || bm->gltexture->handle<=0)
		ogl_loadbmtexture(bm);
	OGL_BINDTEXTURE(bm->gltexture->handle);
	bm->gltexture->lastrend=GameTime;
	bm->gltexture->numrend++;
}

//gltexture MUST be bound first
void ogl_texwrap(ogl_texture *gltexture,int state)
{
	if (gltexture->wrapstate != state || gltexture->numrend < 1)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
		gltexture->wrapstate = state;
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
					if (GameArg.DbgAltTexMerge == 0 || (bm2->bm_flags & BM_FLAG_SUPER_TRANSPARENT))
						bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
					else {
						ogl_loadbmtexture(bm2);
					}
				}
				ogl_loadbmtexture(bm);
			}
		}
		glmprintf((0,"finished ef:%i\n",ef));
	}
	reset_special_effects();
	init_special_effects();
	{
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

bool g3_draw_line(g3s_point *p0,g3s_point *p1)
{
	int c;
	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
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

float bright_g[4]={	32.0/256,	255.0/256,	32.0/256,	1.0};
float dark_g[4]={	32.0/256,	138.0/256,	32.0/256,	0.6};

void ogl_draw_reticle(int cross,int primary,int secondary)
{
	fix size=270;
	float scale = ((float)SWIDTH/SHEIGHT);

	glPushMatrix();
	glTranslatef((grd_curcanv->cv_bitmap.bm_w/2+grd_curcanv->cv_bitmap.bm_x)/(float)last_width,1.0-(grd_curcanv->cv_bitmap.bm_h/2+grd_curcanv->cv_bitmap.bm_y)/(float)last_height,0);

	if (scale >= 1)
	{
		size/=scale;
		glScalef(f2glf(size),f2glf(size*scale),f2glf(size));
	}
	else
	{
		size*=scale;
		glScalef(f2glf(size/scale),f2glf(size),f2glf(size));
	}


	glLineWidth(linedotscale*2);
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	if (!cross_lh[cross])
	{
		cross_lh[cross]=glGenLists(1);
		glNewList(cross_lh[cross], GL_COMPILE_AND_EXECUTE);
		glBegin(GL_LINES);
		//cross top left
		glColor4fv(dark_g);
		glVertex2f(-4.0,2.0);
		if (cross)
			glColor4fv(bright_g);
		else
			glColor4fv(dark_g);
		glVertex2f(-2.0,0.0);
		//cross bottom left
		glColor4fv(dark_g);
		glVertex2f(-3.0,-4.0);
		if (cross)
			glColor4fv(bright_g);
		glVertex2f(-2.0,-3.0);
		//cross top right
		glColor4fv(dark_g);
		glVertex2f(4.0,2.0);
		if (cross)
			glColor4fv(bright_g);
		else
			glColor4fv(dark_g);
		glVertex2f(2.0,0.0);
		//cross bottom right
		glColor4fv(dark_g);
		glVertex2f(3.0,-4.0);
		if (cross)
			glColor3fv(bright_g);
		glVertex2f(2.0,-3.0);
		glEnd();
		glEndList();
	}
	else
		glCallList(cross_lh[cross]);

	if (!primary_lh[primary])
	{
		primary_lh[primary]=glGenLists(1);
		glNewList(primary_lh[primary], GL_COMPILE_AND_EXECUTE);
		//left primary bar
		glBegin(GL_POLYGON);
		if (primary==0)
			glColor4fv(dark_g);
		else
			glColor4fv(bright_g);
		glVertex3f( -5.5f,  -5.0f, 0.0f);
		glVertex3f( -6.5f,  -7.5f, 0.0f);
		glColor4fv(dark_g);
		glVertex3f( -10.0f, -8.7f, 0.0f);
		glVertex3f( -10.0f, -7.0f, 0.0f);
		glEnd();
		glBegin(GL_POLYGON);
		glColor4fv(dark_g);
		glVertex3f( -10.0f, -8.7f, 0.0f);
		glVertex3f( -10.0f, -7.0f, 0.0f);
		if (primary!=2)
			glColor4fv(dark_g);
		else
			glColor4fv(bright_g);
		glVertex3f( -15.0f, -8.5f, 0.0f);
		glVertex3f( -15.0f, -9.5f, 0.0f);
		glEnd();
		//right primary bar
		glBegin(GL_POLYGON);
		if (primary==0)
			glColor4fv(dark_g);
		else
			glColor4fv(bright_g);
		glVertex3f( 5.5f,  -5.0f, 0.0f);
		glVertex3f( 6.5f,  -7.5f, 0.0f);
		glColor4fv(dark_g);
		glVertex3f( 10.0f, -8.7f, 0.0f);
		glVertex3f( 10.0f, -7.0f, 0.0f);
		glEnd();
		glBegin(GL_POLYGON);
		glColor4fv(dark_g);
		glVertex3f( 10.0f, -8.7f, 0.0f);
		glVertex3f( 10.0f, -7.0f, 0.0f);
		if (primary!=2)
			glColor4fv(dark_g);
		else
			glColor4fv(bright_g);
		glVertex3f( 15.0f, -8.5f, 0.0f);
		glVertex3f( 15.0f, -9.5f, 0.0f);
		glEnd();

		glEndList();
	}
	else
		glCallList(primary_lh[primary]);

	if (!secondary_lh[secondary])
	{
		secondary_lh[secondary]=glGenLists(1);
		glNewList(secondary_lh[secondary], GL_COMPILE_AND_EXECUTE);
		if (secondary<=2){
			//left secondary
			if (secondary!=1)
				glColor4fv(dark_g);
			else
				glColor4fv(bright_g);
			ogl_drawcircle2(16,GL_LINE_LOOP,2.0,-10.0,2.0,-2.0);
			//right secondary
			if (secondary!=2)
				glColor4fv(dark_g);
			else
				glColor4fv(bright_g);
			ogl_drawcircle2(16,GL_LINE_LOOP,2.0,10.0,2.0,-2.0);
		}
		else
		{
			//bottom/middle secondary
			if (secondary!=4)
				glColor4fv(dark_g);
			else
				glColor4fv(bright_g);
			ogl_drawcircle2(16,GL_LINE_LOOP,2.0,0.0,2.0,-8.0);
		}
		glEndList();
	}
	else
		glCallList(secondary_lh[secondary]);

	glPopMatrix();
}

int g3_draw_sphere(g3s_point *pnt,fix rad){
	int c;
	float scale = ((float)grd_curcanv->cv_bitmap.bm_w/grd_curcanv->cv_bitmap.bm_h);

	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glPushMatrix();
	glTranslatef(f2glf(pnt->p3_vec.x),f2glf(pnt->p3_vec.y),-f2glf(pnt->p3_vec.z));
	if (scale >= 1)
	{
		rad/=scale;
		glScalef(f2glf(rad),f2glf(rad*scale),f2glf(rad));
	}
	else
	{
		rad*=scale;
		glScalef(f2glf(rad/scale),f2glf(rad),f2glf(rad));
	}
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
	}else{
		glDepthMask(GL_FALSE);
		glColor4f(PAL2Tr(c), PAL2Tg(c), PAL2Tb(c), 1.0 - (float)Gr_scanline_darkening_level / ((float)GR_FADE_LEVELS - 1.0));
	}
	glBegin(GL_TRIANGLE_FAN);
	for (c=0;c<nv;c++){
		glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
	}
	glEnd();
	glDepthMask(GL_TRUE);
	return 0;
}

void gr_upoly_tmap(int nverts, int *vert ){
		glmprintf((0,"gr_upoly_tmap: unhandled\n"));//should never get called
}

void draw_tmap_flat(grs_bitmap *bm,int nv,g3s_point **vertlist){
		glmprintf((0,"draw_tmap_flat: unhandled\n"));//should never get called
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
		glmprintf((0,"g3_draw_tmap: unhandled tmap_drawer %p\n",tmap_drawer_ptr));
	}

	return 0;
}

bool g3_draw_tmap_2(int nv, g3s_point **pointlist, g3s_uvl *uvl_list, grs_bitmap *bmbot, grs_bitmap *bm, int orient)
{
	int c;
	float l,u1,v1;

	g3_draw_tmap(nv,pointlist,uvl_list,bmbot);//draw the bottom texture first.. could be optimized with multitexturing..

	r_tpolyc++;
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
			l=f2fl(uvl_list[c].l);
		}
		glColor3f(l,l,l);
		glTexCoord2f(u1,v1);
		glVertex3f(f2glf(pointlist[c]->p3_vec.x),f2glf(pointlist[c]->p3_vec.y),-f2glf(pointlist[c]->p3_vec.z));
	}
	glEnd();
	return 0;
}

bool g3_draw_bitmap(vms_vector *pos,fix width,fix height,grs_bitmap *bm, object *obj)
{
	vms_vector pv,v1;
	int i;
	r_bitmapc++;
	v1.z=0;
 
	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);
	glDepthMask(GL_FALSE);
 
	if (Endlevel_sequence)
		glDepthFunc(GL_ALWAYS);
 
	glBegin(GL_QUADS);

	// Define alpha by looking for object TYPE or ID. We do this here so we have it seperated from the rest of the code.
	if (PlayerCfg.OglAlphaEffects && // if -gl_transparency draw following bitmaps
		(obj->type==OBJ_FIREBALL || // all types of explosions and energy-effects
		(obj->type==OBJ_WEAPON && obj->id != PROXIMITY_ID) || // weapon fire except bombs
		obj->id==POW_EXTRA_LIFE || // extra life
		obj->id==POW_ENERGY || // energy powerup
		obj->id==POW_SHIELD_BOOST || // shield boost
		obj->id==POW_CLOAK || // cloak
		obj->id==POW_INVULNERABILITY)) // invulnerability
		glColor4f(1.0,1.0,1.0,0.6); // ... with 0.6 alpha
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
 
		if (obj->id == 5 && obj->type == 1) // create small z-Offset for missile exploding effect - prevents ugly wall-clipping
			pv.z -= F1_0;
 
		glVertex3f(f2glf(pv.x),f2glf(pv.y),-f2glf(pv.z));
	}
	glEnd();
	glDepthMask(GL_TRUE);
 
	return 0;
}

bool ogl_ubitmapm_c(int x, int y,grs_bitmap *bm,int c)
{
	GLfloat xo,yo,xf,yf;
	GLfloat u1,u2,v1,v2;
	x+=grd_curcanv->cv_bitmap.bm_x;
	y+=grd_curcanv->cv_bitmap.bm_y;
	xo=x/(float)last_width;
	xf=(bm->bm_w+x)/(float)last_width;
	yo=1.0-y/(float)last_height;
	yf=1.0-(bm->bm_h+y)/(float)last_height;

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);
	
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
bool ogl_ubitmapm(int x, int y,grs_bitmap *bm){
	return ogl_ubitmapm_c(x,y,bm,-1);
}

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
	
	ogl_texwrap(&tex,GL_CLAMP_TO_EDGE);

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

GLubyte *pixels = NULL;

void ogl_start_frame(void){
	r_polyc=0;r_tpolyc=0;r_bitmapc=0;r_ubitbltc=0;r_upixelc=0;

	OGL_VIEWPORT(grd_curcanv->cv_bitmap.bm_x,grd_curcanv->cv_bitmap.bm_y,Canvas_width,Canvas_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glLineWidth(linedotscale);
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

void gr_flip(void)
{
	if (GameArg.DbgRenderStats)
		ogl_texture_stats();

	ogl_do_palfx();
	ogl_swap_buffers_internal();
	glClear(GL_COLOR_BUFFER_BIT);
}

int tex_format_supported(int iformat,int format){
	switch (iformat){
		case GL_INTENSITY4:
			if (!GameArg.DbgGlIntensity4Ok) return 0; break;
		case GL_LUMINANCE4_ALPHA4:
			if (!GameArg.DbgGlLuminance4Alpha4Ok) return 0; break;
		case GL_RGBA2:
			if (!GameArg.DbgGlRGBA2Ok) return 0; break;
	}
	if (GameArg.DbgGlGetTexLevelParamOk){
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
	int x,y,c,i;

	if ((width > max(grd_curscreen->sc_w, 1024)) || (height > max(grd_curscreen->sc_h, 256)))
		Error("Texture is too big: %ix%i", width, height);

	i=0;
	for (y=0;y<theight;y++)
	{
		i=dxo+truewidth*(y+dyo);
		for (x=0;x<twidth;x++)
		{
			if (x<width && y<height)
			{
				if (data_format)
				{
					int j;

					for (j = 0; j < data_format; ++j)
						(*(texp++)) = data[i * data_format + j];
					i++;
					continue;
				}
				else
				{
					c = data[i++];
				}
			}
			else
			{
				c = 256; // fill the pad space with transparency (or blackness)
			}

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
				switch (type)
				{
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
			}
			else
			{
				switch (type)
				{
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
						(*(texp++))=ogl_pal[c*3]*4;
						(*(texp++))=ogl_pal[c*3+1]*4;
						(*(texp++))=ogl_pal[c*3+2]*4;
						(*(texp++))=255;//not transparent
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
				if (GameArg.DbgGlLuminance4Alpha4Ok){
					tex->internalformat=GL_LUMINANCE4_ALPHA4;
					tex->format=GL_LUMINANCE_ALPHA;
					break;
				}//note how it will fall through here if the statement is false
			case GL_LUMINANCE4_ALPHA4:
				if (GameArg.DbgGlRGBA2Ok){
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
				glmprintf((0,"...no tex format to fall back on\n"));
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
	if (GameArg.DbgGlGetTexLevelParamOk){
		GLint t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_LUMINANCE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTENSITY_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_RED_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_GREEN_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_BLUE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_ALPHA_SIZE,&t);a+=t;
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

	//calculate u/v values that would make the resulting texture correctly sized
	tex->u = (float) ((double) tex->w / (double) tex->tw);
	tex->v = (float) ((double) tex->h / (double) tex->th);

	if (data) {
		if (bm_flags >= 0)
			ogl_filltexbuf (data, texbuf, tex->lw, tex->w, tex->h, dxo, dyo, tex->tw, tex->th, 
								 tex->format, bm_flags, data_format);
		else {
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
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OglTexMagFilt);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OglTexMinFilt);
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
			con_printf(CON_DEBUG,"%s: %ux%ux%i p=%i(%i) c=%i a=%i chans=%i\n", filename, pdata.width, pdata.height, pdata.depth, pdata.paletted, pdata.num_palette, pdata.color, pdata.alpha, pdata.channels);
			if (pdata.depth == 8 && pdata.color)
			{
				if (bm->gltexture == NULL)
					ogl_init_texture(bm->gltexture = ogl_get_free_texture(), pdata.width, pdata.height, flags | ((pdata.alpha || bm->bm_flags & BM_FLAG_TRANSPARENT) ? OGL_FLAG_ALPHA : 0));
				ogl_loadtexture(pdata.data, 0, 0, bm->gltexture, bm->bm_flags, pdata.paletted ? 0 : pdata.channels);
				d_free(pdata.data);
				if (pdata.palette)
					d_free(pdata.palette);
				return;
			}
			else
			{
				con_printf(CON_DEBUG,"%s: unsupported texture format: must be rgb, rgba, or paletted, and depth 8\n", filename);
				d_free(pdata.data);
				if (pdata.palette)
					d_free(pdata.palette);
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
	}
}

bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale) // to scale bitmaps
{
	GLfloat xo,yo,xf,yf;
	GLfloat u1,u2,v1,v2;
	GLdouble h,a;
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

	a = (double) grd_curscreen->sc_w / (double) grd_curscreen->sc_h;
	h = (double) scale / (double) F1_0;

	xo = x / ((double) last_width * h);
	xf = (dw + x) / ((double) last_width * h);
	yo = 1.0 - y / ((double) last_height * h);
	yf = 1.0 - (dh + y) / ((double) last_height * h);

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);
	
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
