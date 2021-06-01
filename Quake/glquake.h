/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers
Copyright (C) 2016 Axel Gneiting

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __GLQUAKE_H
#define __GLQUAKE_H

void GL_WaitForDeviceIdle (void);
qboolean GL_BeginRendering (int *x, int *y, int *width, int *height);
qboolean GL_AcquireNextSwapChainImage (void);
void GL_EndRendering (qboolean swapchain_acquired);
qboolean GL_Set2D (void);

extern	int glx, gly, glwidth, glheight;

// r_local.h -- private refresh defs

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
#define	MAX_LBM_HEIGHT		480

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01

#define	MAX_GLTEXTURES	2048
#define NUM_COLOR_BUFFERS 2
#define INITIAL_STAGING_BUFFER_SIZE_KB	16384

#define FAN_INDEX_BUFFER_SIZE 126

void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
texture_t *R_TextureAnimation (texture_t *base, int frame);

typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int			lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int			dlight;
	int			size;		// including header
	unsigned		width;
	unsigned		height;		// DEBUG only needed for debug
	float			mipscale;
	struct texture_s	*texture;	// checked for animating textures
	byte			data[4];	// width*height elements
} surfcache_t;


typedef struct
{
	pixel_t		*surfdat;	// destination for generated surface
	int		rowbytes;	// destination logical width in bytes
	msurface_t	*surf;		// description for surface to generate
	fixed8_t	lightadj[MAXLIGHTMAPS];
							// adjust for lightmap levels for dynamic lighting
	texture_t	*texture;	// corrected for animating textures
	int		surfmip;	// mipmapped ratio of surface texels / world pixels
	int		surfwidth;	// in mipmapped texels
	int		surfheight;	// in mipmapped texels
} drawsurf_t;


typedef enum {
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} particle_t;

typedef struct vulkan_pipeline_layout_s {
	VkPipelineLayout		handle;
	VkPushConstantRange		push_constant_range;
} vulkan_pipeline_layout_t;

typedef struct vulkan_pipeline_s {
	VkPipeline					handle;
	vulkan_pipeline_layout_t	layout;
} vulkan_pipeline_t;

typedef struct vulkan_desc_set_layout_s {
	VkDescriptorSetLayout		handle;
	int							num_combined_image_samplers;
	int							num_ubos_dynamic;
	int							num_input_attachments;
	int							num_storage_images;
} vulkan_desc_set_layout_t;

#define WORLD_PIPELINE_COUNT 8

