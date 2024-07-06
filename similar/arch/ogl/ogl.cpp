/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Graphics support functions for OpenGL.
 *
 */

#include "dxxsconf.h"
#include <bit>
#include <stdexcept>
#include <tuple>
#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#if DXX_USE_OGLES
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "3d.h"
#include "piggy.h"
#include "common/3d/globvars.h"
#include "dxxerror.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "console.h"
#include "config.h"
#include "u_mem.h"

#include "segment.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "laser.h"
#include "player.h"
#include "robot.h"
#include "gamefont.h"
#include "byteutil.h"
#include "internal.h"
#include "gauges.h"
#include "object.h"
#include "args.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_zip.h"
#include "partial_range.h"

#include <algorithm>
#include <memory>
#include <utility>
using std::max;

//change to 1 for lots of spew.
#if 0
#define glmprintf(A) con_printf A
#else
#define glmprintf(A)
#endif

#ifndef M_PI
#define M_PI 3.14159
#endif

namespace {

static constexpr float PAL2Tr(const color_palette_index c)
{
	return PAL2T(c).r / 63.0f;
}

static constexpr float PAL2Tg(const color_palette_index c)
{
	return PAL2T(c).g / 63.0f;
}

static constexpr float PAL2Tb(const color_palette_index c)
{
	return PAL2T(c).b / 63.0f;
}

template <unsigned G>
struct enable_ogl_client_state
{
	enable_ogl_client_state() noexcept
	{
		glEnableClientState(G);
	}
	~enable_ogl_client_state() noexcept
	{
		glDisableClientState(G);
	}
};

template <typename T, unsigned... Gs>
using ogl_client_states = std::tuple<T, enable_ogl_client_state<Gs>...>;

template <typename T, std::size_t N1, std::size_t N2>
union flatten_array
{
	std::array<T, N1 * N2> flat;
	std::array<std::array<T, N1>, N2> nested;
	static_assert(sizeof(flat) == sizeof(nested), "array padding error");
};

}

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__)) || defined(__sun__) || defined(macintosh)
#define cosf(a) cos(a)
#define sinf(a) sin(a)
#endif

namespace dcx {

static std::unique_ptr<GLubyte[]> texbuf;

unsigned last_width=~0u,last_height=~0u;
int GL_TEXTURE_2D_enabled=-1;

static int r_texcount = 0, r_cachedtexcount = 0;
#if DXX_USE_OGLES
static int ogl_rgba_internalformat = GL_RGBA;
static int ogl_rgb_internalformat = GL_RGB;
#else
static int ogl_rgba_internalformat = GL_RGBA8;
static int ogl_rgb_internalformat = GL_RGB8;
#endif
static std::unique_ptr<GLfloat[]> sphere_va, circle_va, disk_va;
static std::array<std::unique_ptr<GLfloat[]>, 3> secondary_lva;
static int r_polyc,r_tpolyc,r_bitmapc,r_ubitbltc;
#define f2glf(x) (f2fl(x))

#define OGL_BINDTEXTURE(a) glBindTexture(GL_TEXTURE_2D, a);

/* I assume this ought to be >= MAX_BITMAP_FILES in piggy.h? */
static std::array<ogl_texture, 20000> ogl_texture_list;
static int ogl_texture_list_cur;

/* some function prototypes */

#define GL_TEXTURE0_ARB 0x84C0
static int ogl_loadtexture(const palette_array_t &, const uint8_t *data, int dxo, int dyo, ogl_texture &tex, int bm_flags, int data_format, opengl_texture_filter texfilt, bool texanis, bool edgepad) __attribute_nonnull();
static void ogl_freetexture(ogl_texture &gltexture);

static void ogl_loadbmtexture(grs_bitmap &bm, bool edgepad)
{
	ogl_loadbmtexture_f(bm, CGameCfg.TexFilt, CGameCfg.TexAnisotropy, edgepad);
}

}

