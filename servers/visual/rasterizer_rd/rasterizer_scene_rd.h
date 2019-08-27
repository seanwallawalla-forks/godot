#ifndef RASTERIZER_SCENE_RD_H
#define RASTERIZER_SCENE_RD_H

#include "core/rid_owner.h"
#include "servers/visual/rasterizer.h"
#include "servers/visual/rasterizer_rd/rasterizer_storage_rd.h"
#include "servers/visual/rendering_device.h"

class RasterizerSceneRD : public RasterizerScene {
protected:
	struct RenderBufferData {

		virtual void configure(RID p_render_target, int p_width, int p_height, VS::ViewportMSAA p_msaa) = 0;
		virtual ~RenderBufferData() {}
	};
	virtual RenderBufferData *_create_render_buffer_data() = 0;

	virtual void _render_scene(RenderBufferData *p_buffer_data, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count, RID *p_light_cull_result, int p_light_cull_count, RID *p_reflection_probe_cull_result, int p_reflection_probe_cull_count, RID p_environment, RID p_shadow_atlas, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass) = 0;

private:
	int roughness_layers;

	RasterizerStorageRD *storage;

	struct Sky {
		int radiance_size = 256;
		VS::SkyMode mode = VS::SKY_MODE_QUALITY;
		RID panorama;
		RID radiance;
		bool dirty = false;
		Sky *dirty_list = nullptr;
		struct Layer {
			struct Mipmap {
				RID framebuffers[6];
				RID views[6];
				Size2i size;
			};
			Vector<Mipmap> mipmaps;
		};
		RID radiance_base_cubemap; //cubemap for first layer, first cubemap
		Vector<Layer> layers;
	};

	Sky *dirty_sky_list = nullptr;

	void _sky_invalidate(Sky *p_sky);
	void _update_dirty_skys();

	uint32_t sky_ggx_samples_quality;
	uint32_t sky_ggx_samples_realtime;
	bool sky_use_cubemap_array;

	mutable RID_Owner<Sky> sky_owner;

	struct Environent {

		// BG
		VS::EnvironmentBG background = VS::ENV_BG_CLEAR_COLOR;
		RID sky;
		float sky_custom_fov = 0.0;
		Basis sky_orientation;
		Color bg_color;
		float bg_energy = 1.0;
		int canvas_max_layer = 0;
		VS::EnvironmentAmbientSource ambient_source = VS::ENV_AMBIENT_SOURCE_BG;
		Color ambient_light;
		float ambient_light_energy = 1.0;
		float ambient_sky_contribution = 1.0;
		VS::EnvironmentReflectionSource reflection_source = VS::ENV_REFLECTION_SOURCE_BG;

		/// Tonemap

		VS::EnvironmentToneMapper tone_mapper;
		float exposure = 1.0;
		float white = 1.0;
		bool auto_exposure = false;
		float min_luminance = 0.2;
		float max_luminance = 8.0;
		float auto_exp_speed = 0.2;
		float auto_exp_scale = 0.5;
	};

	mutable RID_Owner<Environent> environment_owner;

	struct RenderBuffers {

		RenderBufferData *data = nullptr;
		int width = 0, height = 0;
		VS::ViewportMSAA msaa = VS::VIEWPORT_MSAA_DISABLED;
		RID render_target;
	};

	mutable RID_Owner<RenderBuffers> render_buffers_owner;

public:
	/* SHADOW ATLAS API */

	RID shadow_atlas_create() { return RID(); }
	void shadow_atlas_set_size(RID p_atlas, int p_size) {}
	void shadow_atlas_set_quadrant_subdivision(RID p_atlas, int p_quadrant, int p_subdivision) {}
	bool shadow_atlas_update_light(RID p_atlas, RID p_light_intance, float p_coverage, uint64_t p_light_version) { return false; }

	int get_directional_light_shadow_size(RID p_light_intance) { return 0; }
	void set_directional_shadow_count(int p_count) {}

	/* SKY API */

	RID sky_create();
	void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	void sky_set_mode(RID p_sky, VS::SkyMode p_mode);
	void sky_set_texture(RID p_sky, RID p_panorama);

	RID sky_get_panorama_texture_rd(RID p_sky) const;
	RID sky_get_radiance_texture_rd(RID p_sky) const;

	/* ENVIRONMENT API */

	RID environment_create();

	void environment_set_background(RID p_env, VS::EnvironmentBG p_bg);
	void environment_set_sky(RID p_env, RID p_sky);
	void environment_set_sky_custom_fov(RID p_env, float p_scale);
	void environment_set_sky_orientation(RID p_env, const Basis &p_orientation);
	void environment_set_bg_color(RID p_env, const Color &p_color);
	void environment_set_bg_energy(RID p_env, float p_energy);
	void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	void environment_set_ambient_light(RID p_env, const Color &p_color, VS::EnvironmentAmbientSource p_ambient = VS::ENV_AMBIENT_SOURCE_BG, float p_energy = 1.0, float p_sky_contribution = 0.0, VS::EnvironmentReflectionSource p_reflection_source = VS::ENV_REFLECTION_SOURCE_BG);