typedef struct
{
	VkDevice							device;
	qboolean							device_idle;
	qboolean							validation;
	VkQueue								queue;
	VkCommandBuffer						command_buffer;
	vulkan_pipeline_t					current_pipeline;
	VkClearValue						color_clear_value;
	VkFormat							swap_chain_format;
	qboolean							want_full_screen_exclusive;
	qboolean							swap_chain_full_screen_exclusive;
	qboolean							swap_chain_full_screen_acquired;
	VkPhysicalDeviceProperties			device_properties;
	VkPhysicalDeviceMemoryProperties	memory_properties;
	uint32_t							gfx_queue_family_index;
	VkFormat							color_format;
	VkFormat							depth_format;
	VkSampleCountFlagBits				sample_count;
	qboolean							supersampling;
	qboolean							non_solid_fill;

	// Instance extensions
	qboolean							get_surface_capabilities_2;
	qboolean							get_physical_device_properties_2;

	// Device extensions
	qboolean							dedicated_allocation;
	qboolean							full_screen_exclusive;

	// Buffers
	VkImage								color_buffers[NUM_COLOR_BUFFERS];

	// Index buffers
	VkBuffer							fan_index_buffer;

	// Staging buffers
	int									staging_buffer_size;

	// Render passes
	VkRenderPass						main_render_pass;
	VkClearValue						main_clear_values[4];
	VkRenderPassBeginInfo				main_render_pass_begin_infos[2];
	VkRenderPass						ui_render_pass;
	VkRenderPassBeginInfo				ui_render_pass_begin_info;
	VkRenderPass						warp_render_pass;

	// Pipelines
	vulkan_pipeline_t					basic_alphatest_pipeline[2];
	vulkan_pipeline_t					basic_blend_pipeline[2];
	vulkan_pipeline_t					basic_notex_blend_pipeline[2];
	vulkan_pipeline_t					basic_poly_blend_pipeline;
	vulkan_pipeline_layout_t			basic_pipeline_layout;
	vulkan_pipeline_t					world_pipelines[WORLD_PIPELINE_COUNT];
	vulkan_pipeline_layout_t			world_pipeline_layout;
	vulkan_pipeline_t					water_pipeline;
	vulkan_pipeline_t					water_blend_pipeline;
	vulkan_pipeline_t					raster_tex_warp_pipeline;
	vulkan_pipeline_t					particle_pipeline;
	vulkan_pipeline_t					sprite_pipeline;
	vulkan_pipeline_t					sky_color_pipeline;
	vulkan_pipeline_t					sky_box_pipeline;
	vulkan_pipeline_t					sky_layer_pipeline;
	vulkan_pipeline_t					alias_pipeline;
	vulkan_pipeline_t					alias_blend_pipeline;
	vulkan_pipeline_t					alias_alphatest_pipeline;
	vulkan_pipeline_t					postprocess_pipeline;
	vulkan_pipeline_t					screen_warp_pipeline;
	vulkan_pipeline_t					cs_tex_warp_pipeline;
	vulkan_pipeline_t					showtris_pipeline;
	vulkan_pipeline_t					showtris_depth_test_pipeline;

	// Descriptors
	VkDescriptorPool					descriptor_pool;
	vulkan_desc_set_layout_t			ubo_set_layout;
	vulkan_desc_set_layout_t			single_texture_set_layout;
	vulkan_desc_set_layout_t			input_attachment_set_layout;
	VkDescriptorSet						screen_warp_desc_set;
	vulkan_desc_set_layout_t			screen_warp_set_layout;
	vulkan_desc_set_layout_t			single_texture_cs_write_set_layout;

	// Samplers
	VkSampler							point_sampler;
	VkSampler							linear_sampler;
	VkSampler							point_aniso_sampler;
	VkSampler							linear_aniso_sampler;

	// Matrices
	float								projection_matrix[16];
	float								view_matrix[16];
	float								view_projection_matrix[16];

	//Dispatch table
	PFN_vkCmdBindPipeline				vk_cmd_bind_pipeline;
	PFN_vkCmdPushConstants				vk_cmd_push_constants;
	PFN_vkCmdBindDescriptorSets			vk_cmd_bind_descriptor_sets;
	PFN_vkCmdBindIndexBuffer			vk_cmd_bind_index_buffer;
	PFN_vkCmdBindVertexBuffers			vk_cmd_bind_vertex_buffers;
	PFN_vkCmdDraw						vk_cmd_draw;
	PFN_vkCmdDrawIndexed				vk_cmd_draw_indexed;
	PFN_vkCmdPipelineBarrier			vk_cmd_pipeline_barrier;
	PFN_vkCmdCopyBufferToImage			vk_cmd_copy_buffer_to_image;
} vulkanglobals_t;

extern vulkanglobals_t vulkan_globals;

//====================================================

extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t	*currententity;
extern	int		r_visframecount;	// ??? what difs?
extern	int		r_framecount;
extern	mplane_t	frustum[4];
extern	int render_pass_index;
extern	qboolean render_warp;
extern	qboolean in_update_screen;
extern	qboolean use_simd;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_pos;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_lavaalpha;
extern	cvar_t	r_telealpha;
extern	cvar_t	r_slimealpha;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;

extern	cvar_t	gl_polyblend;
extern	cvar_t	gl_nocolors;

extern	cvar_t	gl_subdivide_size;

