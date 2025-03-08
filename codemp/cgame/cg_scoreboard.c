// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/bg_saga.h"

#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	25
#define SB_INTER_HEIGHT		15 // interleaved height

#define SB_MAXCLIENTS_NORMAL  0
//((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X_CONST (SCOREBOARD_X+32)
#define SB_HEAD_X_CONST (SCOREBOARD_X+64)
static int SB_BOTICON_X = (SCOREBOARD_X+32);
static int SB_HEAD_X = (SCOREBOARD_X+64);

static int SB_SCORELINE_X = 100;
#define SB_SCORELINE_X_CONST		100
#define SB_SCORELINE_WIDTH_CONST	(640 - SB_SCORELINE_X_CONST * 2)
static int SB_SCORELINE_WIDTH = (640 - SB_SCORELINE_X_CONST * 2);

#define SB_RATING_WIDTH	    0 // (6 * BIGCHAR_WIDTH)
/*static int SB_NAME_X = (SB_SCORELINE_X_CONST);
static int SB_SCORE_X = (SB_SCORELINE_X_CONST + .55 * SB_SCORELINE_WIDTH_CONST);
static int SB_PING_X = (SB_SCORELINE_X_CONST + .75 * SB_SCORELINE_WIDTH_CONST);
static int SB_TIME_X = (SB_SCORELINE_X_CONST + .90 * SB_SCORELINE_WIDTH_CONST);*/

static int SB_TIME_X = (SB_SCORELINE_X_CONST + .07 * SB_SCORELINE_WIDTH_CONST);
static int SB_PING_X = (SB_SCORELINE_X_CONST + .22 * SB_SCORELINE_WIDTH_CONST);
static int SB_SCORE_X = (SB_SCORELINE_X_CONST + .30 * SB_SCORELINE_WIDTH_CONST);
static int SB_NAME_X = (SB_SCORELINE_X_CONST + .48 * SB_SCORELINE_WIDTH_CONST);

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed

static void CG_DrawSpectated(float x, float y, score_t *score, float scale) {
	int i, client;
	qboolean samePingFound = qfalse;
	if (score->team != TEAM_SPECTATOR)
		return;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_SPECTATOR)
			continue;
		if (cg.scores[i].ping == score->ping) {
			if (samePingFound)
				return;
			samePingFound = qtrue;
			client = cg.scores[i].client;
		}
	}
	if (samePingFound)
		CG_Text_Paint(x, y, 1.0f*scale, colorWhite, va("@ %s", cgs.clientinfo[client].name), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL);
}
/*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int y, score_t *score, float *color, float fade, qboolean largeFormat ) 
{
	//vec3_t	headAngles;
	clientInfo_t	*ci;
	int iconx, headx;
	float		scale;

	if ( largeFormat )
	{
		scale = 1.0f;
	}
	else
	{
		scale = 0.75f;
	}

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}
	
	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) )
	{
		if( largeFormat )
		{
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32*cgs.widthRatioCoef, 32, TEAM_FREE, qfalse );
		}
		else
		{
			CG_DrawFlagModel( iconx, y, 16*cgs.widthRatioCoef, 16, TEAM_FREE, qfalse );
		}
	}
	else if ( ci->powerups & ( 1 << PW_REDFLAG ) )
	{
		if( largeFormat )
		{
			CG_DrawFlagModel( iconx, y, 32*cgs.widthRatioCoef, 32, TEAM_RED, qfalse );
		}
		else
		{
			CG_DrawFlagModel( iconx, y, 32*cgs.widthRatioCoef, 32, TEAM_RED, qfalse );
		}
	}
	else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) )
	{
		if( largeFormat )
		{
			CG_DrawFlagModel( iconx, y, 32*cgs.widthRatioCoef, 32, TEAM_BLUE, qfalse );
		}
		else
		{
			CG_DrawFlagModel( iconx, y, 32*cgs.widthRatioCoef, 32, TEAM_BLUE, qfalse );
		}
	}
	else if (cgs.gametype == GT_POWERDUEL &&
		(ci->duelTeam == DUELTEAM_LONE || ci->duelTeam == DUELTEAM_DOUBLE))
	{
		if (ci->duelTeam == DUELTEAM_LONE)
		{
			//need to test in power duel demo
			CG_DrawPic ( iconx, y, 16*cgs.widthRatioCoef, 16, trap_R_RegisterShaderNoMip ( "gfx/mp/pduel_icon_lone" ) );
		}
		else
		{
			//need to test in power duel demo
			CG_DrawPic ( iconx, y, 16*cgs.widthRatioCoef, 16, trap_R_RegisterShaderNoMip ( "gfx/mp/pduel_icon_double" ) );
		}
	}
	else if (cgs.gametype == GT_SIEGE)
	{ //try to draw the shader for this class on the scoreboard
		if (ci->siegeIndex != -1)
		{
			siegeClass_t *scl = &bgSiegeClasses[ci->siegeIndex];

			if (scl->classShader)
			{
				//need to test in siege demo
				CG_DrawPic (iconx, y, (largeFormat?24:12)*cgs.widthRatioCoef, largeFormat?24:12, scl->classShader);
			}
		}
	}
	else
	{
		// draw the wins / losses
		/*
		if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) 
		{
			CG_DrawSmallStringColor( iconx, y + SMALLCHAR_HEIGHT/2, va("%i/%i", ci->wins, ci->losses ), color );
		}
		*/
		//rww - in duel, we now show wins/losses in place of "frags". This is because duel now defaults to 1 kill per round.
	}

	// highlight your position
	if ( score->client == cg.playerCent->currentState.number ) 
	{
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR 
			|| cgs.gametype >= GT_TEAM ) {
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if ( rank == 0 ) {
#ifdef TEAM_GREEN
			hcolor[0] = 0;
			hcolor[1] = 0.6f;
			hcolor[2] = 0;
#else
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
#endif
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.4;
		CG_FillRect( SB_SCORELINE_X - 5, y + 2, SB_SCORELINE_WIDTH + 10, largeFormat?SB_NORMAL_HEIGHT:SB_INTER_HEIGHT, hcolor );
	}
	
	CG_Text_Paint (SB_NAME_X, y, 1.0f * scale, colorWhite, ci->name,0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	
	if ( score->ping != -1 )
	{
		if ( ci->team != TEAM_SPECTATOR || cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL )
		{
			if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
			{
				CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i/%i", ci->wins, ci->losses), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i/%i", ci->wins, ci->losses),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
			else
			{
				if ((cg.japlus.detected || cg.japro.detected) && cg_japlusFix.integer) {
					CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i/%i", score->score, score->deaths), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i/%i", score->score, score->deaths),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
				} else {
					CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i", score->score), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i", score->score),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
				}
			}
		}

		CG_Text_Paint (SB_PING_X - CG_Text_Width( va("%i", score->ping), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i", score->ping),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );	
		CG_Text_Paint (SB_TIME_X - CG_Text_Width( va("%i", score->time), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i", score->time),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );		
		//entTODO: implement spectated for non-team games
		if (cgs.gametype >= GT_TEAM)
			CG_DrawSpectated(320 + (SB_SCORELINE_X / 2), y, score, scale);
	}
	else
	{
		CG_Text_Paint (SB_SCORE_X - CG_Text_Width( "-", 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, "-",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint (SB_PING_X - CG_Text_Width( "-", 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, "-",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );	
		CG_Text_Paint (SB_TIME_X - CG_Text_Width( "-", 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, "-",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) 
	{
		if( SB_SCORELINE_X < 320 ) {
			CG_Text_Paint (SB_SCORELINE_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "READY"), 0.7f * scale, FONT_MEDIUM ) - 10, y + 2, 0.7f * scale, colorWhite, CG_GetStringEdString("MP_INGAME", "READY"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
		} else {
			CG_Text_Paint (SB_SCORELINE_X + SB_SCORELINE_WIDTH + 10, y + 2, 0.7f * scale, colorWhite, CG_GetStringEdString("MP_INGAME", "READY"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
		}
	}
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly ) 
{
	int		i, avgping = 0;
	score_t	*score;
	float	color[4], scale;
	int		count;
	clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;
	
	if ( lineHeight == SB_NORMAL_HEIGHT )
	{
		scale = 1.0f;
	}
	else
	{
		scale = 0.75f;
	}

	count = 0;
	if ( cgs.gametype >= GT_TEAM ) {
		if( team == TEAM_RED ) { //red team on left
			if( !countOnly ) {
				CG_Text_Paint ((SB_NAME_X + SB_SCORELINE_X + SB_SCORELINE_WIDTH ) / 2 - CG_Text_Width( "^1Red Team", 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, "^1Red Team",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
				CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i", cg.teamScores[0]), 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, va("%i", cg.teamScores[0]),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
			count++;
			maxClients++;
		} else if( team == TEAM_BLUE ) {
			if( !countOnly ) {
#ifdef TEAM_GREEN
				CG_Text_Paint ((SB_NAME_X + SB_SCORELINE_X  + SB_SCORELINE_WIDTH ) / 2 - CG_Text_Width( "^2Green Team", 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, "^2Green Team",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
				CG_Text_Paint ((SB_NAME_X + SB_SCORELINE_X  + SB_SCORELINE_WIDTH ) / 2 - CG_Text_Width( "^4Blue Team", 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, "^4Blue Team",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#endif
				CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i", cg.teamScores[1]), 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, va("%i", cg.teamScores[1]),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
			count++;
			maxClients++;
		}
	}
	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		if ( !countOnly )
		{
			CG_DrawClientScore( y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );
			if( score->ping != -1 ) avgping += score->ping;
		}

		count++;
	}
	
	if( !countOnly && cgs.gametype >= GT_TEAM && (team == TEAM_RED || team == TEAM_BLUE ) ) {
		if( count-1 )avgping /= count-1;
		CG_Text_Paint (SB_PING_X - CG_Text_Width( va("%i", avgping), 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, va("%i", avgping),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}

	return count;
}

int	CG_GetClassCount(team_t team,int siegeClass )
{
	int i = 0;
	int count = 0;
	clientInfo_t	*ci;
	siegeClass_t *scl;

	for ( i = 0 ; i < cgs.maxclients ; i++ )
	{
		ci = &cgs.clientinfo[ i ];

		if ((!ci->infoValid) || ( team != ci->team ))
		{
			continue;
		}

		scl = &bgSiegeClasses[ci->siegeIndex];

		// Correct class?
		if ( siegeClass != scl->classShader )
		{
			continue;
		}

		count++;
	}

 	return count;

}

int CG_GetTeamNonScoreCount(team_t team)
{
	int i = 0,count=0;
	clientInfo_t	*ci;

	for ( i = 0 ; i < cgs.maxclients ; i++ )
	{
		ci = &cgs.clientinfo[ i ];

		if ( (!ci->infoValid) || (team != ci->team && team != ci->siegeDesiredTeam) )
		{
			continue;
		}

		count++;
	}

 	return count;
}

int CG_GetTeamCount(team_t team, int maxClients)
{
	int i = 0;
	int count = 0;
	clientInfo_t	*ci;
	score_t	*score;

	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ )
	{
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team )
		{
			continue;
		}

		count++;
	}

	return count;

}
/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
int cg_siegeWinTeam = 0;
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;
	float	mySBScale;
	int totalClients = 0, totalPrivate = 0;

	// don't draw amuthing if the menu or console is up
	if ( cl_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || ( cg.playerPredicted && cg.predictedPlayerState.pm_type == PM_DEAD )
		|| ( !cg.playerPredicted && ( cg.playerCent->currentState.eFlags & EF_DEAD ) )
		|| cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}
	
	mySBScale = 1.0f;
	SB_SCORELINE_WIDTH = SB_SCORELINE_WIDTH_CONST;
	SB_SCORELINE_X = SB_SCORELINE_X_CONST;
	SB_BOTICON_X = SB_BOTICON_X_CONST;
	SB_HEAD_X = SB_HEAD_X_CONST;
//	if ( cgs.gametype == GT_CTF ) {
	if ( cgs.gametype >= GT_TEAM ) {
		SB_SCORELINE_X /= 2;
		SB_SCORELINE_WIDTH = (SB_SCORELINE_WIDTH + SB_SCORELINE_X) / 2;
		SB_BOTICON_X /= 2;
		SB_HEAD_X /= 2;
		mySBScale = 0.75f;
	}

	// fragged by ... line
	// or if in intermission and duel, prints the winner of the duel round
	if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.duelWinner != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		s = va("%s^7 %s", cgs.clientinfo[cgs.duelWinner].name, CG_GetStringEdString("MP_INGAME", "DUEL_WINS") );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = ( SCREEN_WIDTH ) / 2;
		y = 40;
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.duelist1 != -1 && cgs.duelist2 != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			s = va("%s^7 %s %s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name, CG_GetStringEdString("MP_INGAME", "AND"), cgs.clientinfo[cgs.duelist3].name );
		}
		else
		{
			s = va("%s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name );
		}
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = ( SCREEN_WIDTH ) / 2;
		y = 40;
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ( cg.killerName[0] ) {
		s = va("%s %s", CG_GetStringEdString("MP_INGAME", "KILLEDBY"), cg.killerName );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = ( SCREEN_WIDTH ) / 2;
		y = 40;
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}

	// current rank
	if (cgs.gametype == GT_POWERDUEL)
	{ //do nothing?
	}
	else if ( cgs.gametype < GT_TEAM) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) 
		{
			char sPlace[256];
			char sOf[256];
			char sWith[256];

			trap_SP_GetStringTextString("MP_INGAME_PLACE",	sPlace,	sizeof(sPlace));
			trap_SP_GetStringTextString("MP_INGAME_OF",		sOf,	sizeof(sOf));
			trap_SP_GetStringTextString("MP_INGAME_WITH",	sWith,	sizeof(sWith));

			s = va("%s %s (%s %i) %s %i",
				CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
				sPlace,
				sOf,
				cg.numScores,
				sWith,
				cg.snap->ps.persistant[PERS_SCORE] );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH ) / 2;
			y = 60;
			//CG_DrawBigString( x, y, s, fade );
			UI_DrawProportionalString(x, y, s, UI_CENTER|UI_DROPSHADOW, colorTable[CT_WHITE]);
		}
	}
	else if (cgs.gametype != GT_SIEGE)
	{
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
#ifdef TEAM_GREEN
			s = va("^1Red Team^7 and ^2Green Team^7 are tied at %d", cg.teamScores[0] );
#else
			s = va("%s %i", CG_GetStringEdString("MP_INGAME", "TIEDAT"), cg.teamScores[0] );
#endif
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
#ifdef TEAM_GREEN
			s = va( "^1Red Team^7 leads with %d, ^2Green Team^7 has %d", cg.teamScores[0], cg.teamScores[1] );
#else
			s = va("%s, %i / %i", CG_GetStringEdString("MP_INGAME", "RED_LEADS"), cg.teamScores[0], cg.teamScores[1] );
#endif
		} else {
#ifdef TEAM_GREEN
			s = va( "^2Green Team^7 leads with %d, ^1Red Team^7 has %d", cg.teamScores[1], cg.teamScores[0] );
#else
			s = va("%s, %i / %i", CG_GetStringEdString("MP_INGAME", "BLUE_LEADS"), cg.teamScores[1], cg.teamScores[0] );
#endif
		}

		x = ( SCREEN_WIDTH ) / 2;
		y = 60;
		
		CG_Text_Paint ( /*x - CG_Text_Width ( s, 1.0f, FONT_SMALL ) / 2*/SB_SCORELINE_X, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
	else if (cgs.gametype == GT_SIEGE && (cg_siegeWinTeam == 1 || cg_siegeWinTeam == 2))
	{
		if (cg_siegeWinTeam == 1)
		{
			s = va("%s", CG_GetStringEdString("MP_INGAME", "SIEGETEAM1WIN") );
		}
		else
		{
			s = va("%s", CG_GetStringEdString("MP_INGAME", "SIEGETEAM2WIN") );
		}

		x = ( SCREEN_WIDTH ) / 2;
		y = 60;
		
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}

	// scoreboard
	
	//y = SB_HEADER + (int)(32.0f - (32.0f * mySBScale ) )*2;
	y = SB_TOP - CG_Text_Height( 0, FONT_SMALL, mySBScale ) - 5;
	
	/*SB_NAME_X = (SB_SCORELINE_X);
	SB_SCORE_X = (SB_SCORELINE_X + .55 * SB_SCORELINE_WIDTH);
	SB_PING_X = (SB_SCORELINE_X + .75 * SB_SCORELINE_WIDTH);
	SB_TIME_X = (SB_SCORELINE_X + .90 * SB_SCORELINE_WIDTH);*/
	
	SB_TIME_X = (SB_SCORELINE_X + .05 * SB_SCORELINE_WIDTH);
	SB_PING_X = (SB_SCORELINE_X + .20 * SB_SCORELINE_WIDTH);
	SB_SCORE_X = (SB_SCORELINE_X + .37 * SB_SCORELINE_WIDTH);
	SB_NAME_X = (SB_SCORELINE_X + .50 * SB_SCORELINE_WIDTH);

	//CG_DrawPic ( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap_R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );

	CG_Text_Paint ( SB_NAME_X, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "NAME"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
	{
		char sWL[100];
		trap_SP_GetStringTextString("MP_INGAME_W_L", sWL,	sizeof(sWL));

		CG_Text_Paint ( SB_SCORE_X - CG_Text_Width(sWL, mySBScale, FONT_SMALL) / 2.0f, y, mySBScale, colorWhite, sWL, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
	else
	{
		CG_Text_Paint ( SB_SCORE_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "SCORE"), mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "SCORE"), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
	CG_Text_Paint ( SB_PING_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "PING"), mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "PING"), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	CG_Text_Paint ( SB_TIME_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "TIME"), mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "TIME"), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if ( cg.numScores > SB_MAXCLIENTS_NORMAL || cgs.gametype == GT_CTF ) {
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	} else {
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 8;
	}
	localClient = qfalse;


	//I guess this should end up being able to display 19 clients at once.
	//In a team game, if there are 9 or more clients on the team not in the lead,
	//we only want to show 10 of the clients on the team in the lead, so that we
	//have room to display the clients in the lead on the losing team.

	//I guess this can be accomplished simply by printing the first teams score with a maxClients
	//value passed in related to how many players are on both teams.
//	if ( cgs.gametype == GT_CTF ) {
	if ( cgs.gametype >= GT_TEAM ) {
		//
		// teamplay scoreboard
		//
		int team1MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);
		int team2MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);
		vec4_t		hcolor;
		
		topBorderSize = 0;
		bottomBorderSize = 4;
		
		//y += lineHeight/2;

		n1 = CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qtrue );
		
		// time
		CG_DrawTeamBackground( SB_SCORELINE_X - 3, y + lineHeight, (SB_PING_X + SB_TIME_X) / 2 - (SB_SCORELINE_X - 3) - 1, (n1 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		// ping
		CG_DrawTeamBackground( (SB_PING_X + SB_TIME_X) / 2 + 1, y - topBorderSize, (SB_PING_X + SB_SCORE_X) / 2 - ((SB_PING_X + SB_TIME_X) / 2 + 1) - 1, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		// score
		CG_DrawTeamBackground( (SB_SCORE_X + SB_PING_X) / 2 + 1, y - topBorderSize, (SB_NAME_X - 9) - ((SB_SCORE_X + SB_PING_X) / 2 + 1) - 1, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		// name
		//CG_DrawTeamBackground( SB_NAME_X - 7, y + lineHeight, (SB_SCORELINE_WIDTH + SB_SCORELINE_X + 7) - (SB_NAME_X - 5) - 1, (n1 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		
		CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qfalse );
		hcolor[0] = 0.66f;
		hcolor[1] = 0.66f;
		hcolor[2] = 0.66f;
		hcolor[3] = 1.0f;
		CG_FillRect( SB_SCORELINE_X - 5, y + lineHeight, SB_SCORELINE_WIDTH + 10, 1, hcolor );

		maxClients -= team1MaxCl;
		y = ((27 - CG_GetTeamCount(TEAM_SPECTATOR, maxClients)) * lineHeight) + BIGCHAR_HEIGHT;
		y -= 11;
		
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse );
		//y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (cgs.clientinfo[i].infoValid) {
				if (i < cgs.privateclients)
					totalPrivate++;
				else
					totalClients++;
			}
		}
		CG_Text_Paint(SB_SCORELINE_X, y+(((float)n1+0.7f)*lineHeight), ((lineHeight == SB_NORMAL_HEIGHT)?(1.0f):(0.75f)), colorWhite, va("Players: %d/%d (Private: %d/%d)", totalClients, cgs.maxclients-cgs.privateclients, totalPrivate, cgs.privateclients),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL);
		
		SB_SCORELINE_X = 320 + (SB_SCORELINE_X / 2);
		/*SB_NAME_X = (SB_SCORELINE_X);
		SB_SCORE_X = (SB_SCORELINE_X + .55 * SB_SCORELINE_WIDTH);
		SB_PING_X = (SB_SCORELINE_X + .75 * SB_SCORELINE_WIDTH);
		SB_TIME_X = (SB_SCORELINE_X + .90 * SB_SCORELINE_WIDTH);*/
		SB_TIME_X = (SB_SCORELINE_X + .05 * SB_SCORELINE_WIDTH);
		SB_PING_X = (SB_SCORELINE_X + .20 * SB_SCORELINE_WIDTH);
		SB_SCORE_X = (SB_SCORELINE_X + .37 * SB_SCORELINE_WIDTH);
		SB_NAME_X = (SB_SCORELINE_X + .50 * SB_SCORELINE_WIDTH);
		
		SB_BOTICON_X = 640 - 32 - SB_BOTICON_X;
		SB_HEAD_X = 640 - 32 - SB_BOTICON_X;
		
		y = SB_TOP - CG_Text_Height( 0, FONT_SMALL, mySBScale ) - 5;
	
		//CG_DrawPic ( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap_R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );
	
		CG_Text_Paint ( SB_NAME_X, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "NAME"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint ( SB_SCORE_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "SCORE"), mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "SCORE"), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint ( SB_PING_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "PING"), mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "PING"), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint ( SB_TIME_X - CG_Text_Width( CG_GetStringEdString("MP_INGAME", "TIME"), mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, CG_GetStringEdString("MP_INGAME", "TIME"), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		
		y = SB_TOP;
		
		n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qtrue );
		
		// time
		CG_DrawTeamBackground( SB_SCORELINE_X - 3, y + lineHeight, (SB_PING_X + SB_TIME_X) / 2 - (SB_SCORELINE_X - 3) - 1, (n2 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		// ping
		CG_DrawTeamBackground( (SB_PING_X + SB_TIME_X) / 2 + 1, y - topBorderSize, (SB_PING_X + SB_SCORE_X) / 2 - ((SB_PING_X + SB_TIME_X) / 2 + 1) - 1, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		// score
		CG_DrawTeamBackground( (SB_SCORE_X + SB_PING_X) / 2 + 1, y - topBorderSize, (SB_NAME_X - 9) - ((SB_SCORE_X + SB_PING_X) / 2 + 1) - 1, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		// name
		//CG_DrawTeamBackground( SB_NAME_X - 7, y + lineHeight, (SB_SCORELINE_WIDTH + SB_SCORELINE_X + 7) - (SB_NAME_X - 5) - 1, (n2 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		
		CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qfalse );
		hcolor[0] = 0.66f;
		hcolor[1] = 0.66f;
		hcolor[2] = 0.66f;
		hcolor[3] = 1.0f;
		CG_FillRect( SB_SCORELINE_X - 5, y + lineHeight, SB_SCORELINE_WIDTH + 10, 1, hcolor );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
/*	} else if ( cgs.gametype >= GT_TEAM ) {
		//
		// teamplay scoreboard
		//
		
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			int team1MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);
			int team2MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);

			if (team1MaxCl > 10 && (team1MaxCl+team2MaxCl) > maxClients)
			{
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if (team1MaxCl < 10)
				{
					team1MaxCl = 10;
				}
			}

			team2MaxCl = (maxClients-team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n1;

			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n2;

			maxClients -= (team1MaxCl+team2MaxCl);
		} else {
			int team1MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);
			int team2MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);

			if (team1MaxCl > 10 && (team1MaxCl+team2MaxCl) > maxClients)
			{
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if (team1MaxCl < 10)
				{
					team1MaxCl = 10;
				}
			}

			team2MaxCl = (maxClients-team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, TEAM_BLUE, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n1;

			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, TEAM_RED, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n2;

			maxClients -= (team1MaxCl+team2MaxCl);
		}
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		*/
	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}
	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.playerCent->currentState.number ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

