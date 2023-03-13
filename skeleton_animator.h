#ifndef SKELETON_ANIMATOR_H
#define SKELETON_ANIMATOR_H

#include "core/math/transform.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "scene/animation/animation_player.h"
#include "scene/3d/skeleton.h"
#include "scene/resources/packed_scene.h"


class SkeletonAnimator : public Node {
	GDCLASS(SkeletonAnimator, Node);

	public:

		// EXPORT

		enum AnimationApplyMode {
			ANIMATION_APPLY_MODE_ADDITIVE,
			ANIMATION_APPLY_MODE_OVERRIDE,
			ANIMATION_APPLY_MODE_OVERRIDE_ROTATION,
		};
		AnimationApplyMode apply_mode = ANIMATION_APPLY_MODE_OVERRIDE;

		enum AnimationState {
			ANIMATION_STATE_NONE,
			ANIMATION_STATE_STARTING,
			ANIMATION_STATE_LOOPING,
			ANIMATION_STATE_EXITING,
			ANIMATION_STATE_EXITED,
		};
		AnimationState state = ANIMATION_STATE_NONE;

		NodePath target_skeleton_path;
		Skeleton *target_skeleton = nullptr;

		Dictionary bone_id_mapping = Dictionary();

		Ref<Animation>	animation;

		bool		playing = false;
		int			loop_count = 0;
		float		position = 0.0f;

		bool		fade = false;
		float		fade_ratio = 0.25;
		float		fade_position = 0.0f;
		float		fade_multiplier = 1.0;

		float		loop_start = 0.0f;
		float		loop_end = 0.0f;

		float		speed = 1.0f;
		float		multiplier = 1.0f;
		bool		mirror = false;



		bool uses_left_arm = false;
		bool uses_right_arm = false;

		PoolStringArray bone_filter = PoolStringArray();


		void set_target_skeleton_path(const NodePath &p_target_skeleton_path);

		void set_animation(Ref<Animation> p_animation);
		Ref<Animation> get_animation();

		void set_position(float p_position);
		float get_position() const;

		void set_multiplier(float p_multiplier);
		float get_multiplier() const;

		bool is_playing() const;
		bool is_middle() const;
		bool is_looping() const;
		bool has_ended() const;

		bool is_using_left_arm() const;
		bool is_using_right_arm() const;

		void play(float p_time = 0.0f, int p_loop = 1, float p_speed = 1.0f);
		void stop(bool immediate);

		void set_playing_speed(float p_speed);
		float get_playing_speed() const;

		void set_fade(bool p_fade);
		bool is_fade() const;

		void set_loop_count(int p_loop);
		int get_loop_count() const;

		void set_bone_filter(PoolStringArray p_bones);
		PoolStringArray get_bone_filter() const;

		float get_animation_looped_length() const;
		String get_animation_name() const;

		void process_animation(float delta);
		void apply_animation(SkeletonAnimator::AnimationApplyMode apply_mode, Ref<Animation> p_animation, float p_position, float p_multiplier, PoolStringArray bone_filter, bool mirror = false);

		SkeletonAnimator();
		~SkeletonAnimator();

	protected:
		static void _bind_methods();


};

VARIANT_ENUM_CAST(SkeletonAnimator::AnimationState);
VARIANT_ENUM_CAST(SkeletonAnimator::AnimationApplyMode);

#endif // SKELETON_ANIMATOR_H
