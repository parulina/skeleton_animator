#include "skeleton_animator.h"
#include "core/method_bind_ext.gen.inc"


void SkeletonAnimator::set_target_skeleton_path(const NodePath& p_target_skeleton_path) {
	target_skeleton_path = p_target_skeleton_path;
	target_skeleton = Object::cast_to<Skeleton>(get_node(target_skeleton_path));

	bone_id_mapping.clear();
	if (target_skeleton) {
		for (int bidx = 0; bidx < target_skeleton->get_bone_count(); bidx ++) {
			String bone_name = target_skeleton->get_bone_name(bidx);
			bone_id_mapping[bone_name] = bidx;
		}
	}
}

void SkeletonAnimator::set_animation(Ref<Animation> p_animation) {
	animation = p_animation->duplicate(false);

	position = -1;
	loop_count = -1;
}
Ref<Animation> SkeletonAnimator::get_animation() {
	return animation;
}

void SkeletonAnimator::play(float p_time, int p_loop, float p_speed) {
	if (!animation.is_valid()) {
		return;
	}

	position = p_time;
	speed = p_speed;

	state = ANIMATION_STATE_STARTING;
	fade_multiplier = 1.0f;
	fade_position = 0.0f;

	loop_start = 0.0f;
	loop_end = 1.0f;

	loop_count = p_loop;

	bone_uses.clear();

	for (int tk = 0; tk < animation->get_track_count(); tk++) {
		NodePath path = animation->track_get_path(tk);
		if (path.get_subname_count() == 1) {
			String bone_name = path.get_subname(0);
			bone_uses[bone_name] = animation->track_get_key_count(tk);

			if (bone_name == "animCtrl") {
				int key_count = animation->track_get_key_count(tk);
				if (key_count >= 1) {
					int key_index = 0;
					for (int k = 0; k < key_count; k++) {
						if (animation->track_get_type(tk) != Animation::TYPE_TRANSFORM)
							continue;
						Dictionary frame = animation->track_get_key_value(tk, k);
						Vector3 location = frame.get("location", Vector3());
						float pos_y = roundf(location.y * 10.0f) / 10.0f;
						if ((key_index == 0) && (pos_y == 0.0f)) {
							key_index += 1;
						}
						else if ((key_index == 1 && pos_y == 1.0f) || (key_index == 2 && pos_y != 1.0f)) {
							float tt = animation->track_get_key_time(tk, k);
							tt = ceilf(tt * 24.0f) / 24.0f;
							if (key_index == 1) {
								loop_start = tt;
							}
							if (key_index == 2) {
								loop_end = tt;
							}
							key_index += 1;
						}
					}
				}
			}
		}
	}

	playing = true;
	emit_signal("animation_change", get_animation_name(), loop_count, position, speed);
	emit_signal("animation_start", get_animation_name());
}


void SkeletonAnimator::set_position(float p_position) {
	position = p_position;
}
float SkeletonAnimator::get_position() const {
	return position;
}


void SkeletonAnimator::set_multiplier(float p_multiplier) {
	multiplier = p_multiplier;
}
float SkeletonAnimator::get_multiplier() const {
	return multiplier;
}

float SkeletonAnimator::get_animation_looped_length() const {
	if (!animation.is_valid()) {
		return 0.0f;
	}
	if (loop_count == -1) {
		return -1.0f;
	}
	float middle = animation->get_length() - (loop_start + (animation->get_length() - loop_end));
	return (middle + middle * loop_count);
}

String SkeletonAnimator::get_animation_name() const {
	if (!animation.is_valid())
		return "";
	return animation->get_name();
}