	VS::EnvironmentBG environment_get_background(RID p_env) const;
	RID environment_get_sky(RID p_env) const;
	float environment_get_sky_custom_fov(RID p_env) const;
	Basis environment_get_sky_orientation(RID p_env) const;
	Color environment_get_bg_color(RID p_env) const;
	float environment_get_bg_energy(RID p_env) const;
	int environment_get_canvas_max_layer(RID p_env) const;
	Color environment_get_ambient_light_color(RID p_env) const;
	VS::EnvironmentAmbientSource environment_get_ambient_light_ambient_source(RID p_env) const;
	float environment_get_ambient_light_ambient_energy(RID p_env) const;
	float environment_get_ambient_sky_contribution(RID p_env) const;
	VS::EnvironmentReflectionSource environment_get_reflection_source(RID p_env) const;

	bool is_environment(RID p_env) const;

	void environment_set_dof_blur_near(RID p_env, bool p_enable, float p_distance, float p_transition, float p_far_amount, VS::EnvironmentDOFBlurQuality p_quality) {}
	void environment_set_dof_blur_far(RID p_env, bool p_enable, float p_distance, float p_transition, float p_far_amount, VS::EnvironmentDOFBlurQuality p_quality) {}
	void environment_set_glow(RID p_env, bool p_enable, int p_level_flags, float p_intensity, float p_strength, float p_bloom_threshold, VS::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold, float p_hdr_bleed_scale, float p_hdr_luminance_cap, bool p_bicubic_upscale) {}

	void environment_set_fog(RID p_env, bool p_enable, float p_begin, float p_end, RID p_gradient_texture) {}

	void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int, float p_fade_out, float p_depth_tolerance, bool p_roughness) {}
	void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity, float p_radius2, float p_intensity2, float p_bias, float p_light_affect, float p_ao_channel_affect, const Color &p_color, VS::EnvironmentSSAOQuality p_quality, VS::EnvironmentSSAOBlur p_blur, float p_bilateral_sharpness) {}

	void environment_set_tonemap(RID p_env, VS::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white, bool p_auto_exposure, float p_min_luminance, float p_max_luminance, float p_auto_exp_speed, float p_auto_exp_scale);
	VS::EnvironmentToneMapper environment_get_tonemapper(RID p_env) const;
	float environment_get_exposure(RID p_env) const;
	float environment_get_white(RID p_env) const;
	bool environment_get_auto_exposure(RID p_env) const;
	float environment_get_min_luminance(RID p_env) const;
	float environment_get_max_luminance(RID p_env) const;
	float environment_get_auto_exposure_scale(RID p_env) const;
	float environment_get_auto_exposure_speed(RID p_env) const;

	void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness, float p_contrast, float p_saturation, RID p_ramp) {}

	void environment_set_fog(RID p_env, bool p_enable, const Color &p_color, const Color &p_sun_color, float p_sun_amount) {}
	void environment_set_fog_depth(RID p_env, bool p_enable, float p_depth_begin, float p_depth_end, float p_depth_curve, bool p_transmit, float p_transmit_curve) {}
	void environment_set_fog_height(RID p_env, bool p_enable, float p_min_height, float p_max_height, float p_height_curve) {}

	RID light_instance_create(RID p_light) { return RID(); }
	void light_instance_set_transform(RID p_light_instance, const Transform &p_transform) {}
	void light_instance_set_shadow_transform(RID p_light_instance, const CameraMatrix &p_projection, const Transform &p_transform, float p_far, float p_split, int p_pass, float p_bias_scale = 1.0) {}
	void light_instance_mark_visible(RID p_light_instance) {}

	RID reflection_atlas_create() { return RID(); }
	void reflection_atlas_set_size(RID p_ref_atlas, int p_size) {}
	void reflection_atlas_set_subdivision(RID p_ref_atlas, int p_subdiv) {}

	RID reflection_probe_instance_create(RID p_probe) { return RID(); }
	void reflection_probe_instance_set_transform(RID p_instance, const Transform &p_transform) {}
	void reflection_probe_release_atlas_index(RID p_instance) {}
	bool reflection_probe_instance_needs_redraw(RID p_instance) { return false; }
	bool reflection_probe_instance_has_reflection(RID p_instance) { return false; }
	bool reflection_probe_instance_begin_render(RID p_instance, RID p_reflection_atlas) { return false; }
	bool reflection_probe_instance_postprocess_step(RID p_instance) { return true; }

	RID gi_probe_instance_create() { return RID(); }
	void gi_probe_instance_set_light_data(RID p_probe, RID p_base, RID p_data) {}
	void gi_probe_instance_set_transform_to_data(RID p_probe, const Transform &p_xform) {}
	void gi_probe_instance_set_bounds(RID p_probe, const Vector3 &p_bounds) {}

	RID render_buffers_create();
	void render_buffers_configure(RID p_render_buffers, RID p_render_target, int p_width, int p_height, VS::ViewportMSAA p_msaa);

	void render_scene(RID p_render_buffers, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count, RID *p_light_cull_result, int p_light_cull_count, RID *p_reflection_probe_cull_result, int p_reflection_probe_cull_count, RID p_environment, RID p_shadow_atlas, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass);

	int get_roughness_layers() const;
	bool is_using_radiance_cubemap_array() const;

	virtual bool free(RID p_rid);

	virtual void update();

	RasterizerSceneRD(RasterizerStorageRD *p_storage);
};

#endif // RASTERIZER_SCENE_RD_H