#if DXX_USE_OGLES
// Replacement for gluPerspective
static void perspective(double fovy, double aspect, double zNear, double zFar)
{
	double xmin, xmax, ymin, ymax;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustumf(xmin, xmax, ymin, ymax, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	
	glDepthMask(GL_TRUE);
}
#endif

namespace dcx {

static void ogl_init_texture_stats(ogl_texture &t)
{
	t.prio=0.3;//default prio
	t.numrend=0;
}

void ogl_init_texture(ogl_texture &t, int w, int h, int flags)
{
	t.handle = 0;
#if !DXX_USE_OGLES
	if (flags & OGL_FLAG_NOCOLOR)
	{
		// use GL_INTENSITY instead of GL_RGB
		if (flags & OGL_FLAG_ALPHA)
		{
			if (CGameArg.DbgGlIntensity4Ok)
			{
				t.internalformat = GL_INTENSITY4;
				t.format = GL_LUMINANCE;
			}
			else if (CGameArg.DbgGlLuminance4Alpha4Ok)
			{
				t.internalformat = GL_LUMINANCE4_ALPHA4;
				t.format = GL_LUMINANCE_ALPHA;
			}
			else if (CGameArg.DbgGlRGBA2Ok)
			{
				t.internalformat = GL_RGBA2;
				t.format = GL_RGBA;
			}
			else
			{
				t.internalformat = ogl_rgba_internalformat;
				t.format = GL_RGBA;
			}
		}
		else
		{
			// there are certainly smaller formats we could use here, but nothing needs it ATM.
			t.internalformat = ogl_rgb_internalformat;
			t.format = GL_RGB;
		}
	}
	else
	{
#endif
		if (flags & OGL_FLAG_ALPHA)
		{
			t.internalformat = ogl_rgba_internalformat;
			t.format = GL_RGBA;
		}
		else
		{
			t.internalformat = ogl_rgb_internalformat;
			t.format = GL_RGB;
		}
#if !DXX_USE_OGLES
	}
#endif
	t.wrapstate = -1;
	t.lw = t.w = w;
	t.h = h;
	ogl_init_texture_stats(t);
}

static void ogl_reset_texture(ogl_texture &t)
{
	ogl_init_texture(t, 0, 0, 0);
}

static void ogl_reset_texture_stats_internal(void){
	range_for (auto &i, ogl_texture_list)
		if (i.handle>0)
			ogl_init_texture_stats(i);
}

void ogl_init_texture_list_internal(void){
	ogl_texture_list_cur=0;
	range_for (auto &i, ogl_texture_list)
		ogl_reset_texture(i);
}

void ogl_smash_texture_list_internal(void){
	sphere_va.reset();
	circle_va.reset();
	disk_va.reset();
	secondary_lva = {};
	range_for (auto &i, ogl_texture_list)
	{
		if (i.handle>0){
			glDeleteTextures( 1, &i.handle );
			i.handle=0;
		}
		i.wrapstate = -1;
	}
}

ogl_texture* ogl_get_free_texture(void){
	for (unsigned i = ogl_texture_list.size(); i--;)
	{
		if (ogl_texture_list[ogl_texture_list_cur].handle<=0 && ogl_texture_list[ogl_texture_list_cur].w==0)
			return &ogl_texture_list[ogl_texture_list_cur];
		if (++ogl_texture_list_cur >= ogl_texture_list.size())
			ogl_texture_list_cur=0;
	}
	throw std::runtime_error("OpenGL: texture list full");
}

static void ogl_texture_stats(grs_canvas &canvas)
{
	int used = 0, usedother = 0, usedidx = 0, usedrgb = 0, usedrgba = 0;
	int databytes = 0, truebytes = 0;
	GLint idx, r, g, b, a, dbl, depth;
	int res, colorsize, depthsize;
	range_for (auto &i, ogl_texture_list)
	{
		if (i.handle>0){
			used++;
			databytes+=i.bytesu;
			truebytes+=i.bytes;
			if (i.format == GL_RGBA)
				usedrgba++;
			else if (i.format == GL_RGB)
				usedrgb++;
#if !DXX_USE_OGLES
			else if (i.format == GL_COLOR_INDEX)
				usedidx++;
#endif
			else
				usedother++;
		}
	}

	res = SWIDTH * SHEIGHT;
#if !DXX_USE_OGLES
	glGetIntegerv(GL_INDEX_BITS, &idx);
#else
	idx=16;
#endif
	glGetIntegerv(GL_RED_BITS, &r);
	glGetIntegerv(GL_GREEN_BITS, &g);
	glGetIntegerv(GL_BLUE_BITS, &b);
	glGetIntegerv(GL_ALPHA_BITS, &a);
#if !DXX_USE_OGLES
	glGetIntegerv(GL_DOUBLEBUFFER, &dbl);
#else
	dbl=1;
#endif
	dbl += 1;
	glGetIntegerv(GL_DEPTH_BITS, &depth);
	const auto &game_font = *GAME_FONT;
	gr_set_fontcolor(canvas, BM_XRGB(255, 255, 255), -1);
	colorsize = (idx * res * dbl) / 8;
	depthsize = res * depth / 8;
	const auto &&fspacx2 = FSPACX(2);
	const auto &&fspacy1 = FSPACY(1);
	const auto &&line_spacing = LINE_SPACING(game_font, game_font);
	gr_printf(canvas, game_font, fspacx2, fspacy1, "%i flat %i tex %i bitmaps", r_polyc, r_tpolyc, r_bitmapc);
	gr_printf(canvas, game_font, fspacx2, fspacy1 + line_spacing, "%i(%i,%i,%i,%i) %iK(%iK wasted) (%i postcachedtex)", used, usedrgba, usedrgb, usedidx, usedother, truebytes / 1024, (truebytes - databytes) / 1024, r_texcount - r_cachedtexcount);
	gr_printf(canvas, game_font, fspacx2, fspacy1 + (line_spacing * 2), "%ibpp(r%i,g%i,b%i,a%i)x%i=%iK depth%i=%iK", idx, r, g, b, a, dbl, colorsize / 1024, depth, depthsize / 1024);
	gr_printf(canvas, game_font, fspacx2, fspacy1 + (line_spacing * 3), "total=%iK", (colorsize + depthsize + truebytes) / 1024);
}

}

static void ogl_bindbmtex(grs_bitmap &bm, bool edgepad){
	if (bm.gltexture==NULL || bm.gltexture->handle<=0)
		ogl_loadbmtexture(bm, edgepad);
	OGL_BINDTEXTURE(bm.gltexture->handle);
	bm.gltexture->numrend++;
}

//gltexture MUST be bound first
static void ogl_texwrap(ogl_texture *const gltexture, const int state)
{
	if (gltexture->wrapstate != state || gltexture->numrend < 1)
	{
		gltexture->wrapstate = state;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
	}
}

//crude texture precaching
//handles: powerups, walls, weapons, polymodels, etc.
//it is done with the horrid do_special_effects kludge so that sides that have to be texmerged and have animated textures will be correctly cached.
//similarly, with the objects(esp weapons), we could just go through and cache em all instead, but that would get ones that might not even be on the level
//TODO: doors

namespace dsx {

void ogl_cache_polymodel_textures(const polygon_model_index model_num)
{
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	if (model_num == polygon_model_index::None)
		return;
	const auto &po = Polygon_models[model_num];
	unsigned i = po.first_texture;
	const unsigned last_texture = i + po.n_textures;
	for (; i != last_texture; ++i)
	{
		const auto objbitmap = ObjBitmaps[ObjBitmapPtrs[i]];
		PIGGY_PAGE_IN(objbitmap);
		ogl_loadbmtexture(GameBitmaps[objbitmap], 1);
	}
}

}

static void ogl_cache_vclip_textures(const vclip &vc)
{
	for (const auto i : partial_const_range(vc.frames, vc.num_frames))
	{
		PIGGY_PAGE_IN(i);
		ogl_loadbmtexture(GameBitmaps[i], 0);
	}
}

static void ogl_cache_vclipn_textures(const d_vclip_array &Vclip, const vclip_index i)
{
	if (Vclip.valid_index(i))
		ogl_cache_vclip_textures(Vclip[i]);
}

static void ogl_cache_weapon_textures(const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, const unsigned weapon_type)
{
	if (weapon_type >= Weapon_info.size())
		return;
	const auto &w = Weapon_info[weapon_type];
	ogl_cache_vclipn_textures(Vclip, w.flash_vclip);
	ogl_cache_vclipn_textures(Vclip, w.robot_hit_vclip);
	ogl_cache_vclipn_textures(Vclip, w.wall_hit_vclip);
	if (w.render == WEAPON_RENDER_VCLIP)
		ogl_cache_vclipn_textures(Vclip, w.weapon_vclip);
	else if (w.render == WEAPON_RENDER_POLYMODEL)
	{
		ogl_cache_polymodel_textures(w.model_num);
		ogl_cache_polymodel_textures(w.model_num_inner);
	}
}

namespace dsx {

void ogl_cache_level_textures(void)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	int max_efx=0,ef;
	
	ogl_reset_texture_stats_internal();//loading a new lev should reset textures
	
	range_for (auto &ec, partial_const_range(Effects, Num_effects))
	{
		ogl_cache_vclipn_textures(Vclip, ec.dest_vclip);
		if (ec.changing_wall_texture == -1 && ec.changing_object_texture == object_bitmap_index::None)
			continue;
		if (ec.vc.num_frames>max_efx)
			max_efx=ec.vc.num_frames;
	}
	glmprintf((CON_DEBUG, "max_efx:%i", max_efx));
	for (ef=0;ef<max_efx;ef++){
		range_for (eclip &ec, partial_range(Effects, Num_effects))
		{
			if (ec.changing_wall_texture == -1 && ec.changing_object_texture == object_bitmap_index::None)
				continue;
			ec.time_left=-1;
		}
		do_special_effects();

		range_for (const unique_segment &seg, vcsegptr)
		{
			range_for (auto &side, seg.sides)
			{
				const auto tmap1 = side.tmap_num;
				const auto tmap2 = side.tmap_num2;
				const auto tmap1idx = get_texture_index(tmap1);
				if (tmap1idx >= NumTextures){
					glmprintf((CON_DEBUG, "ogl_cache_level_textures %p %p %i %i", seg.get_unchecked_pointer(), &side, tmap1, NumTextures));
					//				tmap1=0;
					continue;
				}
				const auto texture1 = Textures[tmap1idx];
				PIGGY_PAGE_IN(texture1);
				grs_bitmap *bm = &GameBitmaps[texture1];
				if (tmap2 != texture2_value::None)
				{
					const auto texture2 = Textures[get_texture_index(tmap2)];
					PIGGY_PAGE_IN(texture2);
					auto &bm2 = GameBitmaps[texture2];
					if (CGameArg.DbgUseOldTextureMerge || bm2.get_flag_mask(BM_FLAG_SUPER_TRANSPARENT))
						bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
					else {
						ogl_loadbmtexture(bm2, 1);
					}
				}
				ogl_loadbmtexture(*bm, 0);
			}
		}
		glmprintf((CON_DEBUG, "finished ef:%i", ef));
	}
	reset_special_effects();
	init_special_effects();
	{
		auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
		// always have lasers, concs, flares.  Always shows player appearance, and at least concs are always available to disappear.
		ogl_cache_weapon_textures(Vclip, Weapon_info, Primary_weapon_to_weapon_info[primary_weapon_index_t::LASER_INDEX]);
		ogl_cache_weapon_textures(Vclip, Weapon_info, Secondary_weapon_to_weapon_info[secondary_weapon_index_t::CONCUSSION_INDEX]);
		ogl_cache_weapon_textures(Vclip, Weapon_info, weapon_id_type::FLARE_ID);
		ogl_cache_vclipn_textures(Vclip, vclip_index::player_appearance);
		ogl_cache_vclipn_textures(Vclip, vclip_index::powerup_disappearance);
		ogl_cache_polymodel_textures(Player_ship->model_num);
		ogl_cache_vclipn_textures(Vclip, Player_ship->expl_vclip_num);

		range_for (const auto &&objp, vcobjptridx)
		{
			if (objp->type == OBJ_POWERUP && objp->render_type == render_type::RT_POWERUP)
			{
				ogl_cache_vclipn_textures(Vclip, objp->rtype.vclip_info.vclip_num);
				const auto id = get_powerup_id(objp);
				primary_weapon_index_t p;
				secondary_weapon_index_t s;
				int w;
				if (
					(
						(
							(id == powerup_type_t::POW_VULCAN_WEAPON && (p = primary_weapon_index_t::VULCAN_INDEX, true)) ||
							(id == powerup_type_t::POW_SPREADFIRE_WEAPON && (p = primary_weapon_index_t::SPREADFIRE_INDEX, true)) ||
							(id == powerup_type_t::POW_PLASMA_WEAPON && (p = primary_weapon_index_t::PLASMA_INDEX, true)) ||
							(id == powerup_type_t::POW_FUSION_WEAPON && (p = primary_weapon_index_t::FUSION_INDEX, true))
						) && (w = Primary_weapon_to_weapon_info[p], true)
					) ||
					(
						(
							(id == powerup_type_t::POW_PROXIMITY_WEAPON && (s = secondary_weapon_index_t::PROXIMITY_INDEX, true)) ||
							((id == powerup_type_t::POW_HOMING_AMMO_1 || id == powerup_type_t::POW_HOMING_AMMO_4) && (s = secondary_weapon_index_t::HOMING_INDEX, true)) ||
							(id == powerup_type_t::POW_SMARTBOMB_WEAPON && (s = secondary_weapon_index_t::SMART_INDEX, true)) ||
							(id == powerup_type_t::POW_MEGA_WEAPON && (s = secondary_weapon_index_t::MEGA_INDEX, true))
						) && (w = Secondary_weapon_to_weapon_info[s], true)
					)
				)
				{
					ogl_cache_weapon_textures(Vclip, Weapon_info, w);
				}
			}
			else if (objp->type != OBJ_NONE && objp->render_type == render_type::RT_POLYOBJ)
			{
				if (objp->type == OBJ_ROBOT)
				{
					auto &ri = Robot_info[get_robot_id(objp)];
					ogl_cache_vclipn_textures(Vclip, ri.exp1_vclip_num);
					ogl_cache_vclipn_textures(Vclip, ri.exp2_vclip_num);
					ogl_cache_weapon_textures(Vclip, Weapon_info, ri.weapon_type);
				}
				if (objp->rtype.pobj_info.tmap_override != -1)
				{
					const auto t = Textures[objp->rtype.pobj_info.tmap_override];
					PIGGY_PAGE_IN(t);
					ogl_loadbmtexture(GameBitmaps[t], 1);
				}
				else
					ogl_cache_polymodel_textures(objp->rtype.pobj_info.model_num);
			}
		}
	}
	glmprintf((CON_DEBUG, "finished caching"));
	r_cachedtexcount = r_texcount;
}

}