void SkeletonAnimator::process_animation(float delta) {
	if (!animation.is_valid()) {
		return;
	}
	float delta_fix = delta * speed;

	if (fade) {
		if (state == ANIMATION_STATE_STARTING) {
			if (loop_start < 0.1f) {
				// If animation doesn't have a startup (as controlled by anim bone)
				fade_multiplier = CLAMP((fade_position / fade_ratio), 0.0f, 1.0f);
				if (fade_position < (loop_start > 0.0f ? loop_start : fade_ratio)) {
					fade_position += delta_fix;
					delta_fix = 0;
				} else {
					fade_multiplier = 1.0f;
				}
			}
			if (position > loop_start) {
				fade_multiplier = 1.0f;
				fade_position = fade_ratio;
			}
		} else if (state == ANIMATION_STATE_EXITING) {
			float anim_length = animation->get_length();
			if ((anim_length - loop_end) < 0.1f && (position + delta_fix) > animation->get_length()) {
				// Only fade if the end length is very short
				if (fade_position > fade_ratio) {
					fade_position = fade_ratio;
				}
				fade_multiplier *= CLAMP((fade_position / fade_ratio), 0.0f, 1.0f);
				if (fade_position > 0.0f) {
					fade_position -= delta_fix;
					delta_fix = 0.0f;
				}
			} else {
				float fade_length = (anim_length * fade_ratio);
				fade_multiplier *= 1.0 - CLAMP((position - (anim_length - fade_length)) / fade_length, 0.0f, 1.0f);
			}
		}
	}
	if (playing) {
		if (state == ANIMATION_STATE_STARTING) {
			if (position > loop_start) {
				if (loop_count == 0) {
					state = ANIMATION_STATE_EXITING;
				} else {
					state = ANIMATION_STATE_LOOPING;
				}
				emit_signal("animation_change", get_animation_name(), loop_count, position, speed);
			}
		} else if (state == ANIMATION_STATE_LOOPING) {
			if (roundf((position + delta_fix) * 24.0) >= roundf(loop_end * 24.0)) {
				if (loop_count > 0)
					loop_count -= 1;
				if (loop_count == -1 || ((loop_count >= 0 && loop_end > 0.9f) || (loop_count >= 1))) {
					position = loop_start;
				}
				if (loop_count == 0) {
					state = ANIMATION_STATE_EXITING;
				}
			}
			if (loop_start >= animation->get_length()) {
				state = ANIMATION_STATE_EXITING;
				stop(true);
			}
		} else if (state == ANIMATION_STATE_EXITING) {
			if ((position + delta_fix) > animation->get_length()) {
				state = ANIMATION_STATE_EXITED;
				stop(true);
			}
		}

		if (animation.is_valid()) {
			if ((loop_count == -1) || (position < animation->get_length())) {
				apply_animation(apply_mode, animation, position, multiplier * fade_multiplier, bone_filter, mirror);
				position += delta_fix;
			}
		}
	}
}

void SkeletonAnimator::apply_animation(SkeletonAnimator::AnimationApplyMode apply_mode, Ref<Animation> p_animation, float p_position, float p_multiplier, PoolStringArray p_bone_filter, bool p_mirror) {
	if (!p_animation.is_valid())
		return;
	if (target_skeleton == nullptr)
		return;

	for (int tk = 0; tk < p_animation->get_track_count(); tk++) {
		Vector3 transform_position;
		Quat transform_rotation;
		Vector3 transform_scale;
		 // This was for specifying 0.0 - 1.0
		//pos = CLAMP(position, 0.0, 1.0) * p_animation->get_length();
		if (p_animation->transform_track_interpolate(tk, p_position, &transform_position, &transform_rotation, &transform_scale) != OK) {
			continue;
		}
		NodePath name = p_animation->track_get_path(tk);
		if (name.get_subname_count() == 1) {
			String bone = name.get_subname(0);
			if (!p_bone_filter.empty()) {
				bool include = false;
				for (int i = 0; i < p_bone_filter.size(); i++) {
					if (bone.begins_with(p_bone_filter.get(i))) {
						if (p_bone_filter.has("!L") && bone.ends_with("_L"))
							continue;
						if (p_bone_filter.has("!R") && bone.ends_with("_R"))
							continue;
						include = true;
					}
				}
				if (!include)
					continue;
			}
			if (p_mirror) {
				if (bone.ends_with("_L"))
					bone = bone.substr(0, bone.length() - 1) + "R";
				if (bone.ends_with("_R"))
					bone = bone.substr(0, bone.length() - 1) + "L";
				transform_rotation.x *= -1;
				transform_rotation.w *= -1;
			}

			int bone_idx = bone_id_mapping.get(bone, -1);
			if (bone_idx == -1) {
				bone_idx = target_skeleton->find_bone(bone);
			}
			if (bone_idx != -1) {
				Transform trans = target_skeleton->get_bone_pose(bone_idx);
				//Transform trans = bone_transform_cache[bone];
				if (apply_mode == ANIMATION_APPLY_MODE_ADDITIVE) {
					Transform trans2 = trans.translated(transform_position);
					trans2.basis = Basis(trans2.basis.orthonormalized().get_rotation_quat() * transform_rotation).scaled(trans.basis.get_scale()).scaled(transform_scale);

					trans = trans.interpolate_with(trans2, p_multiplier);
				} else if (apply_mode == ANIMATION_APPLY_MODE_OVERRIDE) {
					Transform trans2 = Transform(Basis(transform_rotation).scaled(transform_scale), transform_position);
					trans = trans.interpolate_with(trans2, p_multiplier);
				}
				//bone_transform_cache[bone] = trans;
				target_skeleton->set_bone_pose(bone_idx, trans);
			}
		}
	}
}