//johnfitz -- polygon offset
#define OFFSET_BMODEL 1
#define OFFSET_NONE 0
#define OFFSET_DECAL -1
#define OFFSET_FOG -2
#define OFFSET_SHOWTRIS -3

//johnfitz -- rendering statistics
extern int rs_brushpolys, rs_aliaspolys, rs_skypolys, rs_particles, rs_fogpolys;
extern int rs_dynamiclightmaps, rs_brushpasses, rs_aliaspasses, rs_skypasses;
extern float rs_megatexels;

//johnfitz -- track developer statistics that vary every frame
extern cvar_t devstats;
typedef struct {
	int		packetsize;
	int		edicts;
	int		visedicts;
	int		efrags;
	int		tempents;
	int		beams;
	int		dlights;
} devstats_t;
extern devstats_t dev_stats, dev_peakstats;

//ohnfitz -- reduce overflow warning spam
typedef struct {
	double	packetsize;
	double	efrags;
	double	beams;
	double	varstring;
} overflowtimes_t;
extern overflowtimes_t dev_overflows; //this stores the last time overflow messages were displayed, not the last time overflows occured
#define CONSOLE_RESPAM_TIME 3 // seconds between repeated warning messages

typedef struct
{
	float	position[3];
	float	texcoord[2];
	byte	color[4];
} basicvertex_t;

//johnfitz -- moved here from r_brush.c
extern int gl_lightmap_format, lightmap_bytes;

#define LMBLOCK_WIDTH	256	//FIXME: make dynamic. if we have a decent card there's no real reason not to use 4k or 16k (assuming there's no lightstyles/dynamics that need uploading...)
#define LMBLOCK_HEIGHT	256 //Alternatively, use texture arrays, which would avoid the need to switch textures as often.

typedef struct glRect_s {
	unsigned short l,t,w,h;
} glRect_t;
struct lightmap_s
{
	gltexture_t *texture;
	qboolean	modified;
	glRect_t	rectchange;

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		*data;//[4*LMBLOCK_WIDTH*LMBLOCK_HEIGHT];
};
extern struct lightmap_s *lightmap;
extern int lightmap_count;	//allocated lightmaps

extern qboolean r_lightmap_cheatsafe, r_drawworld_cheatsafe; //johnfitz

extern float	map_wateralpha, map_lavaalpha, map_telealpha, map_slimealpha; //ericw

//johnfitz -- fog functions called from outside gl_fog.c
void Fog_ParseServerMessage (void);
float *Fog_GetColor (void);
float Fog_GetDensity (void);
void Fog_EnableGFog (void);
void Fog_DisableGFog (void);
void Fog_SetupFrame (void);
void Fog_NewMap (void);
void Fog_Init (void);

void R_NewGame (void);

void R_AnimateLight (void);
void R_MarkSurfaces (void);
qboolean R_CullBox (vec3_t emins, vec3_t emaxs);
void R_StoreEfrags (efrag_t **ppefrag);
qboolean R_CullModelForEntity (entity_t *e);
void R_RotateForEntity (float matrix[16], vec3_t origin, vec3_t angles);
void R_MarkLights (dlight_t *light, int num, mnode_t *node);

void R_InitParticles (void);
void R_DrawParticles (void);
void CL_RunParticles (void);
void R_ClearParticles (void);

void R_TranslatePlayerSkin (int playernum);
void R_TranslateNewPlayerSkin (int playernum); //johnfitz -- this handles cases when the actual texture changes
void R_UpdateWarpTextures (void);

void R_DrawWorld (void);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);

void R_DrawTextureChains_Water (qmodel_t *model, entity_t *ent, texchain_t chain);

void GL_BuildLightmaps (void);
void GL_DeleteBModelVertexBuffer (void);
void GL_BuildBModelVertexBuffer (void);
void GLMesh_LoadVertexBuffers (void);
void GLMesh_DeleteVertexBuffers (void);

int R_LightPoint (vec3_t p);