namespace dcx {

void g3_draw_line(const g3_draw_line_context &context, const g3s_point &p0, const g3s_point &p1)
{
	ogl_client_states<int, GL_VERTEX_ARRAY, GL_COLOR_ARRAY> cs;
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	std::array<GLfloat, 6> vertices = {{
		f2glf(p0.p3_vec.x), f2glf(p0.p3_vec.y), -f2glf(p0.p3_vec.z),
		f2glf(p1.p3_vec.x), f2glf(p1.p3_vec.y), -f2glf(p1.p3_vec.z)
	}};
	glVertexPointer(3, GL_FLOAT, 0, vertices.data());
	glColorPointer(4, GL_FLOAT, 0, context.color_array.data());
	glDrawArrays(GL_LINES, 0, 2);
}

static void ogl_drawcircle(const unsigned nsides, const unsigned type, GLfloat *const vertices)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glDrawArrays(type, 0, nsides);
	glDisableClientState(GL_VERTEX_ARRAY);
}

static std::unique_ptr<GLfloat[]> circle_array_init(const unsigned nsides)
{
	auto vertices = std::make_unique<GLfloat[]>(nsides * 2);
	for (unsigned i = 0; i < nsides; i++)
	{
		const float ang = 2.0 * M_PI * i / nsides;
		vertices[i * 2] = cosf(ang);
		vertices[i * 2 + 1] = sinf(ang);
	}
	return vertices;
}

static std::unique_ptr<GLfloat[]> circle_array_init_2(const unsigned nsides, const float xsc, const float xo, const float ysc, const float yo)
{
	auto vertices = std::make_unique<GLfloat[]>(nsides * 2);
	for (unsigned i = 0; i < nsides; i++)
	{
		const float ang = 2.0 * M_PI * i / nsides;
		vertices[i * 2] = cosf(ang) * xsc + xo;
		vertices[i * 2 + 1] = sinf(ang) * ysc + yo;
	}
	return vertices;
}

void ogl_draw_vertex_reticle(grs_canvas &canvas, int cross, int primary, int secondary, int color, int alpha, int size_offs)
{
	int size=270+(size_offs*20);
	float scale = (static_cast<float>(SWIDTH)/SHEIGHT);
	const std::array<float, 4> ret_rgba{{
		PAL2Tr(color),
		PAL2Tg(color),
		PAL2Tb(color),
		float{1.0f - (static_cast<float>(alpha) / (static_cast<float>(GR_FADE_LEVELS)))}
	}}, ret_dark_rgba{{
		ret_rgba[0] / 2,
		ret_rgba[1] / 2,
		ret_rgba[2] / 2,
		ret_rgba[3] / 2
	}};
	std::array<GLfloat, 16 * 4> dark_lca, bright_lca;
	for (uint_fast32_t i = 0; i != dark_lca.size(); i += 4)
	{
		bright_lca[i] = ret_rgba[0];
		dark_lca[i] = ret_dark_rgba[0];
		bright_lca[i+1] = ret_rgba[1];
		dark_lca[i+1] = ret_dark_rgba[1];
		bright_lca[i+2] = ret_rgba[2];
		dark_lca[i+2] = ret_dark_rgba[2];
		bright_lca[i+3] = ret_rgba[3];
		dark_lca[i+3] = ret_dark_rgba[3];
	}

	glPushMatrix();
	glTranslatef((canvas.cv_bitmap.bm_w / 2 + canvas.cv_bitmap.bm_x) / static_cast<float>(last_width), 1.0 - (canvas.cv_bitmap.bm_h / 2 + canvas.cv_bitmap.bm_y) / static_cast<float>(last_height), 0);

	{
		float gl1, gl2, gl3;
	if (scale >= 1)
	{
		size/=scale;
		gl2 = f2glf(size*scale);
		gl1 = f2glf(size);
		gl3 = gl1;
	}
	else
	{
		size*=scale;
		gl1 = f2glf(size/scale);
		gl2 = f2glf(size);
		gl3 = gl2;
	}
		glScalef(gl1, gl2, gl3);
	}

	glLineWidth(linedotscale*2);
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	//cross
	std::array<GLfloat, 8 * 4> cross_lca;
	GLfloat *cross_lca_ptr;
	if (cross)
	{
		for (uint_fast32_t i = 0; i != cross_lca.size(); i += 8)
		{
			cross_lca[i] = ret_dark_rgba[0];
			cross_lca[i+1] = ret_dark_rgba[1];
			cross_lca[i+2] = ret_dark_rgba[2];
			cross_lca[i+3] = ret_dark_rgba[3];
			cross_lca[i+4] = ret_rgba[0];
			cross_lca[i+5] = ret_rgba[1];
			cross_lca[i+6] = ret_rgba[2];
			cross_lca[i+7] = ret_rgba[3];
		}
		cross_lca_ptr = cross_lca.data();
	}
	else
	{
		cross_lca_ptr = dark_lca.data();
	}
	glColorPointer(4, GL_FLOAT, 0, cross_lca_ptr);

	static const std::array<GLfloat, 8 * 2> cross_lva{{
		-4.0, 2.0, -2.0, 0, -3.0, -4.0, -2.0, -3.0, 4.0, 2.0, 2.0, 0, 3.0, -4.0, 2.0, -3.0,
	}};
	glVertexPointer(2, GL_FLOAT, 0, cross_lva.data());
	glDrawArrays(GL_LINES, 0, 8);
	
	std::array<GLfloat, 4 * 4> primary_lca0;
	GLfloat *lca0_data;
	//left primary bar
	if(primary == 0)
		lca0_data = dark_lca.data();
	else
	{
		primary_lca0[0] = primary_lca0[4] = ret_rgba[0];
		primary_lca0[1] = primary_lca0[5] = ret_rgba[1];
		primary_lca0[2] = primary_lca0[6] = ret_rgba[2];
		primary_lca0[3] = primary_lca0[7] = ret_rgba[3];
		primary_lca0[8] = primary_lca0[12] = ret_dark_rgba[0];
		primary_lca0[9] = primary_lca0[13] = ret_dark_rgba[1];
		primary_lca0[10] = primary_lca0[14] = ret_dark_rgba[2];
		primary_lca0[11] = primary_lca0[15] = ret_dark_rgba[3];
		lca0_data = primary_lca0.data();
	}
	glColorPointer(4, GL_FLOAT, 0, lca0_data);
	static const std::array<GLfloat, 4 * 2> primary_lva0{{
		-5.5, -5.0, -6.5, -7.5, -10.0, -7.0, -10.0, -8.7
	}};
	static const std::array<GLfloat, 4 * 2> primary_lva1{{
		-10.0, -7.0, -10.0, -8.7, -15.0, -8.5, -15.0, -9.5
	}};
	static const std::array<GLfloat, 4 * 2> primary_lva2{{
		5.5, -5.0, 6.5, -7.5, 10.0, -7.0, 10.0, -8.7
	}};
	static const std::array<GLfloat, 4 * 2> primary_lva3{{
		10.0, -7.0, 10.0, -8.7, 15.0, -8.5, 15.0, -9.5
	}};
	glVertexPointer(2, GL_FLOAT, 0, primary_lva0.data());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	std::array<GLfloat, 4 * 4> primary_lca1;
	GLfloat *lca1_data;
	if(primary != 2)
		lca1_data = dark_lca.data();
	else
	{
		primary_lca1[8] = primary_lca1[12] = ret_rgba[0];
		primary_lca1[9] = primary_lca1[13] = ret_rgba[1];
		primary_lca1[10] = primary_lca1[14] = ret_rgba[2];
		primary_lca1[11] = primary_lca1[15] = ret_rgba[3];
		primary_lca1[0] = primary_lca1[4] = ret_dark_rgba[0];
		primary_lca1[1] = primary_lca1[5] = ret_dark_rgba[1];
		primary_lca1[2] = primary_lca1[6] = ret_dark_rgba[2];
		primary_lca1[3] = primary_lca1[7] = ret_dark_rgba[3];
		lca1_data = primary_lca1.data();
	}
	glColorPointer(4, GL_FLOAT, 0, lca1_data);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva1.data());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//right primary bar
	glColorPointer(4, GL_FLOAT, 0, lca0_data);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva2.data());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glColorPointer(4, GL_FLOAT, 0, lca1_data);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva3.data());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	GLfloat *secondary_lva_ptr;
	if (secondary<=2){
		//left secondary
		glColorPointer(4, GL_FLOAT, 0, (secondary != 1 ? dark_lca : bright_lca).data());
		if(!secondary_lva[0])
			secondary_lva[0] = circle_array_init_2(16, 2.0, -10.0, 2.0, -2.0);
		ogl_drawcircle(16, GL_LINE_LOOP, secondary_lva[0].get());
		//right secondary
		glColorPointer(4, GL_FLOAT, 0, (secondary != 2 ? dark_lca : bright_lca).data());
		if(!secondary_lva[1])
			secondary_lva[1] = circle_array_init_2(16, 2.0, 10.0, 2.0, -2.0);
		secondary_lva_ptr = secondary_lva[1].get();
	}
	else {
		//bottom/middle secondary
		glColorPointer(4, GL_FLOAT, 0, (secondary != 4 ? dark_lca : bright_lca).data());
		if(!secondary_lva[2])
			secondary_lva[2] = circle_array_init_2(16, 2.0, 0.0, 2.0, -8.0);
		secondary_lva_ptr = secondary_lva[2].get();
	}
	ogl_drawcircle(16, GL_LINE_LOOP, secondary_lva_ptr);
	
	//glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glPopMatrix();
	glLineWidth(linedotscale);
}