bool SkeletonAnimator::is_playing() const {
	if (!animation.is_valid())
		return false;
	return (state != ANIMATION_STATE_NONE && state != ANIMATION_STATE_EXITED);
}
bool SkeletonAnimator::is_middle() const {
	if (!animation.is_valid())
		return false;
	if (loop_count > 0) {
		return (state == ANIMATION_STATE_LOOPING);
	}
	return is_playing();
}
bool SkeletonAnimator::is_looping() const {
	if (!animation.is_valid())
		return false;
	return (position > loop_start && position < loop_end);
}
bool SkeletonAnimator::is_exiting() const {
	if (!animation.is_valid())
		return false;
	return (state == ANIMATION_STATE_EXITING);
}
bool SkeletonAnimator::has_ended() const {
	if (!animation.is_valid())
		return false;
	return (state == ANIMATION_STATE_EXITED && position >= animation->get_length());
}


bool SkeletonAnimator::is_using_bone(const String& p_bone) const {
	return bone_uses.has(p_bone);
}
int SkeletonAnimator::get_bone_uses(const String& p_bone) const {
	return bone_uses.get(p_bone, 0);
}


void SkeletonAnimator::stop(bool immediate) {
	if (!animation.is_valid())
		return;

	loop_count = 0;
	if (immediate) {
		speed = 1.0f;
		emit_signal("animation_change", "", 0, -1, speed);
		emit_signal("animation_end", get_animation_name());
		animation.unref();
		state = ANIMATION_STATE_EXITED;
		fade_multiplier = 0.0f;
		position = 0.0f;
	} else if (state != ANIMATION_STATE_EXITING) {
		emit_signal("animation_change", get_animation_name(), 0, position, speed);
		if (state != ANIMATION_STATE_EXITED) {
			state = ANIMATION_STATE_EXITING;
		}
	}
}

void SkeletonAnimator::set_playing_speed(float p_speed) {
	speed = p_speed;
	if (animation.is_valid()) {
		emit_signal("animation_change", get_animation_name(), loop_count, position, speed);
	}
}

float SkeletonAnimator::get_playing_speed() const {
	return speed;
}

void SkeletonAnimator::set_fade(bool p_fade) {
	fade = p_fade;
}
bool SkeletonAnimator::is_fade() const {
	return fade;
}

void SkeletonAnimator::set_loop_count(int p_loop) {
	loop_count = p_loop;
}
int SkeletonAnimator::get_loop_count() const {
	return loop_count;
}


void SkeletonAnimator::set_bone_filter(PoolStringArray p_bones) {
	bone_filter = p_bones;
}

PoolStringArray SkeletonAnimator::get_bone_filter() const {
	return bone_filter;
}

