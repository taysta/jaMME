@echo off

SET DEMONAME=asdcut
SET WIDTH=3840
SET HEIGHT=2160
SET FPS=60
SET QUALITY=18

start jamme +set com_affinity 0 +set fs_game mme +set fs_extraGames "japlus japp japro" +set mme_renderWidth "%WIDTH%" +set mme_renderHeight "%HEIGHT%" +set mme_demoPrecache 0 +demo %DEMONAME% ^
+exec render_settings +set r_fbo "0" +vid_restart ^
+set mme_pipeCommand "ffmpeg -f avi -i - -threads 0 -preset ultrafast -y -pix_fmt yuv420p -crf %QUALITY% %%o.mp4 2> ffmpeglog.txt" ^
+seek 0 ^
+exec strafe_settings ^
+set viewlog 1 ^
Exit

REM +set mme_pipeCommand "ffmpeg -r 60 -f rawvideo -pix_fmt rgb24 -s %WIDTH%x%HEIGHT% -i - -threads 0 -preset ultrafast -y -pix_fmt yuv420p -vf vflip -crf 18 mme/capture/%DEMONAME%/%DEMONAME%.mp4 2> ffmpeglog.txt" ^
REM +set mme_pipeCommand "ffmpeg -f avi -i - -threads 0 -preset ultrafast -y -pix_fmt yuv420p -crf %QUALITY% %%o.mp4 2> ffmpeglog.txt"