/*
 * Stars on heaven in exit sequence, automap objects
 */
void g3_draw_sphere(grs_canvas &canvas, cg3s_point &pnt, fix rad, const uint8_t c)
{
	int i;
	const float scale = (static_cast<float>(canvas.cv_bitmap.bm_w) / canvas.cv_bitmap.bm_h);
	std::array<GLfloat, 20 * 4> color_array;
	const auto color_r{CPAL2Tr(c)};
	const auto color_g{CPAL2Tg(c)};
	const auto color_b{CPAL2Tb(c)};
	for (i = 0; i < 20*4; i += 4)
	{
		color_array[i] = color_r;
		color_array[i+1] = color_g;
		color_array[i+2] = color_b;
		color_array[i+3] = 1.0;
	}
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glTranslatef(f2glf(pnt.p3_vec.x),f2glf(pnt.p3_vec.y),-f2glf(pnt.p3_vec.z));
	GLfloat gl1, gl2;
	if (scale >= 1)
	{
		rad/=scale;
		gl1 = f2glf(rad);
		gl2 = f2glf(rad*scale);
	}
	else
	{
		rad*=scale;
		gl1 = f2glf(rad/scale);
		gl2 = f2glf(rad);
	}
	glScalef(gl1, gl2, f2glf(rad));
	if(!sphere_va)
		sphere_va = circle_array_init(20);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, color_array.data());
	ogl_drawcircle(20, GL_TRIANGLE_FAN, sphere_va.get());
	glDisableClientState(GL_COLOR_ARRAY);
	glPopMatrix();
}

int gr_ucircle(grs_canvas &canvas, const fix xc1, const fix yc1, const fix r1, const uint8_t c)
{
	int nsides;
	OGL_DISABLE(TEXTURE_2D);
	glColor4f(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (canvas.cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0));
	glPushMatrix();
	glTranslatef(
	             (f2fl(xc1) + canvas.cv_bitmap.bm_x + 0.5) / static_cast<float>(last_width),
	             1.0 - (f2fl(yc1) + canvas.cv_bitmap.bm_y + 0.5) / static_cast<float>(last_height),0);
	glScalef(f2fl(r1) / last_width, f2fl(r1) / last_height, 1.0);
	nsides = 10 + 2 * static_cast<int>(M_PI * f2fl(r1) / 19);
	if(!circle_va)
		circle_va = circle_array_init(nsides);
	ogl_drawcircle(nsides, GL_LINE_LOOP, circle_va.get());
	glPopMatrix();
	return 0;
}

int gr_disk(grs_canvas &canvas, const fix x, const fix y, const fix r, const uint8_t c)
{
	int nsides;
	OGL_DISABLE(TEXTURE_2D);
	glColor4f(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (canvas.cv_fade_level >= GR_FADE_OFF) ? 1.0 : 1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0));
	glPushMatrix();
	glTranslatef(
	             (f2fl(x) + canvas.cv_bitmap.bm_x + 0.5) / static_cast<float>(last_width),
	             1.0 - (f2fl(y) + canvas.cv_bitmap.bm_y + 0.5) / static_cast<float>(last_height),0);
	glScalef(f2fl(r) / last_width, f2fl(r) / last_height, 1.0);
	nsides = 10 + 2 * static_cast<int>(M_PI * f2fl(r) / 19);
	if(!disk_va)
		disk_va = circle_array_init(nsides);
	ogl_drawcircle(nsides, GL_TRIANGLE_FAN, disk_va.get());
	glPopMatrix();
	return 0;
}

/*
 * Draw flat-shaded Polygon (Lasers, Drone-arms, Driller-ears)
 */
void _g3_draw_poly(grs_canvas &canvas, const std::span<cg3s_point *const> pointlist, const uint8_t palette_color_index)
{
	if (pointlist.size() > MAX_POINTS_PER_POLY)
		return;
	flatten_array<GLfloat, 4, MAX_POINTS_PER_POLY> color_array;
	flatten_array<GLfloat, 3, MAX_POINTS_PER_POLY> vertices;

	r_polyc++;
	ogl_client_states<int, GL_VERTEX_ARRAY, GL_COLOR_ARRAY> cs;
	OGL_DISABLE(TEXTURE_2D);
	const auto color_r{PAL2Tr(palette_color_index)};
	const auto color_g{PAL2Tg(palette_color_index)};
	const auto color_b{PAL2Tb(palette_color_index)};

	const float color_a = (canvas.cv_fade_level >= GR_FADE_OFF)
		? 1.0
		: 1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0);

	for (auto &&[p, v, c] : zip(
			pointlist,
			unchecked_partial_range(vertices.nested, pointlist.size()),
			unchecked_partial_range(color_array.nested, pointlist.size())
		)
	)
	{
		c[0] = color_r;
		c[1] = color_g;
		c[2] = color_b;
		c[3] = color_a;
		auto &pv = p->p3_vec;
		v[0] = f2glf(pv.x);
		v[1] = f2glf(pv.y);
		v[2] = -f2glf(pv.z);
	}

	glVertexPointer(3, GL_FLOAT, 0, vertices.flat.data());
	glColorPointer(4, GL_FLOAT, 0, color_array.flat.data());
	glDrawArrays(GL_TRIANGLE_FAN, 0, pointlist.size());
}

/*
 * Everything texturemapped (walls, robots, ship)
 */ 