void GL_SubdivideSurface (msurface_t *fa);
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride);
void R_RenderDynamicLightmaps (msurface_t *fa);
void R_UploadLightmaps (void);

void R_DrawWorld_ShowTris(void);
void R_DrawBrushModel_ShowTris (entity_t *e);
void R_DrawAliasModel_ShowTris (entity_t *e);
void R_DrawParticles_ShowTris(void);

void DrawGLPoly (glpoly_t *p, float color[3], float alpha);
void GL_MakeAliasModelDisplayLists (qmodel_t *m, aliashdr_t *hdr);

void Sky_Init (void);
void Sky_DrawSky (void);
void Sky_NewMap (void);
void Sky_LoadTexture (texture_t *mt);
void Sky_LoadSkyBox (const char *name);

void R_ClearTextureChains (qmodel_t *mod, texchain_t chain);
void R_ChainSurface (msurface_t *surf, texchain_t chain);
void R_DrawTextureChains (qmodel_t *model, entity_t *ent, texchain_t chain);
void R_DrawWorld_Water (void);

float GL_WaterAlphaForSurface (msurface_t *fa);

int GL_MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, VkFlags preferred_mask);

void R_CreateDescriptorPool();
void R_CreateDescriptorSetLayouts();
void R_InitSamplers();
void R_CreatePipelineLayouts();
void R_CreatePipelines();
void R_DestroyPipelines();

static inline void R_BindPipeline(VkPipelineBindPoint bind_point, vulkan_pipeline_t pipeline)
{
	assert(pipeline.handle != VK_NULL_HANDLE);
	assert(pipeline.layout.handle != VK_NULL_HANDLE);
	if(vulkan_globals.current_pipeline.handle != pipeline.handle) {
		vulkan_globals.vk_cmd_bind_pipeline(vulkan_globals.command_buffer, bind_point, pipeline.handle);
		if ((vulkan_globals.current_pipeline.layout.push_constant_range.stageFlags != pipeline.layout.push_constant_range.stageFlags)
			|| (vulkan_globals.current_pipeline.layout.push_constant_range.size != pipeline.layout.push_constant_range.size))
		{
			void * zeroes = alloca(pipeline.layout.push_constant_range.size);
			memset(zeroes, 0, pipeline.layout.push_constant_range.size);
			vulkan_globals.vk_cmd_push_constants(vulkan_globals.command_buffer, pipeline.layout.handle, pipeline.layout.push_constant_range.stageFlags, 0, pipeline.layout.push_constant_range.size, zeroes);
		}
		vulkan_globals.current_pipeline = pipeline;
	}
}

static inline void R_PushConstants(VkShaderStageFlags stage_flags, int offset, int size, const void * data)
{
	vulkan_globals.vk_cmd_push_constants(vulkan_globals.command_buffer, vulkan_globals.current_pipeline.layout.handle, stage_flags, offset, size, data);
}

VkDescriptorSet R_AllocateDescriptorSet(vulkan_desc_set_layout_t * layout);
void R_FreeDescriptorSet(VkDescriptorSet desc_set, vulkan_desc_set_layout_t * layout);

void R_InitStagingBuffers();
void R_SubmitStagingBuffers();
byte * R_StagingAllocate(int size, int alignment, VkCommandBuffer * command_buffer, VkBuffer * buffer, int * buffer_offset);

void R_InitGPUBuffers();
void R_SwapDynamicBuffers();
void R_FlushDynamicBuffers();
void R_CollectDynamicBufferGarbage();
void R_CollectMeshBufferGarbage();
byte * R_VertexAllocate(int size, VkBuffer * buffer, VkDeviceSize * buffer_offset);
byte * R_IndexAllocate(int size, VkBuffer * buffer, VkDeviceSize * buffer_offset);
byte * R_UniformAllocate(int size, VkBuffer * buffer, uint32_t * buffer_offset, VkDescriptorSet * descriptor_set);

void GL_SetObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char * name);

#endif	/* __GLQUAKE_H */

