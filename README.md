# Skeleton Animator for Godot 3.5
Custom Godot 3.5 animator module for Kemoverse Online
Quick dirty code to create and use customizable/filtered animations

# Usage
Initialize a SkeletonAnimator child and point it to the skeleton
```
var sa = SkeletonAnimator.new()

sa.name = "Animator"
sa.connect("animation_change", self, "anim_block_change")
sa.connect("animation_start", self, "anim_block_start")
sa.connect("animation_end", self, "anim_block_end")
self.add_child(sa)

sa.set_target_skeleton_path(skeleton.get_path())
```

Apply immediate animation resource to skeleton:
```
# apply mode (_OVERRIDE or _ADDITIVE), animation, start time, multiplier (how strong), bone filter (poolstringarray), mirror
sa.apply_animation(SkeletonAnimator.ANIMATION_APPLY_MODE_OVERRIDE, animation, 0.0, 1.0, PoolStringArray([]), false)
```

Set an animation and play it:
```
sa.set_animation(animation)
# start time, loops, speed
sa.play(0.0, 4, 1.0)
# Set animation to only play on head*, arm* bone, exclude any bones ending in _L
sa.set_bone_filter(PoolStringArray(["head", "arm", "!L"]))
```

Don't forget to process, if you're using the built in anim system:
```
func _process(delta):
    sa.process_animation(delta)
```