void SkeletonAnimator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_animation"), &SkeletonAnimator::set_animation);
	ClassDB::bind_method(D_METHOD("get_animation"), &SkeletonAnimator::get_animation);

	ClassDB::bind_method(D_METHOD("set_position"), &SkeletonAnimator::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &SkeletonAnimator::get_position);
	ClassDB::bind_method(D_METHOD("set_multiplier"), &SkeletonAnimator::set_multiplier);
	ClassDB::bind_method(D_METHOD("get_multiplier"), &SkeletonAnimator::get_multiplier);

	ClassDB::bind_method(D_METHOD("is_playing"), &SkeletonAnimator::is_playing);
	ClassDB::bind_method(D_METHOD("is_middle"), &SkeletonAnimator::is_middle);
	ClassDB::bind_method(D_METHOD("is_looping"), &SkeletonAnimator::is_looping);
	ClassDB::bind_method(D_METHOD("has_ended"), &SkeletonAnimator::has_ended);
	ClassDB::bind_method(D_METHOD("is_exiting"), &SkeletonAnimator::is_exiting);

	ClassDB::bind_method(D_METHOD("play"), &SkeletonAnimator::play);
	ClassDB::bind_method(D_METHOD("stop"), &SkeletonAnimator::stop);
	ClassDB::bind_method(D_METHOD("is_using_bone"), &SkeletonAnimator::is_using_bone);
	ClassDB::bind_method(D_METHOD("get_bone_uses"), &SkeletonAnimator::get_bone_uses);

	ClassDB::bind_method(D_METHOD("set_fade"), &SkeletonAnimator::set_fade);
	ClassDB::bind_method(D_METHOD("is_fade"), &SkeletonAnimator::is_fade);

	ClassDB::bind_method(D_METHOD("set_bone_filter"), &SkeletonAnimator::set_bone_filter);
	ClassDB::bind_method(D_METHOD("get_bone_filter"), &SkeletonAnimator::get_bone_filter);

	//ClassDB::bind_method(D_METHOD(""), &);

	ClassDB::bind_method(D_METHOD("set_playing_speed"), &SkeletonAnimator::set_playing_speed);
	ClassDB::bind_method(D_METHOD("get_playing_speed"), &SkeletonAnimator::get_playing_speed);

	ClassDB::bind_method(D_METHOD("set_loop_count"), &SkeletonAnimator::set_loop_count);
	ClassDB::bind_method(D_METHOD("get_loop_count"), &SkeletonAnimator::get_loop_count);

	ClassDB::bind_method(D_METHOD("get_animation_looped_length"), &SkeletonAnimator::get_animation_looped_length);
	ClassDB::bind_method(D_METHOD("get_animation_name"), &SkeletonAnimator::get_animation_name);

	ClassDB::bind_method(D_METHOD("set_target_skeleton_path"), &SkeletonAnimator::set_target_skeleton_path);

	ClassDB::bind_method(D_METHOD("process_animation"), &SkeletonAnimator::process_animation);
	ClassDB::bind_method(D_METHOD("apply_animation"), &SkeletonAnimator::apply_animation);

	

	BIND_ENUM_CONSTANT(ANIMATION_STATE_EXITED);
	BIND_ENUM_CONSTANT(ANIMATION_STATE_EXITING);
	BIND_ENUM_CONSTANT(ANIMATION_STATE_LOOPING);
	BIND_ENUM_CONSTANT(ANIMATION_STATE_NONE);
	BIND_ENUM_CONSTANT(ANIMATION_STATE_STARTING);

	BIND_ENUM_CONSTANT(ANIMATION_APPLY_MODE_ADDITIVE);
	BIND_ENUM_CONSTANT(ANIMATION_APPLY_MODE_OVERRIDE);

	ADD_SIGNAL(MethodInfo("animation_change"));
	ADD_SIGNAL(MethodInfo("animation_start"));
	ADD_SIGNAL(MethodInfo("animation_end"));
}

/*

func save_state():
	return {
		"animation": animationCache,
		"animname": animationName,
		"animstate": animState,
		"loopstart": loopStart,
		"loopend": loopEnd,
		"looping": looping,
		"playing": playing,
	}

func load_state(state):
	animationCache = state.animation
	animationName = state.animname
	animState = state.animstate
	loopStart = state.loopstart
	loopEnd = state.loopend
	looping = state.looping
	playing = state.playing
	progress = 0.0


*/

SkeletonAnimator::SkeletonAnimator() {

}

SkeletonAnimator::~SkeletonAnimator() {
}


