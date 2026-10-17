/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <vlc/vlc.h>
#include <windows.h>
#include <stdio.h>
int main(int argc,char **argv) {
 if(argc!=2)return 2;
 const char *args[]={"--ignore-config","--aout=dummy","--vout=dummy","--avcodec-hw=none"};
 libvlc_instance_t *instance=libvlc_new(4,args);if(!instance)return 3;
 libvlc_media_t *media=libvlc_media_new_path(instance,argv[1]);
 libvlc_media_add_option(media,":stop-time=1");
 libvlc_media_player_t *player=libvlc_media_player_new_from_media(media);libvlc_media_player_play(player);
 DWORD start=GetTickCount();while(libvlc_media_player_get_state(player)!=libvlc_Ended && GetTickCount()-start<12000)Sleep(100);
 printf("state %d before stop\n",libvlc_media_player_get_state(player));fflush(stdout);
 libvlc_media_player_stop(player);libvlc_media_player_release(player);libvlc_media_release(media);libvlc_release(instance);
 puts("clean shutdown");return 0;
}
