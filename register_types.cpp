/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "skeleton_animator.h"

void register_skeleton_animator_types() {
	ClassDB::register_class<SkeletonAnimator>();
}

void unregister_skeleton_animator_types() {
   // 
}
