# Android fstab file.
# <src>                                         <mnt_point>  <type>  <mnt_flags and options>  <fs_mgr_flags>
# The filesystem that contains the filesystem checker binary (typically /system) cannot
# specify MF_CHECK, and must come before any filesystems that do specify MF_CHECK

/dev/block/platform/msm_sdcc.1/by-name/system       /system         ext4    ro,barrier=1                                                                   wait,first_stage_mount
# mako: real 100MB metadata partition (p26, between cache and userdata)
/dev/block/platform/msm_sdcc.1/by-name/metadata       /metadata       ext4    rw,nosuid,nodev,noexec,barrier=1                                          wait,first_stage_mount
# mako: real /data (userdata). Not first_stage_mount: the first-stage
# switchroot would need /system/data; second-stage mount_all handles it.
/dev/block/platform/msm_sdcc.1/by-name/userdata      /data           ext4    rw,barrier=1                                                                   wait
