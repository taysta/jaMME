@echo off

SET DEMONAME=simple-jka

start jamme +set com_affinity 0 +set mme_demoPrecache 0 +set fs_game mme +exec minimal_settings +vid_restart +demo %DEMONAME% ^
+set viewlog 1 ^
+speed 1 ^
+cg_logStrafeTrail %DEMONAME%
Exit