@echo off

SET DEMONAME=thunder-racepack7(jump_green_pro)-jka
SET WIDTH=3840
SET HEIGHT=2160
SET FPS=60
SET QUALITY=18

start jamme +set com_affinity 0 +set fs_game mme +set fs_extraGames "japlus japp japro" +set mme_renderWidth "%WIDTH%" +set mme_renderHeight "%HEIGHT%" +set mme_demoPrecache 0 +demo %DEMONAME% ^
+exec render_settings +set r_fbo "0" +vid_restart ^
+set mme_pipeCommand "ffmpeg -f avi -i - -threads 0 -preset ultrafast -y -pix_fmt yuv420p -crf %QUALITY% %%o.mp4 2> ffmpeglog.txt" ^
+seek 0 ^
+capture PIPE %FPS% %DEMONAME% ^
+exec strafe_settings ^
+set viewlog 1 ^
Exit