void _g3_draw_tmap(grs_canvas &canvas, const std::span<cg3s_point *const> pointlist, const g3s_uvl *const uvl_list, const g3s_lrgb *const light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr)
{
	GLfloat color_alpha = 1.0;

	ogl_client_states<int, GL_VERTEX_ARRAY, GL_COLOR_ARRAY> cs;
	if (tmap_drawer_ptr == draw_tmap) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		OGL_ENABLE(TEXTURE_2D);
		ogl_bindbmtex(bm, 0);
		ogl_texwrap(bm.gltexture, GL_REPEAT);
		r_tpolyc++;
		color_alpha = (canvas.cv_fade_level >= GR_FADE_OFF) ? 1.0 : (1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0));
	} else if (tmap_drawer_ptr == draw_tmap_flat) {
		OGL_DISABLE(TEXTURE_2D);
		/* for cloaked state faces */
		color_alpha = 1.0 - (static_cast<GLfloat>(canvas.cv_fade_level) / static_cast<GLfloat>(NUM_LIGHTING_LEVELS));
	} else {
		glmprintf((CON_DEBUG, "g3_draw_tmap: unhandled tmap_drawer"));
		return;
	}

	flatten_array<GLfloat, 3, MAX_POINTS_PER_POLY> vertices;
	flatten_array<GLfloat, 4, MAX_POINTS_PER_POLY> color_array;
	flatten_array<GLfloat, 2, MAX_POINTS_PER_POLY> texcoord_array;

	const auto nv = pointlist.size();
	for (auto &&[point, light, uvl, vert, color, texcoord] : zip(
			pointlist,
			unchecked_partial_range(light_rgb, nv),
			unchecked_partial_range(uvl_list, nv),
			unchecked_partial_range(vertices.nested, nv),
			unchecked_partial_range(color_array.nested, nv),
			partial_range(texcoord_array.nested, nv)
		)
	)
	{
		vert[0] = f2glf(point->p3_vec.x);
		vert[1] = f2glf(point->p3_vec.y);
		vert[2] = -f2glf(point->p3_vec.z);
		color[3] = color_alpha;
		if (tmap_drawer_ptr == draw_tmap_flat) {
			color[0] = color[1] = color[2] = 0;
		}
		else
		{
			if (bm.get_flag_mask(BM_FLAG_NO_LIGHTING))
			{
				color[0] = color[1] = color[2] = 1.0;
			}
			else
			{
				color[0] = f2glf(light.r);
				color[1] = f2glf(light.g);
				color[2] = f2glf(light.b);
			}
			texcoord[0] = f2glf(uvl.u);
			texcoord[1] = f2glf(uvl.v);
		}
	}

	glVertexPointer(3, GL_FLOAT, 0, vertices.flat.data());
	glColorPointer(4, GL_FLOAT, 0, color_array.flat.data());
	if (tmap_drawer_ptr == draw_tmap) {
		glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array.flat.data());
	}
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, nv);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*
 * Everything texturemapped with secondary texture (walls with secondary texture)
 */
void _g3_draw_tmap_2(grs_canvas &canvas, const std::span<const g3s_point *const> pointlist, const std::span<const g3s_uvl, 4> uvl_list, const std::span<const g3s_lrgb, 4> light_rgb, grs_bitmap &bmbot, grs_bitmap &bm, const texture2_rotation_low orient, const tmap_drawer_type tmap_drawer_ptr)
{
	_g3_draw_tmap(canvas, pointlist, uvl_list.data(), light_rgb.data(), bmbot, tmap_drawer_ptr);//draw the bottom texture first.. could be optimized with multitexturing..
	ogl_client_states<int, GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY> cs;
	(void)cs;
	r_tpolyc++;
	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm, 1);
	ogl_texwrap(bm.gltexture, GL_REPEAT);

	flatten_array<GLfloat, 4, MAX_POINTS_PER_POLY> color_array;
	{
		const GLfloat alpha = (canvas.cv_fade_level >= GR_FADE_OFF)
			? 1.0
			: (1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0));
		auto &&color_range = unchecked_partial_range(color_array.nested, pointlist.size());
		if (bm.get_flag_mask(BM_FLAG_NO_LIGHTING))
		{
			for (auto &e : color_range)
			{
				e[0] = e[1] = e[2] = 1.0;
				e[3] = alpha;
			}
		}
		else
		{
			for (auto &&[e, l] : zip(
					color_range,
					unchecked_partial_range(light_rgb, pointlist.size())
				)
			)
			{
				e[0] = f2glf(l.r);
				e[1] = f2glf(l.g);
				e[2] = f2glf(l.b);
				e[3] = alpha;
			}
		}
	}

	flatten_array<GLfloat, 3, MAX_POINTS_PER_POLY> vertices;
	flatten_array<GLfloat, 2, MAX_POINTS_PER_POLY> texcoord_array;

	const auto nv = pointlist.size();
	for (auto &&[point, uvl, vert, texcoord] : zip(
			pointlist,
			unchecked_partial_range(uvl_list, nv),
			unchecked_partial_range(vertices.nested, nv),
			partial_range(texcoord_array.nested, nv)
		)
	)
	{
		const GLfloat uf = f2glf(uvl.u), vf = f2glf(uvl.v);
		switch(orient){
			case texture2_rotation_low::_1:
				texcoord[0] = 1.0 - vf;
				texcoord[1] = uf;
				break;
			case texture2_rotation_low::_2:
				texcoord[0] = 1.0 - uf;
				texcoord[1] = 1.0 - vf;
				break;
			case texture2_rotation_low::_3:
				texcoord[0] = vf;
				texcoord[1] = 1.0 - uf;
				break;
			default:
				texcoord[0] = uf;
				texcoord[1] = vf;
				break;
		}
		vert[0] = f2glf(point->p3_vec.x);
		vert[1] = f2glf(point->p3_vec.y);
		vert[2] = -f2glf(point->p3_vec.z);
	}
	glVertexPointer(3, GL_FLOAT, 0, vertices.flat.data());
	glColorPointer(4, GL_FLOAT, 0, color_array.flat.data());
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array.flat.data());
	glDrawArrays(GL_TRIANGLE_FAN, 0, nv);
}

/*
 * 2d Sprites (Fireaballs, powerups, explosions). NOT hostages
 */
void g3_draw_bitmap(grs_canvas &canvas, const vms_vector &pos, const fix iwidth, const fix iheight, grs_bitmap &bm)
{
	r_bitmapc++;
	
	ogl_client_states<int, GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY> cs;
	auto &i = std::get<0>(cs);

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm, 0);
	ogl_texwrap(bm.gltexture,GL_CLAMP_TO_EDGE);

	const auto width = fixmul(iwidth, Matrix_scale.x);
	const auto height = fixmul(iheight, Matrix_scale.y);
	constexpr unsigned point_count = 4;
	struct fvertex_t
	{
		GLfloat x, y, z;
	};
	struct fcolor_t
	{
		GLfloat r, g, b, a;
	};
	struct ftexcoord_t
	{
		GLfloat u, v;
	};
	std::array<fvertex_t, point_count> vertices;
	std::array<fcolor_t, point_count> color_array;
	std::array<ftexcoord_t, point_count> texcoord_array;
	const auto &&rpv{vm_vec_rotate(vm_vec_sub(pos, View_position), View_matrix)};
	const auto bmglu = bm.gltexture->u;
	const auto bmglv = bm.gltexture->v;
	const auto alpha = canvas.cv_fade_level >= GR_FADE_OFF ? 1.0 : (1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0));
	const auto vert_z = -f2glf(rpv.z);
	for (i=0;i<4;i++){
		auto pv = rpv;
		switch (i){
			case 0:
				texcoord_array[i].u = 0.0;
				texcoord_array[i].v = 0.0;
				pv.x+=-width;
				pv.y+=height;
				break;
			case 1:
				texcoord_array[i].u = bmglu;
				texcoord_array[i].v = 0.0;
				pv.x+=width;
				pv.y+=height;
				break;
			case 2:
				texcoord_array[i].u = bmglu;
				texcoord_array[i].v = bmglv;
				pv.x+=width;
				pv.y+=-height;
				break;
			case 3:
				texcoord_array[i].u = 0.0;
				texcoord_array[i].v = bmglv;
				pv.x+=-width;
				pv.y+=-height;
				break;
		}

		color_array[i].r = 1.0;
		color_array[i].g = 1.0;
		color_array[i].b = 1.0;
		color_array[i].a = alpha;
		vertices[i].x = f2glf(pv.x);
		vertices[i].y = f2glf(pv.y);
		vertices[i].z = vert_z;
	}
	glVertexPointer(3, GL_FLOAT, 0, vertices.data());
	glColorPointer(4, GL_FLOAT, 0, color_array.data());
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array.data());
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Replaced GL_QUADS
}

/*
 * Movies
 * Since this function will create a new texture each call, mipmapping can be very GPU intensive - so it has an optional setting for texture filtering.
 */
bool ogl_ubitblt_i(unsigned dw,unsigned dh,unsigned dx,unsigned dy, unsigned sw, unsigned sh, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest, const opengl_texture_filter texfilt)
{
	GLfloat xo,yo,xs,ys,u1,v1;
	struct bitblt_free_ogl_texture
	{
		ogl_texture t;
		~bitblt_free_ogl_texture()
		{
			ogl_freetexture(t);
		}
	};
	ogl_client_states<bitblt_free_ogl_texture, GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY> cs;
	ogl_texture &tex = std::get<0>(cs).t;
	r_ubitbltc++;

	ogl_init_texture(tex, sw, sh, OGL_FLAG_ALPHA);
	tex.prio = 0.0;
	tex.lw=src.bm_rowsize;

	u1=v1=0;
	
	dx+=dest.bm_x;
	dy+=dest.bm_y;
	xo=dx/static_cast<float>(last_width);
	xs=dw/static_cast<float>(last_width);
	yo=1.0-dy/static_cast<float>(last_height);
	ys=dh/static_cast<float>(last_height);
	
	OGL_ENABLE(TEXTURE_2D);
	
	ogl_loadtexture(gr_current_pal, src.get_bitmap_data(), sx, sy, tex, src.get_flags(), 0, texfilt, 0, 0);
	OGL_BINDTEXTURE(tex.handle);
	
	ogl_texwrap(&tex,GL_CLAMP_TO_EDGE);

	const GLfloat vertices[] = {
		xo,
		yo,
		xo + xs,
		yo,
		xo + xs,
		yo - ys,
		xo,
		yo - ys
	};

	const GLfloat texcoord_array[] = {
		u1,
		v1,
		tex.u,
		v1,
		tex.u,
		tex.v,
		u1,
		tex.v
	};

	glVertexPointer(2, GL_FLOAT, 0, vertices);
	static constexpr GLfloat color_array[] = {
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0
	};
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	return 0;
}

