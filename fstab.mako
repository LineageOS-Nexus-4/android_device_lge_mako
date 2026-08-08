# Android fstab file.
# <src>                                         <mnt_point>  <type>  <mnt_flags and options>  <fs_mgr_flags>
# The filesystem that contains the filesystem checker binary (typically /system) cannot
# specify MF_CHECK, and must come before any filesystems that do specify MF_CHECK

/dev/block/platform/msm_sdcc.1/by-name/system       /system         ext4    ro,barrier=1                                                                   wait,first_stage_mount
# mako: spare 250MB "vendor" partition (unused by this build) used as persistent /metadata
/dev/block/platform/msm_sdcc.1/by-name/vendor       /metadata       ext4    rw,nosuid,nodev,noexec,barrier=1                                          wait,first_stage_mount
# mako: real /data (userdata). Not first_stage_mount: the first-stage
# switchroot would need /system/data; second-stage mount_all handles it.
/dev/block/platform/msm_sdcc.1/by-name/userdata      /data           ext4    rw,barrier=1                                                                   wait