bool ogl_ubitblt(unsigned w,unsigned h,unsigned dx,unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest){
	return ogl_ubitblt_i(w, h, dx, dy, w, h, sx, sy, src, dest, opengl_texture_filter::classic);
}

/*
 * set depth testing on or off
 */
void ogl_toggle_depth_test(int enable)
{
	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

/* 
 * set blending function
 */
void ogl_set_blending(const gr_blend cv_blend_func)
{
	GLenum s, d;
	switch (cv_blend_func)
	{
		case gr_blend::additive_a:
			s = GL_SRC_ALPHA;
			d = GL_ONE;
			break;
		case gr_blend::additive_c:
			s = GL_ONE;
			d = GL_ONE;
			break;
		case gr_blend::normal:
		default:
			s = GL_SRC_ALPHA;
			d = GL_ONE_MINUS_SRC_ALPHA;
			break;
	}
	glBlendFunc(s, d);
}

void ogl_start_frame(grs_canvas &canvas)
{
	r_polyc=0;r_tpolyc=0;r_bitmapc=0;r_ubitbltc=0;

	OGL_VIEWPORT(canvas.cv_bitmap.bm_x, canvas.cv_bitmap.bm_y, canvas.cv_bitmap.bm_w, canvas.cv_bitmap.bm_h);
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
#if DXX_USE_OGLES
	perspective(90.0,1.0,0.1,5000.0);   
#else
	gluPerspective(90.0,1.0,0.1,5000.0);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
}

#if DXX_USE_STEREOSCOPIC_RENDER
void ogl_stereo_frame(const bool left_eye, const int xoff)
{
	const float dxoff = xoff * 2.0f / grd_curscreen->sc_canvas.cv_bitmap.bm_w;
	float stereo_transform_dxoff;

	// query if stereo quad buffering available?
	GLboolean ogl_stereo_enabled = false;
	glGetBooleanv(GL_STEREO, &ogl_stereo_enabled);

	int gl_buffer;
	if (left_eye) {
		// left eye view
		if (!ogl_stereo_enabled)
		{
			[]() {
				std::array<GLint, 4> ogl_stereo_viewport;
				glGetIntegerv(GL_VIEWPORT, ogl_stereo_viewport.data());
				// center unsqueezed side-by-side format
				switch (VR_stereo) {
					case StereoFormat::None:
						/* Not reached */
					case StereoFormat::AboveBelow:
					case StereoFormat::SideBySideFullHeight:
						/* No modification needed */
						return;
					case StereoFormat::SideBySideHalfHeight:
						ogl_stereo_viewport[1] -= ogl_stereo_viewport[3] / 2;		// y = h/4
						break;
					case StereoFormat::AboveBelowSync:
						{
							const int dy = VR_sync_width / 2;
							ogl_stereo_viewport[3] -= dy;
							ogl_stereo_viewport[1] += dy;
							break;
						}
				}
				glViewport(ogl_stereo_viewport[0], ogl_stereo_viewport[1], ogl_stereo_viewport[2], ogl_stereo_viewport[3]);
			}();
		}
		// rightward image shift adjustment for left eye offset
		stereo_transform_dxoff = -dxoff;		// xoff < 0
		gl_buffer = GL_BACK_LEFT;
	}
	else
	{
		// right eye view
		if (!ogl_stereo_enabled)
		{
			std::array<GLint, 4> ogl_stereo_viewport;
			glGetIntegerv(GL_VIEWPORT, ogl_stereo_viewport.data());
			switch (VR_stereo) {
				case StereoFormat::None:
					/* Not reached */
					break;
					// center unsqueezed side-by-side format
				case StereoFormat::SideBySideHalfHeight:
					ogl_stereo_viewport[1] -= ogl_stereo_viewport[3] / 2;		// y = h/4
					[[fallthrough]];
					// half-width viewports for side-by-side format
				case StereoFormat::SideBySideFullHeight:
					ogl_stereo_viewport[0] += ogl_stereo_viewport[2];		// x = w/2
					break;
					// half-height viewports for above/below format
				case StereoFormat::AboveBelowSync:
				case StereoFormat::AboveBelow:
					ogl_stereo_viewport[1] -= ogl_stereo_viewport[3];		// y = h/2
					if (VR_stereo == StereoFormat::AboveBelowSync)
						ogl_stereo_viewport[3] -= VR_sync_width / 2;
					break;
			}
			glViewport(ogl_stereo_viewport[0], ogl_stereo_viewport[1], ogl_stereo_viewport[2], ogl_stereo_viewport[3]);
		}
		// leftward image shift adjustment for right eye offset
		stereo_transform_dxoff = dxoff;		// xoff < 0
		gl_buffer = GL_BACK_RIGHT;
	}
	if (ogl_stereo_enabled)
		glDrawBuffer(gl_buffer);
	glMatrixMode(GL_PROJECTION);
	std::array<GLfloat, 16> ogl_stereo_transform;
	glGetFloatv(GL_PROJECTION_MATRIX, ogl_stereo_transform.data());
	ogl_stereo_transform[8] += stereo_transform_dxoff;
	glLoadMatrixf(ogl_stereo_transform.data());
	glMatrixMode(GL_MODELVIEW);
}
#endif

void ogl_end_frame(void){
	OGL_VIEWPORT(0, 0, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();//clear matrix
#if DXX_USE_OGLES
	glOrthof(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#else
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void gr_flip(void)
{
	if (CGameArg.DbgRenderStats)
	{
		gr_set_default_canvas();
		ogl_texture_stats(*grd_curcanv);
	}

	ogl_do_palfx();
	ogl_swap_buffers_internal();
	glClear(GL_COLOR_BUFFER_BIT);
}

// Allocate the pixel buffers 'pixels' and 'texbuf' based on current screen resolution
void ogl_init_pixel_buffers(unsigned w, unsigned h)
{
	w = std::bit_ceil(w);	// convert to OpenGL texture size
	h = std::bit_ceil(h);

	texbuf = std::make_unique<GLubyte[]>(max(w, 1024u)*max(h, 256u)*4);	// must also fit big font texture
}

void ogl_close_pixel_buffers(void)
{
	texbuf.reset();
}

static void ogl_filltexbuf(const palette_array_t &pal, const uint8_t *const data, GLubyte *texp, const unsigned truewidth, const unsigned width, const unsigned height, const int dxo, const int dyo, const unsigned twidth, const unsigned theight, const int type, const int bm_flags, const int data_format)
{
	if ((width > max(static_cast<unsigned>(grd_curscreen->get_screen_width()), 1024u)) ||
		(height > max(static_cast<unsigned>(grd_curscreen->get_screen_height()), 256u)))
		Error("Texture is too big: %ix%i", width, height);

	for (unsigned y=0;y<theight;y++)
	{
		int i=dxo+truewidth*(y+dyo);
		for (unsigned x=0;x<twidth;x++)
		{
			int c;
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
			else if (x == width && y < height) // end of bitmap reached - fill this pixel with last color to make a clean border when filtering this texture
			{
				c = data[(width*(y+1))-1];
			}
			else if (y == height && x < width) // end of bitmap reached - fill this row with color or last row to make a clean border when filtering this texture
			{
				c = data[(width*(height-1))+x];
			}
			else
			{
				c = 256; // fill the pad space with transparency (or blackness)
			}

			if (c == 254 && (bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			{
				switch (type)
				{
					case GL_RGBA:
						(*(texp++)) = 255;
						(*(texp++)) = 255;
						[[fallthrough]];
					case GL_LUMINANCE_ALPHA:
						(*(texp++)) = 255;
						(*(texp++)) = 0; // transparent pixel
						break;
#if !DXX_USE_OGLES
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
#endif
					default:
						Error("ogl_filltexbuf unhandled super-transparent texformat\n");
						break;
				}
			}
			else if ((c == 255 && (bm_flags & BM_FLAG_TRANSPARENT)) || c == 256)
			{
				switch (type)
				{
					case GL_RGBA:
						(*(texp++))=0;
						[[fallthrough]];
					case GL_RGB:
						(*(texp++))=0;
						[[fallthrough]];
					case GL_LUMINANCE_ALPHA:
						(*(texp++))=0;
						[[fallthrough]];
					case GL_LUMINANCE:
						(*(texp++))=0;//transparent pixel
						break;
#if !DXX_USE_OGLES
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
#endif
					default:
						Error("ogl_filltexbuf unknown texformat\n");
						break;
				}
			}
			else
			{
				switch (type)
				{
					case GL_LUMINANCE_ALPHA:
						(*(texp++))=255;
						[[fallthrough]];
					case GL_LUMINANCE://these could prolly be done to make the intensity based upon the intensity of the resulting color, but its not needed for anything (yet?) so no point. :)
						(*(texp++))=255;
						break;
					case GL_RGB:
						(*(texp++)) = pal[c].r * 4;
						(*(texp++)) = pal[c].g * 4;
						(*(texp++)) = pal[c].b * 4;
						break;
					case GL_RGBA:
						(*(texp++)) = pal[c].r * 4;
						(*(texp++)) = pal[c].g * 4;
						(*(texp++)) = pal[c].b * 4;
						(*(texp++)) = 255;//not transparent
						break;
#if !DXX_USE_OGLES
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
#endif
					default:
						Error("ogl_filltexbuf unknown texformat\n");
						break;
				}
			}
		}
	}
}

static void tex_set_size1(ogl_texture &tex, const unsigned dbits, const unsigned bits, const unsigned w, const unsigned h)
{
	int u;
	if (tex.tw!=w || tex.th!=h){
		u=(tex.w/static_cast<float>(tex.tw)*w) * (tex.h/static_cast<float>(tex.th)*h);
		glmprintf((CON_DEBUG, "shrunken texture?"));
	}else
		u=tex.w*tex.h;
	if (bits<=0){//the beta nvidia GLX server. doesn't ever return any bit sizes, so just use some assumptions.
		tex.bytes=(static_cast<float>(w)*h*dbits)/8.0;
		tex.bytesu=(static_cast<float>(u)*dbits)/8.0;
	}else{
		tex.bytes=(static_cast<float>(w)*h*bits)/8.0;
		tex.bytesu=(static_cast<float>(u)*bits)/8.0;
	}
	glmprintf((CON_DEBUG, "tex_set_size1: %ix%i, %ib(%i) %iB", w, h, bits, dbits, tex.bytes));
}

static void tex_set_size(ogl_texture &tex)
{
	GLint w,h;
	int bi=16,a=0;
#if !DXX_USE_OGLES
	if (CGameArg.DbgGlGetTexLevelParamOk)
	{
		GLint t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_LUMINANCE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTENSITY_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_RED_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_GREEN_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_BLUE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_ALPHA_SIZE,&t);a+=t;
	}
	else
#endif
	{
		w=tex.tw;
		h=tex.th;
	}
	switch (tex.format){
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
#if !DXX_USE_OGLES
		case GL_COLOR_INDEX:
			bi = 8;
			break;
#endif
		default:
			throw std::runtime_error("unknown texture format");
	}
	tex_set_size1(tex,bi,a,w,h);
}

//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
static int ogl_loadtexture(const palette_array_t &pal, const uint8_t *data, const int dxo, int dyo, ogl_texture &tex, const int bm_flags, const int data_format, opengl_texture_filter texfilt, const bool texanis, const bool edgepad)
{
	tex.tw = {std::bit_ceil(tex.w)};
	tex.th = {std::bit_ceil(tex.h)};	//calculate smallest texture size that can accommodate us (must be power of 2)

	//calculate u/v values that would make the resulting texture correctly sized
	tex.u = static_cast<float>(static_cast<double>(tex.w) / static_cast<double>(tex.tw));
	tex.v = static_cast<float>(static_cast<double>(tex.h) / static_cast<double>(tex.th));

	auto *bufP = texbuf.get();
	const uint8_t *outP = texbuf.get();
	{
		if (bm_flags >= 0)
			ogl_filltexbuf (pal, data, texbuf.get(), tex.lw, tex.w, tex.h, dxo, dyo, tex.tw, tex.th,
								 tex.format, bm_flags, data_format);
		else {
			if (!dxo && !dyo && (tex.w == tex.tw) && (tex.h == tex.th))
				outP = data;
			else {
				int h, w, tw;

				h = tex.lw / tex.w;
				w = (tex.w - dxo) * h;
				data += tex.lw * dyo + h * dxo;
				
				tw = tex.tw * h;
				h = tw - w;
				for (; dyo < tex.h; dyo++, data += tex.lw) {
					memcpy (bufP, data, w);
					bufP += w;
					memset (bufP, 0, h);
					bufP += h;
				}
				memset (bufP, 0, tex.th * tw - (bufP - texbuf.get()));
			}
		}
	}

	//bleed color (rgb) into the alpha area, to deal with "dark edges problem"
	if (tex.format == GL_RGBA && texfilt != opengl_texture_filter::classic && edgepad && !CGameArg.OglDarkEdges)
	{
		GLubyte *p = bufP;
		GLubyte *pdone = p + (4 * tex.tw * tex.th);
		int line = 4 * tex.tw;
		p += 4;
		pdone -= 4;
		GLubyte *ptop = p + line;
		GLubyte *pbottom = pdone - line;

		for (; p < pdone; p += 4)
		{
			//offsets 0 to 2 are r, g, b. offset 3 is alpha. 0x00 is transparent, 0xff is opaque.
			if (! *(p + 3))
			{
				if (*(p - 1))
				{
					*p = *(p - 4);
					*(p + 1) = *(p - 3);
					*(p + 2) = *(p - 2);
					continue;
				} //from left
				if (*(p + 7))
				{
					*p = *(p + 4);
					*(p + 1) = *(p + 5);
					*(p + 2) = *(p + 6);
					continue;
				} //from right
				if (p >= ptop)
				{
					if (*(p - line + 3))
					{
						*p = *(p - line);
						*(p + 1) = *(p - line + 1);
						*(p + 2) = *(p - line + 2);
						continue;
					} //from above
				}
				if (p < pbottom)
				{
					if (*(p + line + 3))
					{
						*p = *(p + line);
						*(p + 1) = *(p + line + 1);
						*(p + 2) = *(p + line + 2);
						continue;
					} //from below
					if (*(p + line - 1))
					{
						*p = *(p + line - 4);
						*(p + 1) = *(p + line - 3);
						*(p + 2) = *(p + line - 2);
						continue;
					} //bottom left
					if (*(p + line + 7))
					{
						*p = *(p + line + 4);
						*(p + 1) = *(p + line + 5);
						*(p + 2) = *(p + line + 6);
						continue;
					} //bottom right
				}
				if (p >= ptop)
				{
					if (*(p - line - 1))
					{
						*p = *(p - line - 4);
						*(p + 1) = *(p - line - 3);
						*(p + 2) = *(p - line - 2);
						continue;
					} //top left
					if (*(p - line + 7))
					{
						*p = *(p - line + 4);
						*(p + 1) = *(p - line + 5);
						*(p + 2) = *(p - line + 6);
						continue;
					} //top right
				}
			}
		}
	}

	int rescale = 1;
	if (texfilt == opengl_texture_filter::upscale)
	{
		rescale = 4;
		if ((tex.tw > 256) || (tex.th > 256))
		{
			//don't scale big textures, instead "cheat" and render them as normal (nearest)
			//these are normally 320x200 and 640x480 images.
			//this deals with texture size limits, as well as large textures causing noticeable performance issues/hiccups.
			texfilt = opengl_texture_filter::classic;
			rescale = 1;
		}
	}
	std::unique_ptr<GLubyte[]> buftemp;
	if (rescale > 1)
	{
		int rebpp = 3;
		if (tex.format == GL_RGBA) rebpp = 4;
		if (tex.format == GL_LUMINANCE) rebpp = 1;
		buftemp = std::make_unique<GLubyte[]>(rescale * tex.tw * rescale * tex.th * rebpp);
		int x,y;
		GLubyte *p = buftemp.get();
		int len = tex.tw*rebpp*rescale;
		int bppscale = (rebpp*rescale);

		for (y = 0; y < tex.th; y++)
		{
			GLubyte	*p1 = bufP;

			for (x = 0; x < len; x++)
			{
				*(p+x) = *(outP + ((x / bppscale)*rebpp + (x % rebpp)));
			}
			p1 = p;
			p += len;
			int y2;
			for (y2 = 1; y2 < rescale; y2++)
			{
				memcpy (p, p1, len);
				p += len;
			}
			outP += tex.tw*rebpp;
		}
		outP = buftemp.get();
	}

	// Generate OpenGL texture IDs.
	glGenTextures (1, &tex.handle);
#if !DXX_USE_OGLES
	//set priority
	glPrioritizeTextures (1, &tex.handle, &tex.prio);
#endif
	// Give our data to OpenGL.
	OGL_BINDTEXTURE(tex.handle);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// should match structue in menu.cpp
	// organized in switches for better readability
	bool buildmipmap = false;
	GLint gl_mag_filter_int, gl_min_filter_int;
	switch (texfilt)
	{
		default:
		case opengl_texture_filter::classic: // Classic - Nearest
			gl_mag_filter_int = GL_NEAREST;
			if (texanis && ogl_maxanisotropy > 1.0f)
			{
				// looks nicer if anisotropy is applied.
				gl_min_filter_int = GL_NEAREST_MIPMAP_LINEAR;
				buildmipmap = true;
			}
			else
			{
				gl_min_filter_int = GL_NEAREST;
				buildmipmap = false;
			}
			break;
		case opengl_texture_filter::upscale: // Upscaled - i.e. Blocky Filtered (Bilinear)
		case opengl_texture_filter::trilinear: // Smooth - Trilinear
			gl_mag_filter_int = GL_LINEAR;
			gl_min_filter_int = GL_LINEAR_MIPMAP_LINEAR;
			buildmipmap = true;
			break;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_mag_filter_int);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_min_filter_int);
	if (texanis && ogl_maxanisotropy > 1.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, ogl_maxanisotropy);

#if DXX_USE_OGLES // in OpenGL ES 1.1 the mipmaps are automatically generated by a parameter
	glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, buildmipmap ? GL_TRUE : GL_FALSE);
#else
	if (buildmipmap)
	{
		gluBuild2DMipmaps (
				GL_TEXTURE_2D, tex.internalformat, 
				tex.tw * rescale, tex.th * rescale, tex.format,
				GL_UNSIGNED_BYTE, 
				outP);
	}
	else
#endif
	{
		glTexImage2D (
			GL_TEXTURE_2D, 0, tex.internalformat,
			tex.tw * rescale, tex.th * rescale, 0, tex.format, // RGBA textures.
			GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
			outP);
	}

	tex_set_size(tex);
	r_texcount++;
	return 0;
}

void ogl_loadbmtexture_f(grs_bitmap &rbm, const opengl_texture_filter texfilt, bool texanis, bool edgepad)
{
	assert(!rbm.get_flag_mask(BM_FLAG_PAGED_OUT));
	assert(rbm.bm_data);
	grs_bitmap *bm = &rbm;
	while (const auto bm_parent = bm->bm_parent)
		bm = bm_parent;
	if (bm->gltexture && bm->gltexture->handle > 0)
		return;
	auto buf=bm->get_bitmap_data();
	const unsigned bm_w = bm->bm_w;
	if (bm->gltexture == NULL){
		ogl_init_texture(*(bm->gltexture = ogl_get_free_texture()), bm_w, bm->bm_h, ((bm->get_flag_mask(BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) ? OGL_FLAG_ALPHA : 0));
	}
	else {
		if (bm->gltexture->handle>0)
			return;
		if (bm->gltexture->w==0){
			bm->gltexture->lw = bm_w;
			bm->gltexture->w = bm_w;
			bm->gltexture->h=bm->bm_h;
		}
	}

	std::array<uint8_t, 300*1024> decodebuf;
	if (bm->get_flag_mask(BM_FLAG_RLE))
	{
		class bm_rle_expand_state
		{
			uint8_t *dbits;
			uint8_t *const ebits;
		public:
			bm_rle_expand_state(uint8_t *const b, uint8_t *const e) :
				dbits(b), ebits(e)
			{
			}
			uint8_t *get_begin_dbits() const
			{
				return dbits;
			}
			uint8_t *get_end_dbits() const
			{
				return ebits;
			}
			void consume_dbits(const unsigned w)
			{
				dbits += w;
			}
		};
		decodebuf = {};
		buf = decodebuf.data();
		if (!bm_rle_expand(*bm).loop(bm_w, bm_rle_expand_state(begin(decodebuf), end(decodebuf))))
		{
			con_printf(CON_URGENT, "error: insufficient space to decode %ux%hu bitmap.  Please report this as a bug.", bm_w, bm->bm_h);
		}
	}
	ogl_loadtexture(gr_palette, buf, 0, 0, *bm->gltexture, bm->get_flags(), 0, texfilt, texanis, edgepad);
}

static void ogl_freetexture(ogl_texture &gltexture)
{
	if (gltexture.handle>0) {
		r_texcount--;
		glmprintf((CON_DEBUG, "ogl_freetexture(%p):%i (%i left)", &gltexture, gltexture.handle, r_texcount));
		glDeleteTextures( 1, &gltexture.handle );
//		gltexture->handle=0;
		ogl_reset_texture(gltexture);
	}
}

void ogl_freebmtexture(grs_bitmap &bm)
{
	if (auto &gltexture = bm.gltexture)
		ogl_freetexture(*std::exchange(gltexture, nullptr));
}

const ogl_colors::array_type ogl_colors::white = {{
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 1.0, 1.0,
}};

const ogl_colors::array_type &ogl_colors::init_maybe_white(const int c)
{
	return c == -1 ? white : init_palette(c);
}

const ogl_colors::array_type &ogl_colors::init_palette(const unsigned c)
{
	const auto &rgb = gr_current_pal[c];
	const GLfloat r = rgb.r / 63.0, g = rgb.g / 63.0, b = rgb.b / 63.0;
	a = {{
		r, g, b, 1.0,
		r, g, b, 1.0,
		r, g, b, 1.0,
		r, g, b, 1.0,
	}};
	return a;
}

bool ogl_ubitmapm_cs(grs_canvas &canvas, int x, int y,int dw, int dh, grs_bitmap &bm, int c)
{
	ogl_colors color;
	return ogl_ubitmapm_cs(canvas, x, y, dw, dh, bm, color.init(c));
}

/*
 * Menu / gauges 
 */
bool ogl_ubitmapm_cs(grs_canvas &canvas, const int entry_x, const int entry_y, const int entry_dw, const int entry_dh, grs_bitmap &bm, const ogl_colors::array_type &color_array)
{
	GLfloat u1,u2,v1,v2;
	ogl_client_states<GLfloat, GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY> cs;
	auto &xo = std::get<0>(cs);
	const int adjusted_canvas_x = entry_x + canvas.cv_bitmap.bm_x;
	const int adjusted_canvas_y = entry_y + canvas.cv_bitmap.bm_y;

	const int effective_dw = (entry_dw == opengl_bitmap_use_dst_canvas)
		? canvas.cv_bitmap.bm_w
		: ((entry_dw == opengl_bitmap_use_src_bitmap)
			? bm.bm_w
			: entry_dw);
	const int effective_dh = (entry_dh == opengl_bitmap_use_dst_canvas)
		? canvas.cv_bitmap.bm_h
		: ((entry_dh == opengl_bitmap_use_src_bitmap)
			? bm.bm_h
			: entry_dh);

	xo = adjusted_canvas_x / (static_cast<double>(last_width));
	const GLfloat xf = (effective_dw + adjusted_canvas_x) / (static_cast<double>(last_width));
	const GLfloat yo = 1.0 - adjusted_canvas_y / (static_cast<double>(last_height));
	const GLfloat yf = 1.0 - (effective_dh + adjusted_canvas_y) / (static_cast<double>(last_height));

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm, 0);
	ogl_texwrap(bm.gltexture,GL_CLAMP_TO_EDGE);
	
	if (bm.bm_x==0){
		u1=0;
		if (bm.bm_w==bm.gltexture->w)
			u2=bm.gltexture->u;
		else
			u2=(bm.bm_w+bm.bm_x)/static_cast<float>(bm.gltexture->tw);
	}else {
		u1=bm.bm_x/static_cast<float>(bm.gltexture->tw);
		u2=(bm.bm_w+bm.bm_x)/static_cast<float>(bm.gltexture->tw);
	}
	if (bm.bm_y==0){
		v1=0;
		if (bm.bm_h==bm.gltexture->h)
			v2=bm.gltexture->v;
		else
			v2=(bm.bm_h+bm.bm_y)/static_cast<float>(bm.gltexture->th);
	}else{
		v1=bm.bm_y/static_cast<float>(bm.gltexture->th);
		v2=(bm.bm_h+bm.bm_y)/static_cast<float>(bm.gltexture->th);
	}

	const std::array<GLfloat, 8> vertices{{
		xo, yo,
		xf, yo,
		xf, yf,
		xo, yf,
	}};
	const std::array<GLfloat, 8> texcoord_array{{
		u1, v1,
		u2, v1,
		u2, v2,
		u1, v2,
	}};
	glVertexPointer(2, GL_FLOAT, 0, vertices.data());
	glColorPointer(4, GL_FLOAT, 0, color_array.data());
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array.data());
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	return 0;
}

}
