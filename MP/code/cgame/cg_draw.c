/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "../ui/ui_shared.h"

//----(SA) added to make it easier to raise/lower our statsubar by only changing one thing
#define STATUSBARHEIGHT 452
//----(SA) end

extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

// NERVE - SMF
void Controls_GetKeyAssignment( char *command, int *twokeys );
char* BindingFromName( const char *cvar );
void Controls_GetConfig( void );
// -NERVE - SMF

////////////////////////
////////////////////////
////// new hud stuff
///////////////////////
///////////////////////

// RtcwPro - HUD names ETpro port

int	numnames = 0;

typedef struct {
	float x;
	float y;
	float dist;
	char name[MAX_NAME_LENGTH + 1];
} etpro_name_t;

etpro_name_t hudnames[128];

void VP_ResetNames(void) {
	numnames = 0;
}
int RealStrLen(char *s){
	int ret = strlen(s);
	for (; *s; s++) {
		if (Q_IsColorString(s))
			ret -= 2;
	}
	return ret < 50 ? ret : 50;
}
void VP_DrawNames(void) {
	int i;
	float fontsize;
	float dist;

	for (i = 0; i < numnames; i++)
	{
		//need to come up with better scaler
		fontsize = cgs.media.charsetShader;
		dist = hudnames[i].dist > 1000.0f ? 1000.0f : hudnames[i].dist;
		dist /= 2000.0f;
		fontsize -= (dist * fontsize);

		CG_DrawStringExt((hudnames[i].x - ((RealStrLen(hudnames[i].name) * 6) >> 1)),
			hudnames[i].y, hudnames[i].name, colorWhite, qfalse, qfalse, 6, 12, 50);
	}
}
void VP_AddName(int x, int y, float dist, int clientNum) {

	if (clientNum == cg.clientNum)
		return;

	hudnames[numnames].x = x;
	hudnames[numnames].y = y;
	hudnames[numnames].dist = dist;
	Q_strncpyz(hudnames[numnames].name, cgs.clientinfo[clientNum].name, sizeof(hudnames[numnames].name));
	numnames++;

}

int CG_Text_Width( const char *text, float scale, int limit ) {
	int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *fnt = &cgDC.Assets.textFont;

	if ( scale <= cg_smallFont.value ) {
		fnt = &cgDC.Assets.smallFont;
	} else if ( scale > cg_bigFont.value ) {
		fnt = &cgDC.Assets.bigFont;
	}

	useScale = scale * fnt->glyphScale;
	out = 0;
	if ( text ) {
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = &fnt->glyphs[*s & 255];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

int CG_Text_Height( const char *text, float scale, int limit ) {
	int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *fnt = &cgDC.Assets.textFont;

	if ( scale <= cg_smallFont.value ) {
		fnt = &cgDC.Assets.smallFont;
	} else if ( scale > cg_bigFont.value ) {
		fnt = &cgDC.Assets.bigFont;
	}

	useScale = scale * fnt->glyphScale;
	max = 0;
	if ( text ) {
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = &fnt->glyphs[*s & 255];
				if ( max < glyph->height ) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void CG_Text_PaintChar( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader ) {
	float w, h;
	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint( float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *fnt = &cgDC.Assets.textFont;

	if ( scale <= cg_smallFont.value ) {
		fnt = &cgDC.Assets.smallFont;
	} else if ( scale > cg_bigFont.value ) {
		fnt = &cgDC.Assets.bigFont;
	}

	useScale = scale * fnt->glyphScale;

	color[3] *= cg_hudAlpha.value;  // (SA) adjust for cg_hudalpha

	if ( text ) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			glyph = &fnt->glyphs[*s & 255];
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar( x + ofs, y - yadj + ofs,
									   glyph->imageWidth,
									   glyph->imageHeight,
									   useScale,
									   glyph->s,
									   glyph->t,
									   glyph->s2,
									   glyph->t2,
									   glyph->glyph );
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar( x, y - yadj,
								   glyph->imageWidth,
								   glyph->imageHeight,
								   useScale,
								   glyph->s,
								   glyph->t,
								   glyph->s2,
								   glyph->t2,
								   glyph->glyph );
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += ( glyph->xSkip * useScale ) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}

// NERVE - SMF - added back in
int CG_DrawFieldWidth(int x, int y, int width, int value, int charWidth, int charHeight)
{
	char num[16], *ptr;
	int  l;
	int  totalwidth = 0;

	if (width < 1)
	{
		return 0;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
	{
		l = width;
	}

	ptr = num;
	while (*ptr && l)
	{
		totalwidth += charWidth;
		ptr++;
		l--;
	}

	return totalwidth;
}

int CG_DrawField( int x, int y, int width, int value, int charWidth, int charHeight, qboolean dodrawpic, qboolean leftAlign ) {
	char num[16], *ptr;
	int l;
	int frame;
	int startx;

	if ( width < 1 ) {
		return 0;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf( num, sizeof( num ), "%i", value );
	l = strlen( num );
	if ( l > width ) {
		l = width;
	}

	// NERVE - SMF
	if ( !leftAlign ) {
		x -= 2 + charWidth * ( l );
	}

	startx = x;

	ptr = num;
	while ( *ptr && l )
	{
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else {
			frame = *ptr - '0';
		}

		if ( dodrawpic ) {
			CG_DrawPic( x,y, charWidth, charHeight, cgs.media.numberShaders[frame] );
		}
		x += charWidth;
		ptr++;
		l--;
	}

	return startx;
}
// -NERVE - SMF

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t refdef;
	refEntity_t ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;     // no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t cm;
	clientInfo_t    *ci;
	float len;
	vec3_t origin;
	vec3_t mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;    // len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->modelInfo->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles );
//	} else if ( cg_drawIcons.integer ) {
//		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team ) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;

	VectorClear( angles );

	cm = cgs.media.redFlagModel;

	// offset the origin y and z to center the flag
	trap_R_ModelBounds( cm, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the flag nearly fills the box
	// assume heads are taller than wide
	len = 0.5 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;    // len / tan( fov/2 )

	angles[YAW] = 60 * sin( cg.time / 2000.0 );;

	CG_Draw3DModel( x, y, w, h,
					team == TEAM_RED ? cgs.media.redFlagModel : cgs.media.blueFlagModel,
					0, origin, angles );
}


/*
==============
CG_DrawKeyModel
==============
*/
void CG_DrawKeyModel( int keynum, float x, float y, float w, float h, int fadetime ) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;

	VectorClear( angles );

	cm = cg_items[keynum].models[0];

	// offset the origin y and z to center the model
	trap_R_ModelBounds( cm, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

//	len = 0.5 * ( maxs[2] - mins[2] );
	len = 0.75 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;    // len / tan( fov/2 )

	angles[YAW] = 30 * sin( cg.time / 2000.0 );;

	CG_Draw3DModel( x, y, w, h, cg_items[keynum].models[0], 0, origin, angles );
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team ) {
	vec4_t hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

#define UPPERRIGHT_X 640  // RtcwPro move this all the way to the right

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char        *s;
	int w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
			cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES  4
static float CG_DrawFPS( float y ) {
	char        *s;
	int w;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char        *s;
	int w;
	int mins, seconds, tens;
	int msec;

	// NERVE - SMF - draw time remaining in multiplayer
	if ( cgs.gametype >= GT_WOLF ) {
		msec = ( cgs.timelimit * 60.f * 1000.f ) - ( cg.time - cgs.levelStartTime );
	} else {
		msec = cg.time - cgs.levelStartTime;
	}
	// -NERVE - SMF

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	if ( msec < 0 ) {
		s = va( "Sudden Death" );
	} else {
		s = va( "%i:%i%i", mins, tens, seconds );
	}

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

int maxCharsBeforeOverlay;

// set in CG_ParseTeamInfo
int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

#define TEAM_OVERLAY_MAXNAME_WIDTH  16
#define TEAM_OVERLAY_MAXLOCATION_WIDTH  20

static float CG_DrawTeamOverlay( float y ) {
	int x, w, h, xx;
	int i, len = 0;
	const char *p;
	vec4_t hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	char lt[2]; // string for latch classtype
	clientInfo_t *ci;
	// NERVE - SMF
	char classType[2] = { 0, 0 };
	char latchType[2] = { 0, 0 };
	int val;
	vec4_t deathcolor, damagecolor;      // JPW NERVE
	float       *pcolor;
	// -NERVE - SMF

	deathcolor[0] = 1;
	deathcolor[1] = 0;
	deathcolor[2] = 0;
	deathcolor[3] = cg_hudAlpha.value;
	damagecolor[0] = 1;
	damagecolor[1] = 1;
	damagecolor[2] = 0;
	damagecolor[3] = cg_hudAlpha.value;
	maxCharsBeforeOverlay = 80;

	if ( !cg_drawTeamOverlay.integer || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) { // Issue 31 hide team overlay for specs
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED &&
		 cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team

	}
	plyrs = 0;

	// max player name width
	pwidth = 0;
	for ( i = 0; i < numSortedTeamPlayers && i <= TEAM_MAXOVERLAY; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			plyrs++;
			len = CG_DrawStrlen( ci->name );
			if ( len > pwidth ) {
				pwidth = len;
			}
		}
	}

	if ( !plyrs ) {
		return y;
	}

	if ( pwidth > TEAM_OVERLAY_MAXNAME_WIDTH ) {
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;
	}


#if 1
	// max location name width
	lwidth = 0;
	if ( cg_drawTeamOverlay.integer > 1 ) {
		for ( i = 0; i < numSortedTeamPlayers && i <= TEAM_MAXOVERLAY; i++ ) {
			ci = cgs.clientinfo + sortedTeamPlayers[i];
			if ( ci->infoValid &&
				 ci->team == cg.snap->ps.persistant[PERS_TEAM] &&
				 CG_ConfigString( CS_LOCATIONS + ci->location ) ) {
				len = CG_DrawStrlen( CG_TranslateString( CG_ConfigString( CS_LOCATIONS + ci->location ) ) );
				if ( len > lwidth ) {
					lwidth = len;
				}
			}
		}
	}
#else
	// max location name width
	lwidth = 0;
	for ( i = 1; i < MAX_LOCATIONS; i++ ) {
		p = CG_TranslateString( CG_ConfigString( CS_LOCATIONS + i ) );
		if ( p && *p ) {
			len = CG_DrawStrlen( p );
			if ( len > lwidth ) {
				lwidth = len;
			}
		}
	}
#endif

	if ( lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH ) {
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;
	}

	if ( cg_drawTeamOverlay.integer > 1 ) {
		w = ( pwidth + lwidth + 3 + 9 ) * TINYCHAR_WIDTH; // JPW NERVE was +4+7
	} else {
		w = ( pwidth + lwidth + 10 ) * TINYCHAR_WIDTH; // JPW NERVE was +4+7

	}
	

	// RTCWPro
	//x = 640 - w - 4; // JPW was -32
	x = cg_teamOverlayX.integer - w - 4;
	y = cg_teamOverlayY.integer;


	h = plyrs * TINYCHAR_HEIGHT;

	// DHM - Nerve :: Set the max characters that can be printed before the left edge
	if ( cg_fixedAspect.integer == 2 && ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) ) {
		maxCharsBeforeOverlay = ( ( ( cgs.glconfig.vidWidth / cgs.screenXScale ) - w - 1 ) / TINYCHAR_WIDTH ) - 1;
	} else {
		maxCharsBeforeOverlay = ( x / TINYCHAR_WIDTH ) - 1;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 0.5f; // was 0.38 instead of 0.5 JPW NERVE
		hcolor[1] = 0.25f;
		hcolor[2] = 0.25f;
		hcolor[3] = 0.25f * cg_hudAlpha.value;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.25f;
		hcolor[1] = 0.25f;
		hcolor[2] = 0.5f;
		hcolor[3] = 0.25f * cg_hudAlpha.value;
	}

	CG_FillRect( x,y,w,h,hcolor );
	VectorSet( hcolor, 0.4f, 0.4f, 0.4f );
	hcolor[3] = cg_hudAlpha.value;
	CG_DrawRect( x - 1, y, w + 2, h + 2, 1, hcolor );


	for ( i = 0; i < numSortedTeamPlayers && i <= TEAM_MAXOVERLAY; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			// RtcwPro - Add * in front or revivable players..
			char *isRevivable = " ";

			// NERVE - SMF
			// determine class type
			val = cg_entities[ ci->clientNum ].currentState.teamNum;
			if ( val == 0 ) {
				classType[0] = 'S';
			} else if ( val == 1 ) {
				classType[0] = 'M';
			} else if ( val == 2 ) {
				classType[0] = 'E';
			} else if ( val == 3 ) {
				classType[0] = 'L';
			} else {
				classType[0] = 'S';
			}

			Com_sprintf( st, sizeof( st ), "%s", CG_TranslateString( classType ) );

			// deteremine latched class type
			val = ci->latchedClass;

			qboolean playerIsSpawning = (ci->powerups & (1 << PW_INVULNERABLE) && ci->health >= 100);

			if (playerIsSpawning)
			{
				latchType[0] = '\0';
			}
			else if (val == 0) {
				latchType[0] = 'S';
			}
			else if (val == 1) {
				latchType[0] = 'M';
			}
			else if (val == 2) {
				latchType[0] = 'E';
			}
			else if (val == 3) {
				latchType[0] = 'L';
			}

			Com_sprintf(lt, sizeof(lt), "%s", CG_TranslateString(latchType));

			// JPW NERVE
			if ( ci->health > 80 ) {
				pcolor = hcolor;
			} else if ( ci->health > 0 ) {
				pcolor = damagecolor;
			} else {
				pcolor = deathcolor;
				// RtcwPro
				if (ci->playerLimbo != 1) {
					isRevivable = "*";
				}
			}
			// jpw

			xx = x + 1; // * TINYCHAR_WIDTH;

			// RTCWPro - display obj carriers
			if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
			{
				CG_DrawPic(xx - 3, y - 3, 15, 15, cgs.media.treasureIcon); // trap_R_RegisterShaderNoMip("models/multiplayer/treasure/treasure"));
			}

			hcolor[0] = hcolor[1] = 1.0;
			hcolor[2] = 0.0;
			hcolor[3] = cg_hudAlpha.value;
			// RtcwPro put IsRevivable in front of class type

			if (!Q_stricmp(st, lt) || cg_teamOverlayLatchedClass.integer == 0 || playerIsSpawning)
				CG_DrawStringExt(xx, y, va("%s%s", isRevivable, st), damagecolor, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 5); // always draw class name and * yellow
			else
			{
				CG_DrawPic(xx + 16, y - 1, 9, 9, trap_R_RegisterShaderNoMip("gfx/2d/arrow.tga"));
				CG_DrawStringExt(xx, y, va("%s%s%s%s", isRevivable, st, " ", lt), damagecolor, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 5); // always draw class name and * yellow
			}

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = cg_hudAlpha.value;
			
			xx = x + 5 * TINYCHAR_WIDTH;
			CG_DrawStringExt( xx + 1, y, ci->name, pcolor, qtrue, qfalse, // RtcwPro moved IsRevivable above
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH );

			if ( lwidth ) {
				p = CG_ConfigString( CS_LOCATIONS + ci->location );
				if ( !p || !*p ) {
					p = "unknown";
				}
				p = CG_TranslateString( p );
//				len = CG_DrawStrlen( p );
//				if ( len > lwidth ) {
//					len = lwidth;
//				}

				xx = x + 20 + TINYCHAR_WIDTH * 5 + TINYCHAR_WIDTH * pwidth +
					 ( ( lwidth / 2 - len / 2 ) * TINYCHAR_WIDTH );
				CG_DrawStringExt( xx, y,
								  p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
								  TEAM_OVERLAY_MAXLOCATION_WIDTH );
			}


			Com_sprintf( st, sizeof( st ), "%3i", ci->health ); // JPW NERVE pulled class stuff since it's at top now

			if ( cg_drawTeamOverlay.integer > 1 ) {
				xx = x + 20 + TINYCHAR_WIDTH * 6 + TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;
			} else {
				xx = x + 20 + TINYCHAR_WIDTH * 4 + TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;
			}

			CG_DrawStringExt( xx, y,
							  st, pcolor, qfalse, qfalse,
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 3 );

			y += TINYCHAR_HEIGHT;
		}
	}

	return y;
}


/**
 * @brief CG_CalculateReinfTime_Float
 * @param[in] menu
 * @return
 */
float CG_CalculateReinfTime_Float(qboolean menu)
{
	team_t team;
	int    dwDeployTime;

	if (menu)
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			team = cgs.ccSelectedTeam == 0 ? TEAM_RED : TEAM_BLUE;
		}
		else
		{
			team = cgs.clientinfo[cg.clientNum].team;
		}
	}
	else
	{
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}

	dwDeployTime = (team == TEAM_RED) ? cg_redlimbotime.integer : cg_bluelimbotime.integer;
	return (1 + (dwDeployTime - ((cgs.aReinfOffset[team] + cg.time - cgs.levelStartTime) % dwDeployTime)) * 0.001f);
}

/**
 * @brief CG_CalculateReinfTime
 * @param menu
 * @return
 */
int CG_CalculateReinfTime(qboolean menu)
{
	return (int)(CG_CalculateReinfTime_Float(menu));
}

int CG_CalculateShoutcasterReinfTime(team_t team)
{
	int dwDeployTime;

	dwDeployTime = (team == TEAM_RED) ? cg_redlimbotime.integer : cg_bluelimbotime.integer;
	return (int)(1 + (dwDeployTime - ((cgs.aReinfOffset[team] + cg.time - cgs.levelStartTime) % dwDeployTime)) * 0.001f);
}

/*
========================
OSPx
Respawn Timer
========================
*/
static float CG_DrawRespawnTimer(float y) {
	char		*str = { 0 };
	int			w;
	float		x;
	vec4_t color;

	playerState_t* ps;

	if (cgs.gametype < GT_WOLF) {
		return y;
	}

	ps = &cg.snap->ps;

	/*if (ps->stats[STAT_HEALTH] <= 0) { // don't show RT when limbo message is drawn
		return y;
	}*/

	// Don't draw timer if client is checking scoreboard
	if (CG_DrawScoreboard()) {
		return y;
	}

	if (cg.showScores) {
		return y;
	}

	if (cgs.clientinfo[cg.clientNum].shoutStatus) {
		return y;
	}

	if (cgs.gamestate != GS_PLAYING)
		str = "";
	else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		str = "";
	else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED)
		str = va("RT: %-2d", CG_CalculateReinfTime(qfalse));
	else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_BLUE)
		str = va("RT: %-2d", CG_CalculateReinfTime(qfalse));

	w = CG_DrawStrlen(str) * TINYCHAR_WIDTH;

//	x = 46 + 6;
	//x = 46 + 40; - supposed
//	y = 480 - 245;
	//y = 480 - 410; - supposed

	x = cg_reinforcementTimeX.integer;
	y = cg_reinforcementTimeY.integer;

	BG_ParseColorCvar(cg_reinforcementTimeColor.string, color, cg_hudAlpha.value);

	if (cgs.gamestate != GS_PLAYING) {
		CG_DrawStringExt((x + 4) - w, y, str, colorYellow, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}
	else if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR){
		CG_DrawStringExt(x - w, y, str, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}
	return y += TINYCHAR_HEIGHT;
}

/*
========================
RTCWPro
Enemy Timer
========================
*/
static float CG_DrawEnemyTimer(float y) {
	char		*str = { 0 };
	int    w;
	int    tens;
	int    x;
	int    secondsThen;
	int    msec    = (cgs.timelimit * 60.f * 1000.f ) - (cg.time - cgs.levelStartTime);
	int    seconds = msec / 1000;
	int    mins    = seconds / 60;
	vec4_t color;
	playerState_t* ps;

	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;
	ps = &cg.snap->ps;

	if (cgs.gametype < GT_WOLF) { 
		return y;
	}

	// Don't draw timer if client is checking scoreboard
	if (CG_DrawScoreboard()) {
		return y;
	}

	if (cg.showScores) {
		return y;
	}

    if (cg_spawnTimer_set.integer == -1)
        return y;

    if (cgs.gamestate == GS_WARMUP || cgs.gamestate == GS_WAITING_FOR_PLAYERS) {
        return y;
    }

	if (cg_spawnTimer_set.integer != -1 && cgs.gamestate == GS_PLAYING && !cgs.clientinfo[cg.clientNum].shoutStatus) { 
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) { 
			int period = cg_spawnTimer_period.integer > 0 ? cg_spawnTimer_period.integer : 
				(cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ? cg_bluelimbotime.integer / 1000 : cg_redlimbotime.integer / 1000);

			if (period > 0) { // prevent division by 0 for weird cases like limbtotime < 1000
				seconds = msec / 1000;
				secondsThen = ((cgs.timelimit * 60000.f) - cg_spawnTimer_set.integer) / 1000;

				str = va("ERT: %-2i", period + (seconds - secondsThen) % period);
				w = CG_DrawStrlen(str) * TINYCHAR_WIDTH;

				//	x = 46 + 6;
				//x = 46 + 40; - supposed
				//	y = 480 - 245;

				x = cg_enemyTimerX.integer;
				y = cg_enemyTimerY.integer;
				BG_ParseColorCvar(cg_enemyTimerColor.string, color, cg_hudAlpha.value);
				CG_DrawStringExt((x + 5) - w, y, str, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
			}
		}
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING) { 
		// We are not playing and the timer is set so reset/disable it
		// this happens for example when custom period is set by timerSet and map is restarted or changed
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	else { 
        return y;
	}

	return y += TINYCHAR_HEIGHT;
}

/*
========================
RTCWPro
CG_DrawShoutcastTimer
========================
*/
static float CG_DrawShoutcastTimer(float y) {
	vec4_t color = { .6f, .6f, .6f, 1.f };
	char* rtAllies = "", * rtAxis = "";
	int h, x, w, reinfTimeAx, reinfTimeAl;

	if (cgs.gamestate != GS_PLAYING)
	{
		return y;
	}

	x = 46 + 30;
	y = 480 - 410;

	reinfTimeAx = CG_CalculateShoutcasterReinfTime(TEAM_RED);
	reinfTimeAl = CG_CalculateShoutcasterReinfTime(TEAM_BLUE);

	rtAllies = va("^$%i", reinfTimeAl);
	rtAxis = va("^1%i", reinfTimeAx);

	h = CG_DrawStrlen(rtAllies) * TINYCHAR_WIDTH;
	w = CG_DrawStrlen(rtAxis) * TINYCHAR_WIDTH;

	color[3] = 1.f;

	// Axis time
	CG_DrawStringExt(x - w - h, y, rtAxis, colorRed, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	// Allies time
	CG_DrawStringExt(x - w - h + 20, y, rtAllies, colorBlue, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	return y += TINYCHAR_HEIGHT;
}

/*
========================
RTCWPro
OSPx
Counts time in

NOTE: Don't try to read this mess..it will break you.
========================
*/
void CG_startCounter(void) {
	char* seconds = ((cg.timein % 60 < 10) ? va("0%d", cg.timein % 60) : va("%d", cg.timein % 60));
	int minutes = cg.timein / 60;
	char* hours = ((minutes / 60 < 10) ? (minutes / 60 ? va("0%d:", minutes / 60) : "") : va("%d:", minutes / 60));
	char* str = va("^nT: ^7%s%s:%s", (hours ? va("%s", hours) : ""), (minutes ? (minutes < 10 ? va("0%d", minutes) : va("%d", minutes)) : "00"), seconds);

	// Don't draw timer if client is checking scoreboard
	if (CG_DrawScoreboard() || !cg.demoPlayback || (cg.demoPlayback && !demo_showTimein.integer))
		return;

	// It is aligned under Respawn timer..
	CG_DrawStringExt(16, 243, str, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH - 3, SMALLCHAR_HEIGHT - 4, 0);
	return;
}

/*
===================
RTCWPro - draw speed
with extra features
Source: PubJ

CG_DrawTJSpeed
===================
*/
void CG_DrawTJSpeed(void) {
	char         status[128];
	float        sizex, sizey, x, y;
	int          w;
	static vec_t speed;
	vec4_t       color;
	static vec_t topSpeed;

	if (cg.resetmaxspeed)
	{
		cg.topSpeed = 0;
		topSpeed = 0;
		cg.resetmaxspeed = qfalse;
	}

	if (!cg_drawSpeed.integer)
	{
		return;
	}

	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	// pp velocity is sometimes NaN, so check it
	if (speed != speed)
	{
		speed = 0;
	}

	if (speed > topSpeed)
	{
		topSpeed = speed;
	}

	sizex = sizey = 0.25f;

	x = cg_speedX.value;
	y = cg_speedY.value;

	switch (cg_drawSpeed.integer)
	{
	case 1:
		Com_sprintf(status, sizeof(status), va("%.0f", speed));
		break;
	case 2:
		Com_sprintf(status, sizeof(status), va("%.0f %.0f", speed, topSpeed));
		break;
	case 3:
		Com_sprintf(status, sizeof(status), va("%.0f", speed));
		break;
	case 4:
		Com_sprintf(status, sizeof(status), va("%.0f %.0f", speed, topSpeed));
		break;
	default:
		Com_sprintf(status, sizeof(status), va("%.0f", speed));
		break;
	}

	w = CG_Text_Width_Ext(status, sizex, sizey, &cgDC.Assets.textFont) / 2;
	BG_ParseColorCvar("white", color, cg_hudAlpha.value);

	if (cg_drawSpeed.integer > 2 && speed > cg.oldSpeed + 0.001f * 100)
	{
		BG_ParseColorCvar("green", color, cg_hudAlpha.value);
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgDC.Assets.textFont);
	}
	else if (cg_drawSpeed.integer > 2 && speed < cg.oldSpeed - 0.001f * 100)
	{
		BG_ParseColorCvar("red", color, cg_hudAlpha.value);
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgDC.Assets.textFont);
	}
	else
	{
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgDC.Assets.textFont);
	}

	cg.oldSpeed = speed;
}

/*
========================
OSPx > rtcwPub > RTCWPro
Respawn Timer
========================
*/
static float CG_DrawProRespawnTimer(float y) {
	int x;
	int	val = 0;
	float scale = 0.8f;
	vec4_t color;

	// Don't draw timer if client is checking scoreboard
	if (CG_DrawScoreboard()) {
		return y;
	}

	if (cg.showScores) {
		return y;
	}

	if (cgs.gamestate != GS_PLAYING) {
		return y;
	}

	if (cgs.clientinfo[cg.clientNum].shoutStatus) {
		return y;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED) {
		val = CG_CalculateReinfTime(qfalse);
	}
	else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_BLUE) {
		val = CG_CalculateReinfTime(qfalse);
	}

	BG_ParseColorCvar(cg_reinforcementTimeColor.string, color, cg_hudAlpha.value);
	trap_R_SetColor(color);

	x = cg_reinforcementTimeProX.integer;
	y = cg_reinforcementTimeProY.integer;

	CG_DrawField(x, y, 3, val, 20 * scale, 32 * scale, qtrue, qfalse);

	return y;
}

/*
========================
RTCWPro
Enemy Timer
========================
*/
static float CG_DrawProEnemyTimer(float y) {
	int    tens;
	int    x = 290;
	int    secondsThen;
	int    msec = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);
	int    seconds = msec / 1000;
	int    mins = seconds / 60;
	int val = 0;
	float scale = 0.8f;
	vec4_t color;
	playerState_t* ps;

	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;
	ps = &cg.snap->ps;

	if (cgs.gametype < GT_WOLF) {
		return y;
	}

	// Don't draw timer if client is checking scoreboard
	if (CG_DrawScoreboard()) {
		return y;
	}

	if (cg.showScores) {
		return y;
	}

	if (cg_spawnTimer_set.integer == -1)
		return y;

	if (cgs.gamestate == GS_WARMUP || cgs.gamestate == GS_WAITING_FOR_PLAYERS) {
		return y;
	}

	if (cg_spawnTimer_set.integer != -1 && cgs.gamestate == GS_PLAYING && !cgs.clientinfo[cg.clientNum].shoutStatus) {
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) {
			int period = cg_spawnTimer_period.integer > 0 ? cg_spawnTimer_period.integer :
				(cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ? cg_bluelimbotime.integer / 1000 : cg_redlimbotime.integer / 1000);

			if (period > 0) { // prevent division by 0 for weird cases like limbtotime < 1000
				seconds = msec / 1000;
				secondsThen = ((cgs.timelimit * 60000.f) - cg_spawnTimer_set.integer) / 1000;
				val = (period + (seconds - secondsThen) % period);

				BG_ParseColorCvar(cg_enemyTimerColor.string, color, cg_hudAlpha.value);
				trap_R_SetColor(color);

				x = cg_enemyTimerProX.integer;
				y = cg_enemyTimerProY.integer;

				CG_DrawField(x, y, 3, val, 20 * scale, 32 * scale, qtrue, qfalse);
			}
		}
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING) {
		// We are not playing and the timer is set so reset/disable it
		// this happens for example when custom period is set by timerSet and map is restarted or changed
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	else {
		return y;
	}

	return y += TINYCHAR_HEIGHT;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame) {
	float y;
	const char* info;
	char* allowErt;

	y = 0; // JPW NERVE move team overlay below obits, even with timer on left

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);
	}

	if ( cgs.gametype >= GT_TEAM ) {
		// RTCWPro - don't offset if it's not in its default position, since it's customizable
		if (cg_teamOverlayY.integer == 0 && cg_teamOverlayX.integer == 640) {
			y = CG_DrawTeamOverlay(y);
		}
		else {
			CG_DrawTeamOverlay(0);
		}
	}
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer && cgs.gamestate == GS_PLAYING) {
		y = CG_DrawTimer( y );
	}
// (SA) disabling drawattacker for the time being
//	if ( cg_drawAttacker.integer ) {
//		y = CG_DrawAttacker( y );
//	}
//----(SA)	end

	// OSPx - Respawn Timer
	if (cg_drawReinforcementTime.integer == 1) {
		y = CG_DrawRespawnTimer(y);
	}

	// RTCWPro - different style RT
	if (cg_drawReinforcementTime.integer == 2) {
		y = CG_DrawProRespawnTimer(y);
	}

	if (cg_drawReinforcementTime.integer > 2) {
		y = CG_DrawRespawnTimer(y);
		y = CG_DrawProRespawnTimer(y);
	}

	// enemy respawn timer
	info = CG_ConfigString(CS_SERVERINFO);
	allowErt = Info_ValueForKey(info, "g_allowEnemySpawnTimer");

	if (allowErt != NULL && !Q_stricmp(allowErt, "1"))
	{
		if ((cg_spawnTimer_set.integer != -1) && (cg_spawnTimer_period.integer > 0)) {
			if (cg_drawEnemyTimer.integer == 1) {
				y = CG_DrawEnemyTimer(y);
			}
			if (cg_drawEnemyTimer.integer == 2) {
				y = CG_DrawProEnemyTimer(y);
			}
			if (cg_drawEnemyTimer.integer > 2) {
				y = CG_DrawEnemyTimer(y);
				y = CG_DrawProEnemyTimer(y);
			}
		}
	}

	if (cgs.clientinfo[cg.clientNum].shoutStatus && cg_drawReinforcementTime.integer > 0) {
		y = CG_DrawShoutcastTimer(y);
	}

	// RTCWPro - complete OSP demo features
	// OSPx - Time Counter
	CG_startCounter();
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo( void ) {
	int w, h;
	int i, len;
	vec4_t hcolor;
	int chatHeight;
	float alphapercent;
	float chatAlpha = (float)cg_chatAlpha.value;
	// RTCWPro
	int x = cg_chatX.integer;
	int y = cg_chatY.integer;

	#define CHATLOC_Y 385 // bottom end
	#define CHATLOC_X 0

	if ( cg_teamChatHeight.integer < TEAMCHAT_HEIGHT ) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}
	if ( chatHeight <= 0 ) {
		return; // disabled
	}

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );
	}

	if ( cgs.teamLastChatPos != cgs.teamChatPos ) {
		if ( cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer ) {
			cgs.teamLastChatPos++;
		}

		h = ( cgs.teamChatPos - cgs.teamLastChatPos ) * TINYCHAR_HEIGHT;

		w = 0;

		for ( i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++ ) {
			len = CG_DrawStrlen( cgs.teamChatMsgs[i % chatHeight] );
			if ( len > w ) {
				w = len;
			}
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

// JPW NERVE rewritten to support first pass at fading chat messages
		for ( i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i-- ) {
			alphapercent = 1.0f - ( cg.time - cgs.teamChatMsgTimes[i % chatHeight] ) / (float)( cg_teamChatTime.integer );
			if ( alphapercent > 1.0f ) {
				alphapercent = 1.0f;
			} else if ( alphapercent < 0.f ) {
				alphapercent = 0.f;
			}

			if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED ) {
				hcolor[0] = 1;
				hcolor[1] = 0;
				hcolor[2] = 0;
			} else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE ) {
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 1;
			} else {
				hcolor[0] = 0;
				hcolor[1] = 1;
				hcolor[2] = 0;
			}
// RtcwPro
			if (chatAlpha > 1.0f) {
				chatAlpha = 1.0f;
			}
			else if (chatAlpha < 0.f) {
				chatAlpha = 0.f;
			}

			if (!Q_stricmp(cg_chatBackgroundColor.string, ""))
				hcolor[3] = chatAlpha * alphapercent;
			else // Abuse this..
				BG_setCrosshair(cg_chatBackgroundColor.string, hcolor, chatAlpha * alphapercent, "cg_chatBackgroundColor");
// End
			trap_R_SetColor( hcolor );
			if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
				CG_DrawPic( CHATLOC_X, CHATLOC_Y - ( cgs.teamChatPos - i ) * TINYCHAR_HEIGHT, cgs.glconfig.vidWidth, TINYCHAR_HEIGHT, cgs.media.teamStatusBar );
			} else {
				CG_DrawPic( CHATLOC_X, CHATLOC_Y - ( cgs.teamChatPos - i ) * TINYCHAR_HEIGHT, 640, TINYCHAR_HEIGHT, cgs.media.teamStatusBar );
			}
			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor( hcolor );

			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH,
							  CHATLOC_Y - ( cgs.teamChatPos - i ) * TINYCHAR_HEIGHT,
							  cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}

//----(SA)	modified
/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int value;
	float   *fadeColor;
	char pickupText[256];
	float color[4];
	const char *s;

	if (cg_gameType.integer != GT_SINGLE_PLAYER)
		cg_gameSkill.integer = 3; // RTCWPro if a player has g_gameskill set in wolfconfig_mp it could show "50 health" so set this to 3 to get default value of "20 health"

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
	}

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );

			//----(SA)	so we don't pick up all sorts of items and have it print "0 <itemname>"
			if ( bg_itemlist[ value ].giType == IT_AMMO || bg_itemlist[ value ].giType == IT_HEALTH || bg_itemlist[value].giType == IT_POWERUP ) {
				if ( bg_itemlist[ value ].world_model[2] ) {   // this is a multi-stage item
					// FIXME: print the correct amount for multi-stage
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", bg_itemlist[ value ].pickup_name );
				} else {
					Com_sprintf( pickupText, sizeof( pickupText ), "%i  %s", bg_itemlist[ value ].gameskillnumber[( cg_gameSkill.integer ) - 1], bg_itemlist[ value ].pickup_name );
				}
			} else {
				if ( !Q_stricmp( "Blue Flag", bg_itemlist[ value ].pickup_name ) ) {
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", "Objective" );
				} else {
					Com_sprintf( pickupText, sizeof( pickupText ), "%s", bg_itemlist[ value ].pickup_name );
				}
			}

			s = CG_TranslateString( pickupText );

			color[0] = color[1] = color[2] = 1.0;
			color[3] = fadeColor[0];
			CG_DrawStringExt2( 34, 388, s, color, qfalse, qtrue, 10, 10, 0 ); // JPW NERVE moved per atvi req

			trap_R_SetColor( NULL );
		}
	}
}
//----(SA)	end

/*
=================
CG_DrawNotify
=================
*/
#define NOTIFYLOC_Y 42 // bottom end
#define NOTIFYLOC_X 0

static void CG_DrawNotify( void ) {
	int w, h, x, y, i, len, width, height;
	vec4_t hcolor;
	int chatHeight;
	float alphapercent;
	char var[MAX_TOKEN_CHARS];
	float notifytime = 1.0f;
	qboolean shadow;

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

	// RTCWPro
	x = cg_notifyTextX.integer;
	y = cg_notifyTextY.integer;
	shadow = cg_notifyTextShadow.integer;
	width = cg_notifyTextWidth.integer;
	height = cg_notifyTextHeight.integer;

	trap_Cvar_VariableStringBuffer( "con_notifytime", var, sizeof( var ) );
	notifytime = atof( var ) * 1000;

	if ( notifytime <= 100.f ) {
		notifytime = 100.0f;
	}

	chatHeight = NOTIFY_HEIGHT;

	if ( cgs.notifyLastPos != cgs.notifyPos ) {
		if ( cg.time - cgs.notifyMsgTimes[cgs.notifyLastPos % chatHeight] > notifytime ) {
			cgs.notifyLastPos++;
		}

		h = ( cgs.notifyPos - cgs.notifyLastPos ) * TINYCHAR_HEIGHT;

		w = 0;

		for ( i = cgs.notifyLastPos; i < cgs.notifyPos; i++ ) {
			len = CG_DrawStrlen( cgs.notifyMsgs[i % chatHeight] );
			if ( len > w ) {
				w = len;
			}
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if ( maxCharsBeforeOverlay <= 0 ) {
			maxCharsBeforeOverlay = 80;
		}

		for ( i = cgs.notifyPos - 1; i >= cgs.notifyLastPos; i-- ) {
			alphapercent = 1.0f - ( ( cg.time - cgs.notifyMsgTimes[i % chatHeight] ) / notifytime );
			if ( alphapercent > 0.5f ) {
				alphapercent = 1.0f;
			} else {
				alphapercent *= 2;
			}

			if ( alphapercent < 0.f ) {
				alphapercent = 0.f;
			}

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor( hcolor );

			CG_DrawStringExt(x + TINYCHAR_WIDTH, y - (cgs.notifyPos - i) * TINYCHAR_HEIGHT, cgs.notifyMsgs[i % chatHeight],
				hcolor, qfalse, shadow, width, height, maxCharsBeforeOverlay);
				
			/*CG_DrawStringExt( NOTIFYLOC_X + TINYCHAR_WIDTH,
							  NOTIFYLOC_Y - ( cgs.notifyPos - i ) * TINYCHAR_HEIGHT,
							  cgs.notifyMsgs[i % chatHeight], hcolor, qfalse, qfalse,
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, maxCharsBeforeOverlay );*/
		}
	}
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES     128


typedef struct {
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float x, y;
	int cmdNum;
	usercmd_t cmd;
	const char      *s;
	int w;

	// OSPx - Fix for "connection interrupted" when user is previewing demo with timescale lower than 0.5...
	if (cg.demoPlayback && cg_timescale.value != 1.0f) {
		return;
	}

	// ydnar: don't draw if the server is respawning
	if (cg.serverRespawning) {
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		 || cmd.serverTime > cg.time ) { // special check for map_restart
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// also add text in center of screen
	s = CG_TranslateString( "CI" );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	
	//320 - w / 2, 80
	if (cg_lagometer.integer)
		CG_DrawBigString(cg_lagometerX.integer + 10, cg_lagometerY.integer, s, 1.0F );

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	x = 640 - 52;
	y = 480 - 140;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/2d/net.tga" ) );
}


#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	// RTCWPro
	int a, x = cg_lagometerX.integer, y = cg_lagometerY.integer, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;

	if (cg.demoPlayback) {
		return;
	}

	if ( !cg_lagometer.integer || cgs.localServer ) {
//	if(0) {
		CG_DrawDisconnect();
		return;
	}

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
	}

	//
	// draw the graph
	//
	// RTCWPro
	//x = 640 - 55;
	//y = 480 - 140;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// rtcwpro - speed
	if (cg_lagometer.integer > 1)
	{
		static vec_t speed;
		float vscale2, range2, v2;
		vec4_t color2;

		BG_ParseColorCvar("ltgrey", color2, 0.8);

		speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] +
			cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

		if (speed != speed)
		{
			speed = 0;
		}

		range2 = ah;
		vscale2 = range2 / 2048;

		for (a = 0; a < aw; a++)
		{
			v2 = speed;

			if (v2 > 0)
			{
				trap_R_SetColor(color2);

				v2 = v2 * vscale2;

				if (v2 > range2)
				{
					v2 = range2;
				}

				trap_R_DrawStretchPic(ax + aw - a, ay + ah - v2, 1, v2, 0, 0, 0, 0, cgs.media.whiteShader);
			}
		}
	}

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_BLUE )] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_GREEN )] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_RED )] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
#define CP_LINEWIDTH 55         // NERVE - SMF

void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char    *s;
	int i, len;                         // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF
	int priority = 0;

	// NERVE - SMF - don't draw if this print message is less important
	if ( cg.centerPrintTime && priority < cg.centerPrintPriority ) {
		return;
	}

	Q_strncpyz( cg.centerPrint, str, sizeof( cg.centerPrint ) );
	cg.centerPrintPriority = priority;  // NERVE - SMF

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen( cg.centerPrint );
	for ( i = 0; i < len; i++ ) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if ( i % ( CP_LINEWIDTH - 20 ) == 0 && i > 0 ) {
			neednewline = qtrue;
		}
		if ( cg.centerPrint[i] == ' ' && neednewline ) {
			cg.centerPrint[i] = '\n';
			neednewline = qfalse;
		}
	}
	// -NERVE - SMF

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.centerPrintLines++;
		}
		s++;
	}
}

// NERVE - SMF
/*
==============
CG_PriorityCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_PriorityCenterPrint( const char *str, int y, int charWidth, int priority ) {
	char    *s;
	int i, len;                         // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF

	// NERVE - SMF - don't draw if this print message is less important
	if ( cg.centerPrintTime && priority < cg.centerPrintPriority ) {
		return;
	}

	Q_strncpyz( cg.centerPrint, str, sizeof( cg.centerPrint ) );
	cg.centerPrintPriority = priority;  // NERVE - SMF

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen( cg.centerPrint );
	for ( i = 0; i < len; i++ ) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if ( i % ( CP_LINEWIDTH - 20 ) == 0 && i > 0 ) {
			neednewline = qtrue;
		}
		if ( cg.centerPrint[i] == ' ' && neednewline ) {
			cg.centerPrint[i] = '\n';
			neednewline = qfalse;
		}
	}
	// -NERVE - SMF

	cg.centerPrintTime = cg.time + 2000;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.centerPrintLines++;
		}
		s++;
	}
}
// -NERVE - SMF

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char    *start;
	int l;
	int x, y, w;
	float   *color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		cg.centerPrintTime = 0;
		cg.centerPrintPriority = 0;
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < CP_LINEWIDTH; l++ ) {          // NERVE - SMF - added CP_LINEWIDTH
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
						  cg.centerPrintCharWidth, (int)( cg.centerPrintCharWidth * 1.5 ), 0 );

		y += cg.centerPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}


/*
===================
RtcwPro - Cg_PopinPrint

Pops in messages
===================
*/
#define CP_PMWIDTH 84
void CG_PopinPrint(const char *str, int charWidth, qboolean blink) {
	char    *s;
	int i, len;                         // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF

	int x = cg_priorityTextX.integer;
	int y = cg_priorityTextY.integer;

	Q_strncpyz(cg.popinPrint, str, sizeof(cg.popinPrint));

	// Turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.popinPrint);
	for (i = 0; i < len; i++) {

		// Subtract a few chars here so long words still get displayed properly
		if (i % (CP_PMWIDTH - 20) == 0 && i > 0) {
			neednewline = qtrue;
		}
		if (cg.popinPrint[i] == ' ' && neednewline) {
			cg.popinPrint[i] = '\n';
			neednewline = qfalse;
		}
	}
	// -NERVE - SMF

	cg.popinPrintTime = cg.time;
	cg.popinPrintX = x;
	cg.popinPrintY = y;
	cg.popinPrintCharWidth = charWidth;
	cg.popinBlink = blink;

	// count the number of lines for centering
	cg.popinPrintLines = 1;
	s = cg.popinPrint;
	while (*s) {
		if (*s == '\n') {
			cg.popinPrintLines++;
		}
		s++;
	}
}

/*
===================
RtcwPro - CG_DrawPopinString
===================
*/
static void CG_DrawPopinString(void) {
	char    *start;
	int l, x, y;
	float   *color;

	if (!cg.popinPrintTime) {
		return;
	}

	/*int x = cg_priorityTextX.integer;
	int y = cg_priorityTextY.integer;*/

	color = CG_FadeColor(cg.popinPrintTime, 1000 * 7);
	if (!color) {
		cg.popinPrintTime = 0;
		return;
	}

	trap_R_SetColor(color);
	start = cg.popinPrint;

	y = (cg.popinPrintY - 7) - cg.popinPrintLines * TINYCHAR_HEIGHT / 2;
	x = cg.popinPrintX + (cg.popinPrintLines * TINYCHAR_HEIGHT / 2);

	if (cg.popinBlink)
		color[3] = Q_fabs(sin(cg.time * 0.001)) * cg_hudAlpha.value;

	while (1) {
		char linebuffer[1024];

		for (l = 0; l < CP_PMWIDTH; l++) {          // NERVE - SMF - added CP_LINEWIDTH
			if (!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
			CG_DrawStringExt(x, y, linebuffer, color, qfalse, qfalse,
			TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		else
			CG_DrawStringExt(x, y, linebuffer, color, qfalse, qfalse,
			TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

		y += cg.popinPrintCharWidth * 1.5;

		while (*start && (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}
	trap_R_SetColor(NULL);
}

/*
================================================================================

CROSSHAIRS

================================================================================
*/


/*
==============
CG_DrawWeapReticle
==============
*/
static void CG_DrawWeapReticle( void ) {
	qboolean snooper, sniper;
	vec4_t color = {0, 0, 0, 1};
	float mask = 0, lb = 0;

	// DHM - Nerve :: So that we will draw reticle
	if ( cgs.gametype >= GT_WOLF && ( ( cg.snap->ps.pm_flags & PMF_FOLLOW ) || cg.demoPlayback ) ) {
		sniper = (qboolean)( cg.snap->ps.weapon == WP_SNIPERRIFLE );
		snooper = (qboolean)( cg.snap->ps.weapon == WP_SNOOPERSCOPE );
	} else {
		sniper = (qboolean)( cg.weaponSelect == WP_SNIPERRIFLE );
		snooper = (qboolean)( cg.weaponSelect == WP_SNOOPERSCOPE );
	}

	if ( sniper ) {
		if ( cg_reticles.integer ) {

			// sides
			if ( cg_fixedAspect.integer && !cg.limboMenu ) {
				if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
					mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

					CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
					CG_FillRect( 0, 0, mask, 480, color );
					CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
					CG_FillRect( 640 - mask, 0, mask, 480, color );
				} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
					// sides with letterbox
					lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

					CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
					CG_FillRect( 0, 0, 80, 480, color );
					CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
					CG_FillRect( 560, 0, 80, 480, color );

					CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
					CG_FillRect( 0, 480 - lb, 640, lb, color );
					CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
					CG_FillRect( 0, 0, 640, lb, color );
				} else {
					// resolution is 4:3
					CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
					CG_FillRect( 0, 0, 80, 480, color );
					CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
					CG_FillRect( 560, 0, 80, 480, color );
				}
			} else {
				CG_FillRect( 0, 0, 80, 480, color );
				CG_FillRect( 560, 0, 80, 480, color );
			}

			// center
			if ( cg_fixedAspect.integer ) {
				CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
			}

			if ( cgs.media.reticleShaderSimple ) {
				CG_DrawPic( 80, 0, 480, 480, cgs.media.reticleShaderSimple );
			}

			// hairs
			CG_FillRect( 84, 239, 177, 2, color );   // left
			CG_FillRect( 320, 242, 1, 58, color );   // center top
			CG_FillRect( 319, 300, 2, 178, color );  // center bot
			CG_FillRect( 380, 239, 177, 2, color );  // right
		}
	} else if ( snooper )     {
		if ( cg_reticles.integer ) {

			// sides
			if ( cg_fixedAspect.integer && !cg.limboMenu ) {
				if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
					mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 480.0 ) ) / cgs.screenXScale );

					CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
					CG_FillRect( 0, 0, mask, 480, color );
					CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
					CG_FillRect( 640 - mask, 0, mask, 480, color );
				} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
					// sides with letterbox
					lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

					CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
					CG_FillRect( 0, 0, 80, 480, color );
					CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
					CG_FillRect( 560, 0, 80, 480, color );

					CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
					CG_FillRect( 0, 480 - lb, 640, lb, color );
					CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
					CG_FillRect( 0, 0, 640, lb, color );
				} else {
					// resolution is 4:3
					CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
					CG_FillRect( 0, 0, 80, 480, color );
					CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
					CG_FillRect( 560, 0, 80, 480, color );
				}
			} else {
				CG_FillRect( 0, 0, 80, 480, color );
				CG_FillRect( 560, 0, 80, 480, color );
			}

			// center
			if ( cg_fixedAspect.integer ) {
				CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
			}

//----(SA)	added
			// DM didn't like how bright it gets
			{
				vec4_t hcolor = {0.7, .8, 0.7, 0}; // greenish
				float brt;

				brt = cg_reticleBrightness.value;
				Com_Clamp( 0.0f, 1.0f, brt );
				hcolor[0] *= brt;
				hcolor[1] *= brt;
				hcolor[2] *= brt;
				trap_R_SetColor( hcolor );
			}
//----(SA)	end

			if ( cgs.media.snooperShaderSimple ) {
				CG_DrawPic( 80, 0, 480, 480, cgs.media.snooperShaderSimple );
			}

			// hairs

			CG_FillRect( 310, 120, 20, 1, color );   //					-----
			CG_FillRect( 300, 160, 40, 1, color );   //				-------------
			CG_FillRect( 310, 200, 20, 1, color );   //					-----

			CG_FillRect( 140, 239, 360, 2, color );  // horiz ---------------------------

			CG_FillRect( 310, 280, 20, 1, color );   //					-----
			CG_FillRect( 300, 320, 40, 1, color );   //				-------------
			CG_FillRect( 310, 360, 20, 1, color );   //					-----



			CG_FillRect( 400, 220, 1, 40, color );   // l

			CG_FillRect( 319, 60, 2, 360, color );   // vert

			CG_FillRect( 240, 220, 1, 40, color );   // r
		}
	}
}

/*
==============
CG_DrawBinocReticle
==============
*/
static void CG_DrawBinocReticle( void ) {
	vec4_t color = {0, 0, 0, 1};
	float mask = 0, lb = 0;

	if ( cg_fixedAspect.integer ) {
		if ( cgs.glconfig.vidWidth * 480.0 > cgs.glconfig.vidHeight * 640.0 ) {
			mask = 0.5 * ( ( cgs.glconfig.vidWidth - ( cgs.screenXScale * 640.0 ) ) / cgs.screenXScale );

			CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
			CG_FillRect( 0, 0, mask, 480, color );
			CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
			CG_FillRect( 640 - mask, 0, mask, 480, color );
		} else if ( cgs.glconfig.vidWidth * 480.0 < cgs.glconfig.vidHeight * 640.0 ) {
			lb = 0.5 * ( ( cgs.glconfig.vidHeight - ( cgs.screenYScale * 480.0 ) ) / cgs.screenYScale );

			CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
			CG_FillRect( 0, 480 - lb, 640, lb, color );
			CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
			CG_FillRect( 0, 0, 640, lb, color );
		}
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	if ( cg_reticles.integer ) {
		if ( cg_reticleType.integer == 0 ) {
			if ( cgs.media.binocShader ) {
				CG_DrawPic( 0, 0, 640, 480, cgs.media.binocShader );
			}
		} else if ( cg_reticleType.integer == 1 ) {
			// an alternative.  This gives nice sharp lines at the expense of a few extra polys

			if ( cgs.media.binocShaderSimple ) {
				CG_DrawPic( 0, 0, 640, 480, cgs.media.binocShaderSimple );
			}

			CG_FillRect( 146, 239, 348, 1, color );

			CG_FillRect( 188, 234, 1, 13, color );   // ll
			CG_FillRect( 234, 226, 1, 29, color );   // l
			CG_FillRect( 274, 234, 1, 13, color );   // lr
			CG_FillRect( 320, 213, 1, 55, color );   // center
			CG_FillRect( 360, 234, 1, 13, color );   // rl
			CG_FillRect( 406, 226, 1, 29, color );   // r
			CG_FillRect( 452, 234, 1, 13, color );   // rr
		}
	}
}

void CG_FinishWeaponChange( int lastweap, int newweap ); // JPW NERVE


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( void ) {
	float w, h;
	qhandle_t hShader;
	float f;
	float x, y;
	int weapnum;                // DHM - Nerve
	vec4_t hcolor = {1, 1, 1, 1};

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// DHM - Nerve :: show reticle in limbo and spectator
	if ( cgs.gametype >= GT_WOLF && ( ( cg.snap->ps.pm_flags & PMF_FOLLOW ) || cg.demoPlayback ) ) {
		weapnum = cg.snap->ps.weapon;
	} else {
		weapnum = cg.weaponSelect;
	}

	switch ( weapnum ) {

		// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
		if ( cg.zoomedBinoc ) {
			CG_DrawBinocReticle();
		}

		if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			return;
		}
		break;

		// special reticle for weapon
	case WP_SNIPERRIFLE:
		if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {

			// JPW NERVE -- don't let players run with rifles -- speed 80 == crouch, 128 == walk, 256 == run
			if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
				if ( VectorLength( cg.snap->ps.velocity ) > 127.0f ) {
					if ( cg.snap->ps.weapon == WP_SNIPERRIFLE ) {
						CG_FinishWeaponChange( WP_SNIPERRIFLE, WP_MAUSER );
					}
					if ( cg.snap->ps.weapon == WP_SNOOPERSCOPE ) {
						CG_FinishWeaponChange( WP_SNOOPERSCOPE, WP_GARAND );
					}
				}
			}
			// jpw

			CG_DrawWeapReticle();
			return;
		}
		break;
	default:
		break;
	}

	// using binoculars
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}

	if ( cg_drawCrosshair.integer < 0 ) {	//----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if ( cg.snap->ps.leanf ) {
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		// RtcwPro - Crosshair (patched)
		trap_R_SetColor(cg.xhairColor);
	}

	w = h = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	// RtcwPro - Patched for Solid crosshair
	f = (float)((cg_crosshairPulse.integer == 0) ? 0 : cg.snap->ps.aimSpreadScale / 255.0);
	w *= ( 1 + f * 2.0 );
	h *= ( 1 + f * 2.0 );

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	if ( !cg_fixedAspect.integer ) {
		CG_AdjustFrom640( &x, &y, &w, &h );
	}

	hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];

	// NERVE - SMF - modified, fixes crosshair offset in shifted/scaled 3d views
	if ( cg.limboMenu ) { // JPW NERVE
		if ( cg_fixedAspect.integer ) {
			CG_DrawPic( ( ( SCREEN_WIDTH - w ) * 0.5f ) + x, ( ( SCREEN_HEIGHT - h ) * 0.5f ) + y, w, h, hShader );
		} else {
			trap_R_DrawStretchPic( x /*+ cg.refdef.x*/ + 0.5 * ( cg.refdef.width - w ),
								   y /*+ cg.refdef.y*/ + 0.5 * ( cg.refdef.height - h ),
								   w, h, 0, 0, 1, 1, hShader );
		}
	} else {
		if ( cg_fixedAspect.integer ) {
			CG_DrawPic( ( ( SCREEN_WIDTH - w ) * 0.5f ) + x, ( ( SCREEN_HEIGHT - h ) * 0.5f ) + y, w, h, hShader );
		} else {
			trap_R_DrawStretchPic( x + 0.5 * ( cgs.glconfig.vidWidth - w ), // JPW NERVE for scaled-down main windows
								   y + 0.5 * ( cgs.glconfig.vidHeight - h ),
								   w, h, 0, 0, 1, 1, hShader );
		}
	}
	// NERVE - SMF
	if (cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]) {
		w = h = cg_crosshairSize.value;
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
		if (!cg_fixedAspect.integer) {
			CG_AdjustFrom640(&x, &y, &w, &h);

			// RtcwPro - Crosshairs
			if (!cg_crosshairHealth.integer) {
				trap_R_SetColor(cg.xhairColorAlt);
			}

			if (cg.limboMenu) { // JPW NERVE
				if (cg_fixedAspect.integer) {
					CG_DrawPic(((SCREEN_WIDTH - w) * 0.5f) + x, ((SCREEN_HEIGHT - h) * 0.5f) + y, w, h, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
				}
				else {
					trap_R_DrawStretchPic(x + 0.5 * (cg.refdef.width - w), y + 0.5 * (cg.refdef.height - h),
						w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
				}
			}
			else {
				if (cg_fixedAspect.integer) {
					CG_DrawPic(((SCREEN_WIDTH - w) * 0.5f) + x, ((SCREEN_HEIGHT - h) * 0.5f) + y, w, h, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
				}
				else {
					trap_R_DrawStretchPic(x + 0.5 * (cgs.glconfig.vidWidth - w), y + 0.5 * (cgs.glconfig.vidHeight - h), // JPW NERVE fix for small main windows (dunno why people still do this, but it's supported)
						w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
				}
			}
		}
		// -NERVE - SMF

		trap_R_SetColor(NULL);
	}
}


/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D( void ) {
	float w;
	qhandle_t hShader;
	float f;
	int weapnum;                // DHM - Nerve
	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// DHM - Nerve :: show reticle in limbo and spectator
	if ( cgs.gametype >= GT_WOLF && ( ( cg.snap->ps.pm_flags & PMF_FOLLOW ) || cg.demoPlayback ) ) {
		weapnum = cg.snap->ps.weapon;
	} else {
		weapnum = cg.weaponSelect;
	}

	switch ( weapnum ) {

		// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
		if ( cg.zoomedBinoc ) {
			CG_DrawBinocReticle();
		}

		if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			return;
		}
		break;

		// special reticle for weapon
	case WP_SNIPERRIFLE:
		if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {

			// JPW NERVE -- don't let players run with rifles -- speed 80 == crouch, 128 == walk, 256 == run
			if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
				if ( VectorLength( cg.snap->ps.velocity ) > 127.0f ) {
					if ( cg.snap->ps.weapon == WP_SNIPERRIFLE ) {
						CG_FinishWeaponChange( WP_SNIPERRIFLE, WP_MAUSER );
					}
					if ( cg.snap->ps.weapon == WP_SNOOPERSCOPE ) {
						CG_FinishWeaponChange( WP_SNOOPERSCOPE, WP_GARAND );
					}
				}
			}
			// jpw

			CG_DrawWeapReticle();
			return;
		}
		break;
	default:
		break;
	}

	// using binoculars
	if ( cg.zoomedBinoc ) {
		CG_DrawBinocReticle();
		return;
	}

	if ( cg_drawCrosshair.integer < 0 ) {	//----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if ( cg.snap->ps.leanf ) {
		return;
	}

	w = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	f = (float)cg.snap->ps.aimSpreadScale / 255.0;
	w *= ( 1 + f * 2.0 );

	hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	
	VectorCopy(trace.endpos, ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	ent.shaderRGBA[0]=255;
	ent.shaderRGBA[1]=255;
	ent.shaderRGBA[2]=255;
	ent.shaderRGBA[3]=255;

	trap_R_AddRefEntityToScene(&ent);

	if ( cg.crosshairShaderAlt[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ] ) {
		ent.customShader = cg.crosshairShaderAlt[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];

		ent.shaderRGBA[0]=255;
		ent.shaderRGBA[1]=255;
		ent.shaderRGBA[2]=255;
		ent.shaderRGBA[3]=255;

		trap_R_AddRefEntityToScene(&ent);
	}
}


/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t trace;
//	gentity_t	*traceEnt;
	vec3_t start, end;
	int content;

	// DHM - Nerve :: We want this in multiplayer
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		return; //----(SA)	don't use any scanning at the moment.

	}
	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 8192, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
			  cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM );

	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

//	traceEnt = &g_entities[trace.entityNum];


	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
	if ( cg.crosshairClientNum != cg.identifyClientNum && cg.crosshairClientNum != ENTITYNUM_WORLD ) {
		cg.identifyClientRequest = cg.crosshairClientNum;
	}
}



/*
==============
CG_DrawDynamiteStatus
==============
*/
static void CG_DrawDynamiteStatus( void ) {
	float color[4];
	char        *name;
	int timeleft;
	float w;

	if ( cg.snap->ps.weapon != WP_DYNAMITE && cg.snap->ps.weapon != WP_DYNAMITE2 ) {
		return;
	}

	if ( cg.snap->ps.grenadeTimeLeft <= 0 ) {
		return;
	}

	timeleft = cg.snap->ps.grenadeTimeLeft;

//	color = g_color_table[ColorIndex(COLOR_RED)];
	color[0] = color[3] = 1.0f;

	// fade red as it pulses past seconds
	color[1] = color[2] = 1.0f - ( (float)( timeleft % 1000 ) * 0.001f );

	if ( timeleft < 300 ) {        // fade up the text
		color[3] = (float)timeleft / 300.0f;
	}

	trap_R_SetColor( color );

	timeleft *= 5;
	timeleft -= ( timeleft % 5000 );
	timeleft += 5000;
	timeleft /= 1000;

	name = va( "Timer: 30" );
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;

	color[3] *= cg_hudAlpha.value;
	CG_DrawBigStringColor( 320 - w / 2, 170, name, color );

	trap_R_SetColor( NULL );
}


#define CH_KNIFE_DIST       48  // from g_weapon.c
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96

#define CH_DIST             100 //128		// use the largest value from above

/*
==============
CG_CheckForCursorHints
	concept in progress...
==============
*/
void CG_CheckForCursorHints( void ) {
	trace_t trace;
	vec3_t start, end;
	centity_t   *tracent;
	vec3_t pforward, eforward;
	float dist;


	if ( cg.renderingThirdPerson ) {
		return;
	}

	if ( cg.snap->ps.serverCursorHint ) {  // server is dictating a cursor hint, use it.
		cg.cursorHintTime = cg.time;
		cg.cursorHintFade = 500;    // fade out time
		cg.cursorHintIcon = cg.snap->ps.serverCursorHint;
		cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
		return;
	}


	// From here on it's client-side cursor hints.  So if the server isn't sending that info (as an option)
	// then it falls into here and you can get basic cursorhint info if you want, but not the detailed info
	// the server sends.

	// the trace
	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, CH_DIST, cg.refdef.viewaxis[0], end );

//	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_ALL &~CONTENTS_MONSTERCLIP);
	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_PLAYERSOLID );

	if ( trace.fraction == 1 ) {
		return;
	}

	dist = trace.fraction * CH_DIST;

	tracent = &cg_entities[ trace.entityNum ];

	//
	// world
	//
	if ( trace.entityNum == ENTITYNUM_WORLD ) {
		// if looking into water, warn if you don't have a breather
		if ( ( trace.contents & CONTENTS_WATER ) && !( cg.snap->ps.powerups[PW_BREATHER] ) ) {
			if ( dist <= CH_WATER_DIST ) {
				cg.cursorHintIcon = HINT_WATER;
				cg.cursorHintTime = cg.time;
				cg.cursorHintFade = 500;
			}
		}
		// ladder
		else if ( ( trace.surfaceFlags & SURF_LADDER ) && !( cg.snap->ps.pm_flags & PMF_LADDER ) ) {
			if ( dist <= CH_LADDER_DIST ) {
				cg.cursorHintIcon = HINT_LADDER;
				cg.cursorHintTime = cg.time;
				cg.cursorHintFade = 500;
			}
		}


	}
	//
	// people
	//
	else if ( trace.entityNum < MAX_CLIENTS ) {

		// knife
		if ( trace.entityNum < MAX_CLIENTS && ( cg.snap->ps.weapon == WP_KNIFE || cg.snap->ps.weapon == WP_KNIFE2 ) ) {
			if ( dist <= CH_KNIFE_DIST ) {

				AngleVectors( cg.snap->ps.viewangles,   pforward, NULL, NULL );
				AngleVectors( tracent->lerpAngles,      eforward, NULL, NULL );

				if ( DotProduct( eforward, pforward ) > 0.9f ) {   // from behind
					cg.cursorHintIcon = HINT_KNIFE;
					cg.cursorHintTime = cg.time;
					cg.cursorHintFade = 100;
				}
			}
		}
	}
	//
	// other entities
	//
	else {
		if ( tracent->currentState.dmgFlags ) {
			switch ( tracent->currentState.dmgFlags ) {
			case HINT_DOOR:
				if ( dist < CH_DOOR_DIST ) {
					cg.cursorHintIcon = HINT_DOOR;
					cg.cursorHintTime = cg.time;
					cg.cursorHintFade = 500;
				}
				break;
				//case HINT_CHAIR:
			case HINT_MG42:
				if ( dist < CH_DOOR_DIST && !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {
					cg.cursorHintIcon = HINT_ACTIVATE;
					cg.cursorHintTime = cg.time;
					cg.cursorHintFade = 500;
				}
				break;
			}
		} else {

			if ( tracent->currentState.eType == ET_EXPLOSIVE ) {
				if ( ( dist < CH_BREAKABLE_DIST ) && ( cg.cursorHintIcon != HINT_FORCENONE ) ) { // JPW NERVE so we don't get breakables in trigger_objective_info fields for wrong team (see code chunk in g_main.c)
					cg.cursorHintIcon = HINT_BREAKABLE;
					cg.cursorHintTime = cg.time;
					cg.cursorHintFade = 500;
				}

			}
		}
	}
}


void CG_DrawPlayerAmmo(float *color, int weapon, int playerAmmo, int playerAmmoClip, int playerNades) {
	const char* s;
	float w;

	if (weapon == WP_GRENADE_PINEAPPLE || weapon == WP_GRENADE_LAUNCHER || weapon == WP_KNIFE || weapon == WP_KNIFE2) {
		s = va("G:%i", playerNades);
		w = CG_DrawStrlen(s) * TINYCHAR_WIDTH;
		CG_DrawStringExt(320 - w / 2, 205, s, color, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 20);
	}
	else {
		s = va("AMMO:%i-%i", playerAmmoClip + playerAmmo, playerNades);
		w = CG_DrawStrlen(s) * TINYCHAR_WIDTH;
		CG_DrawStringExt(320 - w / 2, 205, s, color, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 20);
	}
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float       *color;
	char        *name;
	float w;
	// NERVE - SMF
	const char  *s, *playerClass;
	int playerHealth, cgClass, val;
	vec4_t c;
	float barFrac;
	// -NERVE - SMF

	if ( cg_drawCrosshair.integer < 0 ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// NERVE - SMF - we don't want to do this in warmup // RtcwPro we can do this during warmup
	//if ( cgs.gamestate != GS_PLAYING && cgs.gametype == GT_WOLF_STOPWATCH ) {
	//	return;
	//}

	// Ridah
	if ( cg_gameType.integer == GT_SINGLE_PLAYER ) {
		return;
	}
	// done.

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 250 );

	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	// NERVE - SMF
	if ( cg.crosshairClientNum > MAX_CLIENTS ) {
		return;
	}

	// we only want to see players on our team
	if ( cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR
		 && cgs.clientinfo[ cg.crosshairClientNum ].team != cgs.clientinfo[cg.snap->ps.clientNum].team ) {
		return;
	}

	// determine player class
	cgClass = cg_entities[cg.snap->ps.clientNum].currentState.teamNum;

	val = cg_entities[ cg.crosshairClientNum ].currentState.teamNum;
	if ( val == 0 ) {
		playerClass = "S";
	} else if ( val == 1 ) {
		playerClass = "M";
	} else if ( val == 2 ) {
		playerClass = "E";
	} else if ( val == 3 ) {
		playerClass = "L";
	} else {
		playerClass = "";
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;

	s = va( "[%s] %s", CG_TranslateString( playerClass ), name );
	if ( !s ) {
		return;
	}
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;

	// draw the name and class
	// RtcwPro - Colored names
	if (cg_coloredCrosshairNames.integer) {
		CG_DrawStringExt(320 - w / 2, 170, s, color, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 40);
	}
	else {
		CG_DrawSmallStringColor(320 - w / 2, 170, s, color);
	}

	// draw the health bar
	playerHealth = cg.identifyClientHealth;

	if ( cg.crosshairClientNum == cg.identifyClientNum ) {
		barFrac = (float)playerHealth / 100;

		if ( barFrac > 1.0 ) {
			barFrac = 1.0;
		} else if ( barFrac < 0 ) {
			barFrac = 0;
		}

		c[0] = 1.0f;
		c[1] = c[2] = barFrac;
		c[3] = 0.25 + barFrac * 0.5 * color[3];

		CG_FilledBar( 320 - w / 2, 190, 110, 10, c, NULL, NULL, barFrac, 16 );
	}
	// -NERVE - SMF

	// RtcwPro add player ammo if cg_drawCrosshairNames is 1
	if (cg_drawCrosshairNames.integer == 1 && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
		CG_DrawPlayerAmmo(color, cgs.clientinfo[cg.crosshairClientNum].playerWeapon, cgs.clientinfo[cg.crosshairClientNum].playerAmmo,
			cgs.clientinfo[cg.crosshairClientNum].playerAmmoClip, cgs.clientinfo[cg.crosshairClientNum].playerNades);
	}

	trap_R_SetColor( NULL );
}



//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator( void ) {
	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}
	
	if (cgs.clientinfo[cg.clientNum].shoutStatus == 1)
	{
		CG_DrawBigString(320 - 9 * 8, 440, CG_TranslateString("SHOUTCASTER"), 1.0F);
	}
	else
	{
		CG_DrawBigString(320 - 9 * 8, 440, CG_TranslateString("SPECTATOR"), 1.0F);
	}

	if ( cgs.gametype == GT_TOURNAMENT ) {
		CG_DrawBigString( 320 - 15 * 8, 460, "waiting to play", 1.0F );
	}
	if ( cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF ) {
		CG_DrawBigString( 320 - 25 * 8, 460, "use the TEAM menu to play", 1.0F );
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote( void ) {
	char *s, *isTournament;
	char str1[32], str2[32];
	float color[4] = { 1, 1, 0, 1 };
	int sec;
	const char* info;

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
	}

	if ( cgs.complaintEndTime > cg.time && !cg.demoPlayback) {

		// RtcwPro exit complaint dialog if g_tournament is 1
		info = CG_ConfigString(CS_SERVERINFO);
		isTournament = Info_ValueForKey(info, "g_tournament");
		if (isTournament != NULL && !Q_stricmp(isTournament, "1"))
			return;

		if ( cgs.complaintClient == -1 ) {
			s = "Your complaint has been filed";
			CG_DrawStringExt( 8, 200, CG_TranslateString( s ), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80 );
			return;
		}
		if ( cgs.complaintClient == -2 ) {
			s = "Complaint dismissed";
			CG_DrawStringExt( 8, 200, CG_TranslateString( s ), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80 );
			return;
		}
		if ( cgs.complaintClient == -3 ) {
			s = "Server Host cannot be complained against";
			CG_DrawStringExt( 8, 200, CG_TranslateString( s ), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80 );
			return;
		}
		if (cgs.complaintClient == -4 && cg_complaintPopUp.integer) {
			s = "You were team-killed by the Server Host";
			CG_DrawStringExt( 8, 200, CG_TranslateString( s ), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80 );
			return;
		}

		Q_strncpyz( str1, BindingFromName( "vote yes" ), 32 );
		if ( !Q_stricmp( str1, "???" ) ) {
			Q_strncpyz( str1, "vote yes", 32 );
		}
		Q_strncpyz( str2, BindingFromName( "vote no" ), 32 );
		if ( !Q_stricmp( str2, "???" ) ) {
			Q_strncpyz( str2, "vote no", 32 );
		}

		// RtcwPro - Complaint popup
		if (cg_complaintPopUp.integer && cgs.gamestate != GS_WARMUP)
		{
			s = va(CG_TranslateString("File complaint against %s for team-killing?"), cgs.clientinfo[cgs.complaintClient].name);
			CG_DrawStringExt(8, 200, s, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

			s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
			CG_DrawStringExt(8, 214, s, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		}
		return;
	}

	if ( !cgs.voteTime ) {
		return;
	}

	Q_strncpyz( str1, BindingFromName( "vote yes" ), 32 );
	if ( !Q_stricmp( str1, "???" ) ) {
		Q_strncpyz( str1, "vote yes", 32 );
	}
	Q_strncpyz( str2, BindingFromName( "vote no" ), 32 );
	if ( !Q_stricmp( str2, "???" ) ) {
		Q_strncpyz( str2, "vote no", 32 );
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	if ( !( cg.snap->ps.eFlags & EF_VOTED ) ) {
		s = va( CG_TranslateString( "VOTE(%i):%s" ), sec, cgs.voteString );
		CG_DrawStringExt( 8, 200, s, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 60 );

		s = va( CG_TranslateString( "YES(%s):%i, NO(%s):%i" ), str1, cgs.voteYes, str2, cgs.voteNo );
		CG_DrawStringExt( 8, 214, s, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 60 );
	} else {
		s = va( CG_TranslateString( "Y:%i, N:%i" ), cgs.voteYes, cgs.voteNo );
		CG_DrawStringExt( 8, 214, s, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 20 );
	}
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		return;
	}

	// RTCWPro - Auto Actions
	if (!cg.demoPlayback) {
		static int doScreenshot = 0, doDemostop = 0;

		if (!cg.latchAutoActions) {
			cg.latchAutoActions = qtrue;

			// Some instantly open console to check the stats..
			if (cg_autoAction.integer & AA_SCREENSHOT) {
				doScreenshot = cg.time + 250;
			}
			if (cg_autoAction.integer & AA_STATSDUMP) {
				CG_dumpStats_f();
			}
			if ((cg_autoAction.integer & AA_DEMORECORD) &&
				((cgs.gametype == GT_WOLF_STOPWATCH && cgs.currentRound == 0) ||
				cgs.gametype != GT_WOLF_STOPWATCH)) {
				doDemostop = cg.time + 5000;
			}
		}

		if (doScreenshot > 0 && doScreenshot < cg.time) {
			CG_autoScreenShot_f();
			doScreenshot = 0;
		}

		if (doDemostop > 0 && doDemostop < cg.time) {
			trap_SendConsoleCommand("stoprecord\n");
			doDemostop = 0;
		}
	}

	trap_Cvar_Set("cg_spawnTimer_set", "-1");
	trap_Cvar_Set("cg_spawnTimer_period", "0");
	// RTCWPro

	cg.scoreFadeTime = cg.time;
	CG_DrawScoreboard();
}

// NERVE - SMF
/*
=================
CG_ActivateLimboMenu
=================
*/
static void CG_ActivateLimboMenu( void ) {
	// ATVI Wolfenstein Misc #339
	// track when the UI would disable limbo, that leaves us in an inconsistent latch state
	// the inconsistent state is a good thing most of the time, except when game sends us back to free fly
	// that had the bad effect of triggering limbo again
	static qboolean ui_disable = qfalse;
	// track when we see cgame conditions change and need to emit a new limbo console command
	static qboolean latch = qfalse;
	qboolean test;
	char buf[32];

	if ( cgs.gametype < GT_WOLF ) {
		return;
	}

	// a test to detect when UI closes the limbo
	trap_Cvar_VariableStringBuffer( "ui_limboMode", buf, sizeof( buf ) );
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && atoi( buf ) == 0 && latch == 1 ) {
		ui_disable = qtrue;
	}

	// should we open the limbo menu
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		test = qfalse;
	} else {
		test = cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.warmup;
	}

	// auto open/close limbo mode
	if ( cg_popupLimboMenu.integer ) {
		// we don't want to trigger limbo in this very particular case
		if ( ui_disable && cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && test && !latch ) {
			// ATVI Wolfenstein Misc #413
			// we manually update this, otherwise there's a chance team selections won't work
			trap_Cvar_Set( "mp_currentTeam", "2" );
			latch = 1;
			ui_disable = qfalse;
		}
		if ( test && !latch ) {
			trap_SendConsoleCommand( "OpenLimboMenu\n" );
			latch = qtrue;
			ui_disable = qfalse;
		} else if ( !test && latch )   {
			trap_SendConsoleCommand( "CloseLimboMenu\n" );
			latch = qfalse;
		}
	}

	if ( atoi( buf ) ) {
		cg.limboMenu = qtrue;
	} else {
		cg.limboMenu = qfalse;
	}
}

/*
=================
CG_DrawSpectatorMessage
=================
*/
static void CG_DrawSpectatorMessage( void ) {
	float color[4] = { 1, 1, 1, 1 };
	const char *str, *str2;
	float x, y;
	char buf[32];

	if ( cgs.gametype < GT_WOLF ) {
		return;
	}

	if ( !cg_descriptiveText.integer ) {
		return;
	}

	if ( !( cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) ) {
		return;
	}

	// RtcwPro - Never during demo..
	if (cg.demoPlayback) {
		return;
	}
	
	trap_Cvar_VariableStringBuffer( "ui_limboMode", buf, sizeof( buf ) );
	if ( atoi( buf ) ) {
		return;
	}

	Controls_GetConfig();

	x = 80;
	y = 408;

	str2 = BindingFromName( "OpenLimboMenu" );
	if ( !Q_stricmp( str2, "???" ) ) {
		str2 = "ESCAPE";
	}
	str = va( CG_TranslateString( "- Press %s to open Limbo Menu" ), str2 );
	CG_DrawStringExt( x, y, str, color, qtrue, 0, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
	y += TINYCHAR_HEIGHT;

	str2 = BindingFromName( "mp_QuickMessage" );
	str = va( CG_TranslateString( "- Press %s to open quick message menu" ), str2 );
	CG_DrawStringExt( x, y, str, color, qtrue, 0, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
	y += TINYCHAR_HEIGHT;

	str2 = BindingFromName( "+attack" );
	str = va( CG_TranslateString( "- Press %s to follow next player" ), str2 );
	CG_DrawStringExt( x, y, str, color, qtrue, 0, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
	y += TINYCHAR_HEIGHT;
}

/*
=================
CG_DrawLimboMessage
=================
*/

#define INFOTEXT_STARTX 42

static void CG_DrawLimboMessage( void ) {
	float color[4] = { 1, 1, 1, 1 };
	const char *str;
	playerState_t *ps;
	//int w;

	if ( cgs.gametype < GT_WOLF ) {
		return;
	}

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] > 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_LIMBO || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR ) {
		return;
	}

	color[3] *= cg_hudAlpha.value;

	if ( cg_descriptiveText.integer ) {
		str = CG_TranslateString( "You are wounded and waiting for a medic." );
		CG_DrawSmallStringColor( INFOTEXT_STARTX, 68, str, color );

		str = CG_TranslateString( "Press JUMP to go into reinforcement queue." );
		CG_DrawSmallStringColor( INFOTEXT_STARTX, 86, str, color );
	}

	// JPW NERVE
	if ( cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0 ) {
		str = CG_TranslateString( "No more reinforcements this round." );
	} else if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ) {
		str = va( CG_TranslateString( "Reinforcements deploy in %d seconds." ),
				  (int)( 1 + (float)( cg_redlimbotime.integer - ( cg.time % cg_redlimbotime.integer ) ) * 0.001f ) );
	} else {
		str = va( CG_TranslateString( "Reinforcements deploy in %d seconds." ),
				  (int)( 1 + (float)( cg_bluelimbotime.integer - ( cg.time % cg_bluelimbotime.integer ) ) * 0.001f ) );
	}

	CG_DrawSmallStringColor( INFOTEXT_STARTX, 104, str, color );
	// jpw

	trap_R_SetColor( NULL );
}
// -NERVE - SMF

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	//float x;
	vec4_t color;
	const char  *name;
	char deploytime[128];        // JPW NERVE
	float y;

	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {
		return qfalse;
	}

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// JPW NERVE -- if in limbo, show different follow message
	if ( cg.snap->ps.pm_flags & PMF_LIMBO ) {
		color[1] = 0.0;
		color[2] = 0.0;
		if ( cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0 ) {
			Q_strncpyz( deploytime, CG_TranslateString( "No more deployments this round" ), sizeof(deploytime) );
		} else if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ) {
			Com_sprintf( deploytime, sizeof(deploytime), CG_TranslateString( "Deploying in %d seconds" ),
					 (int)( 1 + (float)( cg_redlimbotime.integer - ( cg.time % cg_redlimbotime.integer ) ) * 0.001f ) );
		} else {
			Com_sprintf( deploytime, sizeof(deploytime), CG_TranslateString( "Deploying in %d seconds" ),
					 (int)( 1 + (float)( cg_bluelimbotime.integer - ( cg.time % cg_bluelimbotime.integer ) ) * 0.001f ) );
		}

        CG_DrawStringExt( INFOTEXT_STARTX, 83, deploytime, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 80 );// below respawn timer

		// DHM - Nerve :: Don't display if you're following yourself
		if ( cg.snap->ps.clientNum != cg.clientNum ) {
			Com_sprintf( deploytime, sizeof(deploytime), "(%s %s)", CG_TranslateString( "Following" ), cgs.clientinfo[ cg.snap->ps.clientNum ].name );
			CG_DrawStringExt( INFOTEXT_STARTX, 101, deploytime, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 80 ); //  below respawn timer
		}
	} else {
// jpw
		//CG_DrawSmallString( INFOTEXT_STARTX, 68, CG_TranslateString( "following" ), 1.0F ); // original location
		if (cgs.gamestate != GS_PLAYING && cgs.currentRound) { y = 63; } else { y = 83; }
		CG_DrawSmallString( INFOTEXT_STARTX, y, CG_TranslateString( "following" ), 1.0F ); // below respawn timer

		name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;
        //CG_DrawStringExt( 120, 68, name, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 ); // original location
		CG_DrawStringExt( 120, y, name, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 ); // below respawn timer
	} // JPW NERVE
	return qtrue;
}

/*
=================
RtcwPro - CG_DrawPause

Deals with client views/prints when paused.
=================
*/
static void CG_PausePrint( void ) {
	const char* s, * s2 = "";
	float color[4];
	int w;

	// Not in warmup...
	if (cg.warmup)
		return;

	if (cgs.match_paused == PAUSE_ON) {
		s = va("%s", CG_TranslateString("^nMatch is Paused!"));
		s2 = va("%s", CG_TranslateString(va("Timeout expires in ^n%i ^7seconds", cgs.match_resumes - cgs.match_expired)));

		color[3] = fabs(sin(cg.time * 0.001)) * cg_hudAlpha.value;

		if (cg.time > cgs.match_stepTimer) {
			cgs.match_expired++;
			cgs.match_stepTimer = cg.time + 1000;
		}
	}
	else if (cgs.match_paused == PAUSE_RESUMING) {
		s = va("%s", CG_TranslateString("^3Prepare to fight!"));
		if (11 - cgs.match_expired < 11) {
			s2 = va("%s", CG_TranslateString(va("Resuming Match in ^3%d", 11 - cgs.match_expired)));
		}

		color[3] = fabs(sin(cg.time * 0.002)) * cg_hudAlpha.value;

		if (cg.time > cgs.match_stepTimer) {
			cgs.match_expired++;
			cgs.match_stepTimer = cg.time + 1000;
		}
	}
	else {
		return;
	}

	color[0] = color[1] = color[2] = 1.0;

	w = CG_DrawStrlen(s);
	CG_DrawStringExt(320 - w * 6, 100, s, color, qfalse, qtrue, 12, 18, 0);

	w = CG_DrawStrlen(s2);
	CG_DrawStringExt(320 - w * 6, 120, s2, colorWhite, qfalse, qtrue, 12, 18, 0);
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int w;
	int sec;
	int cw = 10;
	const char  *s, *s1, *s2, *configString, *info, *t;
    static qboolean announced = qfalse;
	char *configName;

	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		return;     // (SA) don't bother with this stuff in sp
	}

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);
	}

	info = CG_ConfigString(CS_SERVERINFO);
	configName = Info_ValueForKey(info, "sv_GameConfig");
	if (configName != NULL) configString = va("^3%s Config Loaded", configName);

	// RtcwPro - Ready
	if (cgs.gamestate == GS_WARMUP && cgs.readyState != CREADY_NONE) {

		if (cgs.currentRound) {
			t = va(CG_TranslateString("Clock is now set to %s!"), WM_TimeToString(cgs.nextTimeLimit * 60.f * 1000.f));
			w = CG_DrawStrlen(t);
			CG_DrawStringExt(320 - w * cw / 2, 100, t, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5), 0);

			if (configString != NULL) {
				w = CG_DrawStrlen(configString);
				CG_DrawStringExt(320 - w * cw / 2, 80, configString, colorWhite,
					qfalse, qtrue, cw, (int)(cw * 1.5), 0);
			}
		}
		else {
			if (configString != NULL) {
				w = CG_DrawStrlen(configString);
				CG_DrawStringExt(320 - w * cw / 2, 100, configString, colorWhite,
					qfalse, qtrue, cw, (int)(cw * 1.5), 0);
			}
		}

		// No need to bother with count..scoreboard gives info..
		s = va(CG_TranslateString("^3WARMUP:^7 Waiting on ^2%i ^7%s"), cgs.minclients, cgs.minclients == 1 ? "player" : "players");
		w = CG_DrawStrlen( s );
		CG_DrawStringExt( 320 - w * 6, 120, s, colorWhite, qfalse, qtrue, 12, 18, 0 );

		if ( !cg.demoPlayback && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) || ( cg.snap->ps.pm_flags & PMF_LIMBO ) ) ) {
			s1 = (player_ready_status[cg.clientNum].isReady) ? "^3You are ready" : CG_TranslateString("Type ^3\\ready ^7in the console to start");
			w = CG_DrawStrlen( s1 );
			CG_DrawStringExt( 320 - w * cw / 2, 140, s1, colorWhite,
								qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );
		}

		return;
	}
	sec = cg.warmup;
	if ( !sec ) {
		if ( cgs.gamestate == GS_WAITING_FOR_PLAYERS ) {

			if (configString != NULL)
			{
				w = CG_DrawStrlen(configString);
				CG_DrawStringExt(320 - w * cw / 2, 100, configString, colorWhite,
					qfalse, qtrue, cw, (int)(cw * 1.5), 0);
			}

			s = CG_TranslateString( "^3WARMUP:^7 Waiting for more players" );

			w = CG_DrawStrlen( s );
			CG_DrawStringExt( 320 - w * 6, 120, s, colorWhite, qfalse, qtrue, 12, 18, 0 );


			s1 = va( CG_TranslateString( "Waiting for ^3%i ^7players" ), cgs.minclients );
			s2 = CG_TranslateString( "or call a vote to start match" );

			w = CG_DrawStrlen( s1 );
			CG_DrawStringExt( 320 - w * cw / 2, 140, s1, colorWhite,
							  qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );

			w = CG_DrawStrlen( s2 );
			CG_DrawStringExt( 320 - w * cw / 2, 160, s2, colorWhite,
							  qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );

			return;
		}

		return;
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	s = va( "%s %i", CG_TranslateString( "^3(WARMUP) Match begins in: ^1" ), sec + 1 );
	if (sec == 5) trap_S_StartLocalSound(cgs.media.count5Sound, CHAN_ANNOUNCER);
	if (sec == 4) trap_S_StartLocalSound(cgs.media.count4Sound, CHAN_ANNOUNCER);
	if (sec == 3) trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
	if (sec == 2) trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
	if (sec == 1) trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);

	w = CG_DrawStrlen( s );
	CG_DrawStringExt( 320 - w * 6, 120, s, colorWhite, qfalse, qtrue, 12, 18, 0 );

    if (sec == 10 && !announced)
	{
		trap_S_StartLocalSound(cgs.media.prepFight, CHAN_ANNOUNCER);

		announced = qtrue;
	}
	else if (sec != 10)
	{
		announced = qfalse;
	}

	// NERVE - SMF - stopwatch stuff
	s1 = "";
	s2 = "";

	if ( cgs.gametype == GT_WOLF_STOPWATCH ) {
		const char  *cs;
		int defender;

		s = va( "%s %i", CG_TranslateString( "Stopwatch Round" ), cgs.currentRound + 1 );

		cs = CG_ConfigString( CS_MULTI_INFO );
		defender = atoi( Info_ValueForKey( cs, "defender" ) );

		if ( !defender ) {
			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
				if ( cgs.currentRound == 1 ) {
					s1 = "You have been switched to the Axis team";
					s2 = "Keep the Allies from beating the clock!";
				} else {
					s1 = "You are on the Axis team";
				}
			} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )   {
				if ( cgs.currentRound == 1 ) {
					s1 = "You have been switched to the Allied team";
					s2 = "Try to beat the clock!";
				} else {
					s1 = "You are on the Allied team";
				}
			}
		} else {
			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
				if ( cgs.currentRound == 1 ) {
					s1 = "You have been switched to the Axis team";
					s2 = "Try to beat the clock!";
				} else {
					s1 = "You are on the Axis team";
				}
			} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )   {
				if ( cgs.currentRound == 1 ) {
					s1 = "You have been switched to the Allied team";
					s2 = "Keep the Axis from beating the clock!";
				} else {
					s1 = "You are on the Allied team";
				}
			}
		}

		if ( strlen( s1 ) ) {
			s1 = CG_TranslateString( s1 );
		}

		if ( strlen( s2 ) ) {
			s2 = CG_TranslateString( s2 );
		}

		cw = 10;

		w = CG_DrawStrlen( s ); // RtcwPro - Pushed all lower for 20
		CG_DrawStringExt( 320 - w * cw / 2, 160, s, colorWhite,
						  qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );

		w = CG_DrawStrlen( s1 );
		CG_DrawStringExt( 320 - w * cw / 2, 180, s1, colorWhite,
						  qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );

		w = CG_DrawStrlen( s2 );
		CG_DrawStringExt( 320 - w * cw / 2, 200, s2, colorWhite,
						  qfalse, qtrue, cw, (int)( cw * 1.5 ), 0 );
	}
}

//==================================================================================

/*
=================
CG_DrawFlashFade
=================
*/
static void CG_DrawFlashFade( void ) {
	static int lastTime;
	int elapsed, time;
	vec4_t col;
	qboolean fBlackout = ( int_ui_blackout.integer > 0 );

	if (cg.demoPlayback) return;

	if ( cgs.fadeStartTime + cgs.fadeDuration < cg.time ) {
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	} else if ( cgs.fadeAlphaCurrent != cgs.fadeAlpha ) {
		elapsed = ( time = trap_Milliseconds() ) - lastTime;  // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading
		lastTime = time;
		if ( elapsed < 500 && elapsed > 0 ) {
			if ( cgs.fadeAlphaCurrent > cgs.fadeAlpha ) {
				cgs.fadeAlphaCurrent -= ( (float)elapsed / (float)cgs.fadeDuration );
				if ( cgs.fadeAlphaCurrent < cgs.fadeAlpha ) {
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			} else {
				cgs.fadeAlphaCurrent += ( (float)elapsed / (float)cgs.fadeDuration );
				if ( cgs.fadeAlphaCurrent > cgs.fadeAlpha ) {
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			}
		}
	}
	// OSP - ugh, have to inform the ui that we need to remain blacked out (or not)
	if ( int_ui_blackout.integer == 0 ) {
		if ( cg.snap->ps.powerups[PW_BLACKOUT] > 0 ) {
			trap_Cvar_Set( "ui_blackout", va( "%d", cg.snap->ps.powerups[PW_BLACKOUT] ) );
		}
	} else if ( cg.snap->ps.powerups[PW_BLACKOUT] == 0 ) {
		trap_Cvar_Set( "ui_blackout", "0" );
	}
	// now draw the fade
	if ( cgs.fadeAlphaCurrent > 0.0 || fBlackout ) {
		VectorClear( col );
		col[3] = ( fBlackout ) ? 1.0f : cgs.fadeAlphaCurrent;

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		 	CG_FillRect( -10, -10, 650, 490, col );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {	
			CG_FillRect( -10, -10, 650, 490, col );
		}

		CG_DrawScoreboard();
		//bani - #127 - bail out if we're a speclocked spectator with cg_draw2d = 0
		if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_SPECTATOR && !cg_draw2D.integer ) {
			return;
		}

		// OSP - Show who is speclocked
		if ( fBlackout  && !cg.showScores) {
			int i, nOffset = 90;
			char *str, *format = "The %s team is speclocked!";
			char *teams[TEAM_NUM_TEAMS] = { "??", "AXIS", "ALLIED", "???" };
			float color[4] = { 1, 1, 0, 1 };

			for ( i = TEAM_RED; i <= TEAM_BLUE; i++ ) {
				if ( cg.snap->ps.powerups[PW_BLACKOUT] & i ) {
					str = va( format, teams[i] );
					CG_DrawStringExt( INFOTEXT_STARTX, nOffset, str, color, qtrue, qfalse, 10, 10, 0 );
					nOffset += 12;
				}
			}
		}
	}
}



/*
==============
CG_DrawFlashZoomTransition
	hide the snap transition from regular view to/from zoomed

  FIXME: TODO: use cg_fade?
==============
*/
static void CG_DrawFlashZoomTransition( void ) {
	vec4_t color;
	float frac;
	int fadeTime;

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) {   // don't draw when on mg_42
		// keep the timer fresh so when you remove yourself from the mg42, it'll fade
		cg.zoomTime = cg.time;
		return;
	}

	if ( cgs.gametype != GT_SINGLE_PLAYER ) { // JPW NERVE
		fadeTime = 400;
	} else {
		if ( cg.zoomedScope ) {
			fadeTime = cg.zoomedScope;  //----(SA)
		} else {
			fadeTime = 300;
		}
	}
	// jpw

	frac = cg.time - cg.zoomTime;

	if ( frac < fadeTime ) {
		frac = frac / (float)fadeTime;

		if ( cg.weaponSelect == WP_SNOOPERSCOPE ) {
//			Vector4Set( color, 0.7f, 0.3f, 0.7f, 1.0f - frac );
//			Vector4Set( color, 1, 0.5, 1, 1.0f - frac );
//			Vector4Set( color, 0.5f, 0.3f, 0.5f, 1.0f - frac );
			Vector4Set( color, 0.7f, 0.6f, 0.7f, 1.0f - frac );
		} else {
			Vector4Set( color, 0, 0, 0, 1.0f - frac );
		}

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( -10, -10, 650, 490, color );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( -10, -10, 650, 490, color );
		}
	}
}



/*
=================
CG_DrawFlashDamage
=================
*/
static void CG_DrawFlashDamage( void ) {
	vec4_t col;
	float redFlash;

	if ( !cg.snap ) {
		return;
	}

	if (!cg_blood.integer) {
		return;
	}

	if ( cg.v_dmg_time > cg.time ) {
		redFlash = fabs( cg.v_dmg_pitch * ( ( cg.v_dmg_time - cg.time ) / DAMAGE_TIME ) );

		// blend the entire screen red
		if ( redFlash > 5 ) {
			redFlash = 5;
		}

		VectorSet( col, 0.2, 0, 0 );
		// RtcwPro - Blood Flash
		col[3] = 0.7 * (redFlash / 5.0) * ( (cg_bloodFlash.value > 1.0) ? 1.0 :
											(cg_bloodFlash.value < 0.0) ? 0.0 :
											cg_bloodFlash.value);

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( -10, -10, 650, 490, col );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( -10, -10, 650, 490, col );
		}
	}
}


/*
=================
CG_DrawFlashFire
=================
*/
static void CG_DrawFlashFire( void ) {
	vec4_t col = {1,1,1,1};
	float alpha, max, f;

	if ( !cg.snap ) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	if ( !cg.snap->ps.onFireStart ) {
		cg.v_noFireTime = cg.time;
		return;
	}

	alpha = (float)( ( FIRE_FLASH_TIME - 1000 ) - ( cg.time - cg.snap->ps.onFireStart ) ) / ( FIRE_FLASH_TIME - 1000 );
	if ( alpha > 0 ) {
		if ( alpha >= 1.0 ) {
			alpha = 1.0;
		}

		// fade in?
		f = (float)( cg.time - cg.v_noFireTime ) / FIRE_FLASH_FADEIN_TIME;
		if ( f >= 0.0 && f < 1.0 ) {
			alpha = f;
		}

		max = 0.5 + 0.5 * sin( (float)( ( cg.time / 10 ) % 1000 ) / 1000.0 );
		if ( alpha > max ) {
			alpha = max;
		}
		col[0] = alpha;
		col[1] = alpha;
		col[2] = alpha;
		col[3] = alpha;
		trap_R_SetColor( col );
		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_DrawPic( -10, -10, 650, 490, cgs.media.viewFlashFire[( cg.time / 50 ) % 16] );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_DrawPic( -10, -10, 650, 490, cgs.media.viewFlashFire[( cg.time / 50 ) % 16] );
		}
		trap_R_SetColor( NULL );

		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)( 255.0 * alpha ) );
		CG_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.flameCrackSound, (int)( 255.0 * alpha ) );
	} else {
		cg.v_noFireTime = cg.time;
	}
}

/*
=================
CG_DrawFlashLightning
=================
*/
static void CG_DrawFlashLightning( void ) {
	centity_t *cent;
	qhandle_t shader;

	if ( !cg.snap ) {
		return;
	}

	if ( cg_thirdPerson.integer ) {
		return;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ( !cent->pe.teslaDamagedTime || ( cent->pe.teslaDamagedTime > cg.time ) ) {
		return;
	}

	if ( ( cg.time / 50 ) % ( 2 + ( cg.time % 2 ) ) == 0 ) {
		shader = cgs.media.viewTeslaAltDamageEffectShader;
	} else {
		shader = cgs.media.viewTeslaDamageEffectShader;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
		CG_DrawPic( -10, -10, 650, 490, shader );
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	} else {
		CG_DrawPic( -10, -10, 650, 490, shader );
	}
}


/*
==============
CG_DrawFlashBlendBehindHUD
	screen flash stuff drawn first (on top of world, behind HUD)
==============
*/
static void CG_DrawFlashBlendBehindHUD( void ) {
	CG_DrawFlashZoomTransition();
}


/*
=================
CG_DrawFlashBlend
	screen flash stuff drawn last (on top of everything)
=================
*/
static void CG_DrawFlashBlend( void ) {
	CG_DrawFlashLightning();
	CG_DrawFlashFire();
	CG_DrawFlashDamage();
	CG_DrawFlashFade();
}

// NERVE - SMF
/*
=================
CG_DrawObjectiveInfo
=================
*/
#define OID_LEFT    10
#define OID_TOP     360

void CG_ObjectivePrint( const char *str, int charWidth ) {
	char    *s;
	int i, len;                         // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF

	s = CG_TranslateString( str );

	Q_strncpyz( cg.oidPrint, s, sizeof( cg.oidPrint ) );

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen( cg.oidPrint );
	for ( i = 0; i < len; i++ ) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if ( i % ( CP_LINEWIDTH - 20 ) == 0 && i > 0 ) {
			neednewline = qtrue;
		}
		if ( cg.oidPrint[i] == ' ' && neednewline ) {
			cg.oidPrint[i] = '\n';
			neednewline = qfalse;
		}
	}
	// -NERVE - SMF

	cg.oidPrintTime = cg.time;
	cg.oidPrintY = OID_TOP;
	cg.oidPrintCharWidth = charWidth;

	// count the number of lines for oiding
	cg.oidPrintLines = 1;
	s = cg.oidPrint;
	while ( *s ) {
		if ( *s == '\n' ) {
			cg.oidPrintLines++;
		}
		s++;
	}
}

static void CG_DrawObjectiveInfo( void ) {
	char    *start;
	int l;
	int x, y, w, h;
	int x1, y1, x2, y2;
	float   *color;
	vec4_t backColor;
	backColor[0] = 0.2f;
	backColor[1] = 0.2f;
	backColor[2] = 0.2f;
	backColor[2] = cg_hudAlpha.value;

	if ( !cg.oidPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.oidPrintTime, 250 );
	if ( !color ) {
		cg.oidPrintTime = 0;
		return;
	}

	if ( cg_fixedAspect.integer ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	}

	trap_R_SetColor( color );

	start = cg.oidPrint;

// JPW NERVE
	//	y = cg.oidPrintY - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;
	y = 415 - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	x1 = 319;
	y1 = y - 2;
	x2 = 321;
// jpw

	// first just find the bounding rect
	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < CP_LINEWIDTH; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen( linebuffer ) + 10;
// JPW NERVE
		if ( 320 - w / 2 < x1 ) {
			x1 = 320 - w / 2;
			x2 = 320 + w / 2;
		}
		x = 320 - w / 2;
// jpw

		y += cg.oidPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	x2 = x2 + 4;
	y2 = y - cg.oidPrintCharWidth * 1.5 + 4;

	h = y2 - y1; // JPW NERVE

	VectorCopy( color, backColor );
	backColor[3] = 0.5 * color[3];
	trap_R_SetColor( backColor );

	CG_DrawPic( x1, y1, x2 - x1, y2 - y1, cgs.media.teamStatusBar );

	VectorSet( backColor, 0, 0, 0 );
	CG_DrawRect( x1, y1, x2 - x1, y2 - y1, 1, backColor );

	trap_R_SetColor( color );

	// do the actual drawing
	start = cg.oidPrint;
//	y = cg.oidPrintY - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;
	y = 415 - cg.oidPrintLines * BIGCHAR_HEIGHT / 2; // JPW NERVE


	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < CP_LINEWIDTH; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen( linebuffer );
		if ( x1 + w > x2 ) {
			x2 = x1 + w;
		}

		x = 320 - w / 2; // JPW NERVE

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
						  cg.oidPrintCharWidth, (int)( cg.oidPrintCharWidth * 1.5 ), 0 );

		y += cg.oidPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}

void CG_DrawObjectiveIcons( void ) {
	float x, y, w, h, xx, fade; // JPW NERVE
	float startColor[4];
	const char *s, *buf;
	char teamstr[32];
	float captureHoldFract;
	int i, num, status,barheight;
	vec4_t hcolor = { 0.2f, 0.2f, 0.2f, 1.f };
	int msec, mins, seconds, tens; // JPW NERVE

	// RTCWPro
	playerState_t* ps;
	clientInfo_t* ci;
	
	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	}

// JPW NERVE added round timer
	y = 48;
	x = 5;
	msec = ( cgs.timelimit * 60.f * 1000.f ) - ( cg.time - cgs.levelStartTime );

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;
	// OSPx - Print fancy warmup in corner..
	if (cgs.gamestate != GS_PLAYING) {
		fade = fabs(sin(cg.time * 0.002)) * cg_hudAlpha.value;
		s = va("^3Warmup");
	} else if (msec < 0) {
		fade = fabs( sin( cg.time * 0.002 ) ) * cg_hudAlpha.value;
		s = va( "0:00" );
	} else {
		s = va( "%i:%i%i", mins, tens, seconds ); // float cast to line up with reinforce time
		fade = cg_hudAlpha.value;
	}

	CG_DrawSmallString( x,y,s,fade );

// jpw

	x = 5;
	y = 68;
	w = 24;
	h = 14;

	// draw the stopwatch
	if ( cgs.gametype == GT_WOLF_STOPWATCH ) {
		if ( cgs.currentRound == 0 ) {
			CG_DrawPic( 3, y, 30, 30, trap_R_RegisterShader( "sprites/stopwatch1.tga" ) );
		} else {
			CG_DrawPic( 3, y, 30, 30, trap_R_RegisterShader( "sprites/stopwatch2.tga" ) );
		}
		y += 34;
	}

	// determine character's team
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		strcpy( teamstr, "axis_desc" );
	} else {
		strcpy( teamstr, "allied_desc" );
	}

	s = CG_ConfigString( CS_MULTI_INFO );
	buf = Info_ValueForKey( s, "numobjectives" );

	if ( buf && atoi( buf ) ) {
		num = atoi( buf );

		VectorSet( hcolor, 0.3f, 0.3f, 0.3f );
		hcolor[3] = 0.7f * cg_hudAlpha.value; // JPW NERVE
		CG_DrawRect( x - 1, y - 1, w + 2, ( h + 4 ) * num - 4 + 2, 1, hcolor );

		VectorSet( hcolor, 1.0f, 1.0f, 1.0f );
		hcolor[3] = 0.2f * cg_hudAlpha.value; // JPW NERVE
		trap_R_SetColor( hcolor );
		CG_DrawPic( x, y, w, ( h + 4 ) * num - 4, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		for ( i = 0; i < num; i++ ) {
			s = CG_ConfigString( CS_MULTI_OBJECTIVE1 + i );
			Info_ValueForKey( s, teamstr );

			xx = x;

			hcolor[0] = 0.25f;
			hcolor[1] = 0.38f;
			hcolor[2] = 0.38f;
			hcolor[3] = 0.5f * cg_hudAlpha.value; // JPW NERVE
			trap_R_SetColor( hcolor );
			CG_DrawPic( xx, y, w, h, cgs.media.teamStatusBar );
			trap_R_SetColor( NULL );

			// draw text
			VectorSet( hcolor, 1.f, 1.f, 1.f );
			hcolor[3] = cg_hudAlpha.value; // JPW NERVE
//			CG_DrawStringExt( xx, y, va( "%i", i+1 ), hcolor, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 ); // JPW NERVE pulled per id request

//			xx += 10;

			// draw status flags
			s = CG_ConfigString( CS_MULTI_OBJ1_STATUS + i );
			status = atoi( Info_ValueForKey( s, "status" ) );

			VectorSet( hcolor, 1, 1, 1 );
			hcolor[3] = 0.7f * cg_hudAlpha.value; // JPW NERVE
			trap_R_SetColor( hcolor );

			if ( status == 0 ) {
				CG_DrawPic( xx, y, w, h, trap_R_RegisterShaderNoMip( "ui_mp/assets/ger_flag.tga" ) );
			} else if ( status == 1 )   {
				CG_DrawPic( xx, y, w, h, trap_R_RegisterShaderNoMip( "ui_mp/assets/usa_flag.tga" ) );
			}

			VectorSet( hcolor, 0.2f, 0.2f, 0.2f );
			hcolor[3] = cg_hudAlpha.value; // JPW NERVE
			trap_R_SetColor( hcolor );

//			CG_DrawRect( xx, y, w, h, 2, hcolor );

			y += h + 4;
		}
	}

// JPW NERVE compute & draw hold bar
	if ( cgs.gametype == GT_WOLF_CPH ) {
		if ( cg.snap->ps.stats[STAT_CAPTUREHOLD_RED] || cg.snap->ps.stats[STAT_CAPTUREHOLD_BLUE] ) {
			captureHoldFract = (float)cg.snap->ps.stats[STAT_CAPTUREHOLD_RED] /
							   (float)( cg.snap->ps.stats[STAT_CAPTUREHOLD_RED] + cg.snap->ps.stats[STAT_CAPTUREHOLD_BLUE] );
		} else {
			captureHoldFract = 0.5;
		}

		barheight = y - 72; // (68+4)


		startColor[0] = 1;
		startColor[1] = 1;
		startColor[2] = 1;
		startColor[3] = 1;
		if ( captureHoldFract > 0.5 ) {
			startColor[3] = cg_hudAlpha.value;
		} else {
			startColor[3] = cg_hudAlpha.value * ( ( captureHoldFract ) + 0.15 );
		}
//		startColor[3] = 0.25;
		trap_R_SetColor( startColor );
		CG_DrawPic( 32,68,5,barheight * captureHoldFract,cgs.media.redColorBar );

		if ( captureHoldFract < 0.5 ) {
			startColor[3] = cg_hudAlpha.value;
		} else {
			startColor[3] = cg_hudAlpha.value * ( 0.15 + ( 1.0f - captureHoldFract ) );
		}
//		startColor[3] = 0.25;
		trap_R_SetColor( startColor );
		CG_DrawPic( 32,68 + barheight * captureHoldFract,5,barheight - barheight * captureHoldFract,cgs.media.blueColorBar );
	}
// jpw


	// draw treasure icon if we have the flag
	y += 4;

	VectorSet( hcolor, 1, 1, 1 );
	hcolor[3] = cg_hudAlpha.value;
	trap_R_SetColor( hcolor );
	// RTCWPro
	/*if ( cgs.clientinfo[cg.snap->ps.clientNum].powerups & ( 1 << PW_REDFLAG ) ||
		 cgs.clientinfo[cg.snap->ps.clientNum].powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_DrawPic( -7, y, 48, 48, trap_R_RegisterShader( "models/multiplayer/treasure/treasure" ) );
		y += 50;
	}*/
	ps = &cg.snap->ps;
	ci = &cgs.clientinfo[ps->clientNum];
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG]) {
		CG_DrawPic(-7, y, 48, 48, trap_R_RegisterShader("models/multiplayer/treasure/treasure"));
		y += 50;
	}

}
// -NERVE - SMF

//==================================================================================


void CG_DrawTimedMenus( void ) {
	if ( cg.voiceTime ) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName( "voiceMenu" );
			trap_Cvar_Set( "cl_conXOffset", "0" );
			cg.voiceTime = 0;
		}
	}
}


/*
=================
CG_Fade
=================
*/
void CG_Fade( int r, int g, int b, int a, float time ) {
	if ( time <= 0 ) {  // do instantly
		cg.fadeRate = 1;
		cg.fadeTime = cg.time - 1;  // set cg.fadeTime behind cg.time so it will start out 'done'
	} else {
		cg.fadeRate = 1.0f / time;
		cg.fadeTime = cg.time + time;
	}

	cg.fadeColor2[ 0 ] = ( float )r / 255.0f;
	cg.fadeColor2[ 1 ] = ( float )g / 255.0f;
	cg.fadeColor2[ 2 ] = ( float )b / 255.0f;
	cg.fadeColor2[ 3 ] = ( float )a / 255.0f;
}


/*
=================
CG_ScreenFade
=================
*/
static void CG_ScreenFade( void ) {
	int msec;
	int i;
	float t, invt;
	vec4_t color;

	if ( !cg.fadeRate ) {
		return;
	}

	msec = cg.fadeTime - cg.time;
	if ( msec <= 0 ) {
		cg.fadeColor1[ 0 ] = cg.fadeColor2[ 0 ];
		cg.fadeColor1[ 1 ] = cg.fadeColor2[ 1 ];
		cg.fadeColor1[ 2 ] = cg.fadeColor2[ 2 ];
		cg.fadeColor1[ 3 ] = cg.fadeColor2[ 3 ];

		if ( !cg.fadeColor1[ 3 ] ) {
			cg.fadeRate = 0;
			return;
		}

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			CG_FillRect( 0, 0, 640, 480, cg.fadeColor1 );
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		} else {
			CG_FillRect( 0, 0, 640, 480, cg.fadeColor1 );
		}

	} else {
		t = ( float )msec * cg.fadeRate;
		invt = 1.0f - t;

		for ( i = 0; i < 4; i++ ) {
			color[ i ] = cg.fadeColor1[ i ] * t + cg.fadeColor2[ i ] * invt;
		}

		if ( color[ 3 ] ) {
			if ( cg_fixedAspect.integer ) {
				CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
				CG_FillRect( 0, 0, 640, 480, color );
				CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
			} else {
				CG_FillRect( 0, 0, 640, 480, color );
			}
		}
	}
}


// OSPx - Draw HUD Names
void CG_DrawOnScreenNames(void)
{
	static vec3_t	mins = { -1, -1, -1 };
	static vec3_t	maxs = { 1, 1, 1 };
	vec4_t			white = { 1.0f, 1.0f, 1.0f, 1.0f };
	specName_t		*spcNm;
	trace_t			tr;
	int				clientNum;
	int				FadeOut = 0;
	int				FadeIn = 0;

	for (clientNum = 0; clientNum < cgs.maxclients; clientNum++) {

		if (!cgs.clientinfo[clientNum].infoValid)
			continue;

		spcNm = &cg.specOnScreenNames[clientNum];
		if (!spcNm || !spcNm->visible)
			continue;

		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, spcNm->origin, -1, CONTENTS_SOLID);

		if (tr.fraction < 1.0f) {
			spcNm->lastInvisibleTime = cg.time;
		}
		else {
			spcNm->lastVisibleTime = cg.time;
		}

		FadeOut = cg.time - spcNm->lastVisibleTime;
		FadeIn = cg.time - spcNm->lastInvisibleTime;

		if (FadeIn) {
			white[3] = (FadeIn > 500) ? 1.0 : FadeIn / 500.0f;
			if (white[3] < spcNm->alpha)
				white[3] = spcNm->alpha;
		}
		if (FadeOut) {
			white[3] = (FadeOut > 500) ? 0.0 : 1.0 - FadeOut / 500.0f;
			if (white[3] > spcNm->alpha)
				white[3] = spcNm->alpha;
		}
		if (white[3] > 1.0)
			white[3] = 1.0;

		spcNm->alpha = white[3];
		if (spcNm->alpha <= 0.0) continue;	// no alpha = nothing to draw..

		CG_Text_Paint_ext2(spcNm->x, spcNm->y, spcNm->scale, white, spcNm->text, 0, 0, 0);
		// expect update next frame again
		spcNm->visible = qfalse;
	}
}

// JPW NERVE
void CG_Draw2D2( void ) {
	qhandle_t weapon;
	vec4_t hcolor;

	VectorSet( hcolor, 1.f,1.f,1.f );
//	VectorSet(hcolor, cg_hudAlpha.value, cg_hudAlpha.value, cg_hudAlpha.value);
	hcolor[3] = cg_hudAlpha.value;
	trap_R_SetColor( hcolor );

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	CG_DrawPic( 0, 480, 640, -70, cgs.media.hud1Shader );

	if ( !( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) {
		switch ( cg.snap->ps.weapon ) {
		case WP_COLT:
		case WP_LUGER:
			weapon = cgs.media.hud2Shader;
			break;
		case WP_KNIFE:
			weapon = cgs.media.hud5Shader;
			break;
		case WP_VENOM:
			weapon = cgs.media.hud4Shader;
			break;
		default:
			weapon = cgs.media.hud3Shader;
		}
		CG_DrawPic( 220,410, 200,-200,weapon );
	}
}
// jpw

/*
=================
CG_DrawCompassIcon

NERVE - SMF
=================
*/
void CG_DrawCompassIcon( int x, int y, int w, int h, vec3_t origin, vec3_t dest, qhandle_t shader ) {
	float angle, pi2 = M_PI * 2;
	vec3_t v1, angles;
	float len;

	VectorCopy( dest, v1 );
	VectorSubtract( origin, v1, v1 );
	len = VectorLength( v1 );
	VectorNormalize( v1 );
	vectoangles( v1, angles );

	if ( v1[0] == 0 && v1[1] == 0 && v1[2] == 0 ) {
		return;
	}

	angles[YAW] = AngleSubtract( cg.snap->ps.viewangles[YAW], angles[YAW] );

	angle = ( ( angles[YAW] + 180.f ) / 360.f - ( 0.50 / 2.f ) ) * pi2;

	w /= 2;
	h /= 2;

	x += w;
	y += h;

	w = sqrt( ( w * w ) + ( h * h ) ) / 3.f * 2.f * 0.9f;

	x = x + ( cos( angle ) * w );
	y = y + ( sin( angle ) * w );

	len = 1 - min( 1.f, len / 2000.f );

	CG_DrawPic( x - ( 14 * len + 4 ) / 2, y - ( 14 * len + 4 ) / 2, 14 * len + 4, 14 * len + 4, shader );
}

/*
=================
CG_DrawCompass

NERVE - SMF
=================
*/
static void CG_DrawCompass( void ) {
	// RTCWPro
	float basex = cg_compassX.integer; //290 
	float basey = cg_compassY.integer; //420;

	float basew = 60, baseh = 60;
	snapshot_t  *snap;
	vec4_t hcolor;
	float angle;
	int i;

	if ( cgs.gametype < GT_WOLF ) {
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
		CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	}

	angle = ( cg.snap->ps.viewangles[YAW] + 180.f ) / 360.f - ( 0.25 / 2.f );

	VectorSet( hcolor, 1.f,1.f,1.f );
	hcolor[3] = cg_hudAlpha.value;
	trap_R_SetColor( hcolor );

	CG_DrawRotatedPic( basex, basey, basew, baseh, trap_R_RegisterShader( "gfx/2d/compass2.tga" ), angle );
	CG_DrawPic( basex, basey, basew, baseh, trap_R_RegisterShader( "gfx/2d/compass.tga" ) );

	// draw voice chats
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		centity_t *cent = &cg_entities[i];

		if ( cg.snap->ps.clientNum == i || !cgs.clientinfo[i].infoValid || cg.snap->ps.persistant[PERS_TEAM] != cgs.clientinfo[i].team ) {
			continue;
		}

		// also draw revive icons if cent is dead and player is a medic
		if ( cent->voiceChatSpriteTime < cg.time ) {
			continue;
		}

		if ( cgs.clientinfo[i].health <= 0 ) {
			// reset
			cent->voiceChatSpriteTime = cg.time;
			continue;
		}

		CG_DrawCompassIcon( basex, basey, basew, baseh, cg.snap->ps.origin, cent->lerpOrigin, cent->voiceChatSprite );
	}

	// draw explosives if an engineer
	if ( cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER ) {
		if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
			snap = cg.nextSnap;
		} else {
			snap = cg.snap;
		}

		for ( i = 0; i < snap->numEntities; i++ ) {
			centity_t *cent = &cg_entities[ snap->entities[ i ].number ];

			if ( cent->currentState.eType != ET_EXPLOSIVE_INDICATOR ) {
				continue;
			}

			if ( cent->currentState.teamNum == 1 && cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
				continue;
			} else if ( cent->currentState.teamNum == 2 && cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				continue;
			}

			CG_DrawCompassIcon( basex, basey, basew, baseh, cg.snap->ps.origin, cent->lerpOrigin, trap_R_RegisterShader( "sprites/destroy.tga" ) );
		}
	}

	// draw revive medic icons
	if ( cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_MEDIC ) {
		if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
			snap = cg.nextSnap;
		} else {
			snap = cg.snap;
		}

		for ( i = 0; i < snap->numEntities; i++ ) {
			entityState_t *ent = &snap->entities[i];

			if ( ent->eType != ET_PLAYER ) {
				continue;
			}

			if ( ( ent->eFlags & EF_DEAD ) && ent->number == ent->clientNum ) {
				if ( !cgs.clientinfo[ent->clientNum].infoValid || cg.snap->ps.persistant[PERS_TEAM] != cgs.clientinfo[ent->clientNum].team ) {
					continue;
				}

				CG_DrawCompassIcon( basex, basey, basew, baseh, cg.snap->ps.origin, ent->pos.trBase, cgs.media.medicReviveShader );
			}
		}
	}
}
// -NERVE - SMF

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame) {

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg.cameraMode ) { //----(SA)	no 2d when in camera view
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	CG_ScreenFade();

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	// RtcwPro - Hud Names
	if (vp_drawnames.integer)
		VP_DrawNames();

	VP_ResetNames();
	
	CG_DrawFlashBlendBehindHUD();

#ifndef PRE_RELEASE_DEMO
	if ( cg_uselessNostalgia.integer ) {
		CG_DrawCrosshair();
		CG_Draw2D2();
		return;
	}
#endif

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();
			
		if (cg_drawNames.integer)
			CG_DrawOnScreenNames();
		else
			CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead
		if ( cg.snap->ps.stats[STAT_HEALTH] > 0 || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {

			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();

			CG_DrawDynamiteStatus();
			CG_DrawCrosshairNames();

			// DHM - Nerve :: Don't draw icon in upper-right when switching weapons
			//CG_DrawWeaponSelect();

			// RtcwPro - In warmup we print ready intel there..
			if (!cgs.gamestate == GS_WARMUP || cg_drawPickupItems.integer )
				CG_DrawPickupItem();
		}

		if ( cg_drawStatus.integer ) {
			if ( cg_fixedAspect.integer == 2 && cg.limboMenu ) {
				CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
			} else if ( cg_fixedAspect.integer == 2 && !cg.limboMenu ) {
				CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
			}

			Menu_PaintAll();
			CG_DrawTimedMenus();
		}
	}

	if ( cgs.gametype >= GT_TEAM ) {
		CG_DrawTeamInfo();
	}

	CG_DrawVote();

	CG_DrawLagometer();

	if ( !cg_paused.integer ) {
		CG_DrawUpperRight(stereoFrame);
	}

	// don't draw center string if scoreboard is up
	if ( !CG_DrawScoreboard() ) {
		CG_DrawCenterString();

		// RtcwPro - Pause print
		CG_PausePrint();

		// RtcwPro - Pop in print..
		CG_DrawPopinString();
		
		CG_DrawFollow();
		CG_DrawWarmup();

		//if ( cg_drawNotifyText.integer )
		CG_DrawNotify();

		// NERVE - SMF
		if ( cg_drawCompass.integer ) {
			CG_DrawCompass();
		}

		CG_DrawObjectiveInfo();
		CG_DrawObjectiveIcons();

		CG_DrawSpectatorMessage();

		CG_DrawLimboMessage();
		// -NERVE - SMF

		// RtcwPro
		if (cg_drawSpeed.integer)
		{
			CG_DrawTJSpeed();
		}
	}

	// RtcwPro - Announcer
	CG_DrawAnnouncer();
	// RtcwPro - window updates
	CG_windowDraw();
	// Ridah, draw flash blends now
	CG_DrawFlashBlend();
}

// NERVE - SMF
void CG_StartShakeCamera( float p ) {
	cg.cameraShakeScale = p;

	cg.cameraShakeLength = 1000 * ( p * p );
	cg.cameraShakeTime = cg.time + cg.cameraShakeLength;
	cg.cameraShakePhase = crandom() * M_PI; // start chain in random dir
}

void CG_ShakeCamera( void ) {
	float x, val;

	if ( cg.time > cg.cameraShakeTime ) {
		cg.cameraShakeScale = 0; // JPW NERVE all pending explosions resolved, so reset shakescale
		return;
	}

	// JPW NERVE starts at 1, approaches 0 over time
	x = ( cg.cameraShakeTime - cg.time ) / cg.cameraShakeLength;

	// up/down
	val = sin( M_PI * 8 * x + cg.cameraShakePhase ) * x * 18.0f * cg.cameraShakeScale;
	cg.refdefViewAngles[0] += val;

	// left/right
	val = sin( M_PI * 15 * x + cg.cameraShakePhase ) * x * 16.0f * cg.cameraShakeScale;
	cg.refdefViewAngles[1] += val;

	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
}
// -NERVE - SMF

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// if they are waiting at the mission stats screen, show the stats
	if ( cg_gameType.integer == GT_SINGLE_PLAYER ) {
		if ( strlen( cg_missionStats.string ) > 1 ) {
			trap_Cvar_Set( "com_expectedhunkusage", "-2" );
			CG_DrawInformation();
			return;
		}
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		 ( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	cg.refdef.glfog.registered = 0; // make sure it doesn't use fog from another scene

	// NERVE - SMF - activate limbo menu and draw small 3d window
	CG_ActivateLimboMenu();

	if ( cg.limboMenu ) {
		float x, y, w, h;
		x = LIMBO_3D_X;
		y = LIMBO_3D_Y;
		w = LIMBO_3D_W;
		h = LIMBO_3D_H;

		if ( cg_fixedAspect.integer ) {
			CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
		}

		cg.refdef.width = 0;
		CG_AdjustFrom640( &x, &y, &w, &h );

		cg.refdef.x = x;
		cg.refdef.y = y;
		cg.refdef.width = w;
		cg.refdef.height = h;
	}
	// -NERVE - SMF

	CG_ShakeCamera();       // NERVE - SMF

	// RTCWPro - draw triggers for shoutcasters
	if (cg_drawTriggers.integer && cgs.clientinfo[cg.clientNum].shoutStatus && !(cg.snap->ps.pm_flags & PMF_FOLLOW)) {

		CG_DrawTriggers();
	}

	trap_R_RenderScene( &cg.refdef );

	// draw status bar and other floating elements
	CG_Draw2D(stereoView);

	// RTCWPro
	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR && 
		cgs.clientinfo[cg.snap->ps.clientNum].shoutStatus &&
		!cg.showScores) 
	{
		CG_ShoutcasterItems();
	}
}

/*
=====================
RtcwPro - CG_Text_PaintChar_Ext

Ported from ET
=====================
*/
void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader) {
	w *= scalex;
	h *= scaley;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

/*
=====================
RtcwPro - CG_Text_Paint_ext2

Ported from S4NDMoD
=====================
*/
void CG_Text_Paint_ext2(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	//if (scale <= cg_smallFont.value) {
	//	font = &cgDC.Assets.smallFont;
	//} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	//}
	useScale = scale * font->glyphScale;

	color[3] *= cg_hudAlpha.value;	// (SA) adjust for cg_hudalpha

  if (text) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s];
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs,
														glyph->imageWidth,
														glyph->imageHeight,
														useScale,
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj,
													glyph->imageWidth,
													glyph->imageHeight,
													useScale,
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}

/*
=====================
RtcwPro - CG_Text_Width_ext2

Ported from S4NDMoD
=====================
*/
int CG_Text_Width_ext2(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;

	font = &cgDC.Assets.bigFont;

	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

/*
=====================
RtcwPro - CG_Text_Height_ext2

Ported from S4NDMoD
=====================
*/
int CG_Text_Height_ext2(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;

	font = &cgDC.Assets.bigFont;

	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s];
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

/*
=====================
RtcwPro - CG_Text_Paint_Ext

Ported from ET
=====================
*/

void CG_Text_Paint_Ext( float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t* font ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;

	scalex *= font->glyphScale;
	scaley *= font->glyphScale;

	if (text) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(unsigned char)*s];
			if ( Q_IsColorString( s ) ) {
				if( *(s+1) == COLOR_NULL ) {
					memcpy( newColor, color, sizeof(newColor) );
				} else {
					memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
					newColor[3] = color[3];
				}
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = scaley * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex) + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex), y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
				x += (glyph->xSkip * scalex) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}

/*
=====================
RtcwPro - CG_Text_Height_Ext

Ported from ET
=====================
*/
int CG_Text_Height_Ext( const char *text, float scale, int limit, fontInfo_t* font ) {
 int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;

	useScale = scale * font->glyphScale;
	max = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(unsigned char)*s];
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

/*
=====================
RtcwPro - CG_Text_Width_Ext

Ported from ET
=====================
*/

int CG_Text_Width_Ext( const char *text, float scale, int limit, fontInfo_t* font ) {
	int count, len;
	glyphInfo_t *glyph;
	const char *s = text;
	float out, useScale = scale * font->glyphScale;

	out = 0;
	if( text ) {
		len = strlen( text );
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(unsigned char)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

/*
=====================
RtcwPro - CG_DrawAnnouncer

Ported from ET
=====================
*/

void CG_DrawAnnouncer( void )
{
	int		x, y, w, h;
	float	scale, fade;
	vec4_t	color;

	if(  !cg_announcer.integer ||  cg.centerPrintAnnouncerTime <= cg.time )
		return;

	fade = (float)(cg.centerPrintAnnouncerTime - cg.time)/cg.centerPrintAnnouncerDuration;

	color[0] = cg.centerPrintAnnouncerColor[0];
	color[1] = cg.centerPrintAnnouncerColor[1];
	color[2] = cg.centerPrintAnnouncerColor[2];

	switch( cg.centerPrintAnnouncerMode ){
		default:
		case ANNOUNCER_NORMAL:
			color[3] = fade;
			break;
		case ANNOUNCER_SINE:
			color[3] = sin( M_PI * fade );
			break;
		case ANNOUNCER_INVERSE_SINE:
			color[3] = 1-sin( M_PI * fade );
			break;
		case ANNOUNCER_TAN:
			color[3] = tan( M_PI * fade );
	}

	scale = (1.1f - color[3]) * cg.centerPrintAnnouncerScale;

	h = CG_Text_Height_Ext( cg.centerPrintAnnouncer, scale, 0, &cgDC.Assets.textFont );

	y = (SCREEN_HEIGHT - h) / 2;

	w = CG_Text_Width_Ext(cg.centerPrintAnnouncer, scale, 0, &cgDC.Assets.textFont );

	x = ( SCREEN_WIDTH - w ) / 2;

	CG_Text_Paint_Ext( x, y, scale, scale , color, cg.centerPrintAnnouncer, 0, 0, 0, &cgDC.Assets.textFont );
}

void CG_AddAnnouncer(char *text, sfxHandle_t sound, float scale, int duration, float r, float g, float b, int mode)
{
	if ( !cg_announcer.integer )
		return;

	if ( sound )
		trap_S_StartLocalSound( sound, CHAN_ANNOUNCER );

	if ( text ){
		cg.centerPrintAnnouncer = text;
		cg.centerPrintAnnouncerTime = cg.time + duration;
		cg.centerPrintAnnouncerScale = scale;
		cg.centerPrintAnnouncerDuration = duration;
		cg.centerPrintAnnouncerColor[0] = r;
		cg.centerPrintAnnouncerColor[1] = g;
		cg.centerPrintAnnouncerColor[2] = b;
		cg.centerPrintAnnouncerMode = mode;
	}
}

/*
=============================
RTCWPro
CG_SCSortDistance
=============================
*/
int QDECL CG_SCSortDistance(const void* a, const void* b) {
	scItem_t* A = (scItem_t*)a;
	scItem_t* B = (scItem_t*)b;

	if (A->dist < B->dist) 
	{
		return 1;
	}
	else 
	{
		return -1;
	}
}

/*
=============================
RTCWPro
CG_ShoutcasterItems
=============================
*/
void CG_ShoutcasterItems() {
	int			i;
	centity_t* cent;

	memset(cg.scItems, 0, MAX_SCITEMS * sizeof(cg.scItems[0]));
	cg.numSCItems = 0;

	for (i = 0; i < MAX_ENTITIES; i++) 
	{
		cent = &cg_entities[i];

		if (!cent->currentValid)
			continue;

		switch (cent->currentState.eType) 
		{
		case ET_MISSILE:
			CG_ShoutcasterDynamite(i);
			break;
		default:
			break;
		}
	}

	// Sort
	qsort(cg.scItems, cg.numSCItems, sizeof(cg.scItems[0]), CG_SCSortDistance);

	// Draw
	for (i = 0; i < cg.numSCItems; i++) 
	{
		CG_Text_Paint_Ext(cg.scItems[i].position[0], cg.scItems[i].position[1], cg.scItems[i].scale, 
			cg.scItems[i].scale, cg.scItems[i].color, cg.scItems[i].text, 0, 0, ITEM_TEXTSTYLE_NORMAL, &cgDC.Assets.textFont);
	}
}
/*
=============================
RTCWPro
CG_Shoutcaster_Dynamite
=============================
*/
void CG_ShoutcasterDynamite(int num) {
	centity_t* cent;
	vec3_t		origin;
	vec_t		position[2];

	cent = &cg_entities[num];
	if (cent->currentState.eType != ET_MISSILE || cent->currentState.weapon != WP_DYNAMITE || cent->currentState.teamNum >= 4)
		return;

	if (!cent->currentValid)
		return;

	// Ent visible?
	if (PointVisible(cent->lerpOrigin))
		cent->lastSeenTime = cg.time;

	// Break if no action
	if (!cent->lastSeenTime || cg.time - cent->lastSeenTime >= 1000)
		return;

	// Ent position
	VectorCopy(cent->lerpOrigin, origin);

	// Add height, plus a little
	origin[2] += 20;

	if (!VisibleToScreen(origin, position)) 
	{
		return;
	}

	cg.scItems[cg.numSCItems].position[0] = position[0] / cgs.screenXScale;
	cg.scItems[cg.numSCItems].position[1] = position[1] / cgs.screenYScale;

	// Distance to player
	cg.scItems[cg.numSCItems].dist = VectorDistance(cg.predictedPlayerState.origin, origin);
	cg.scItems[cg.numSCItems].scale = 600 / cg.scItems[cg.numSCItems].dist * 0.2f;

	// Figure out color
	CG_ColorForPercent(100 * (30000 - cg.time + cent->currentState.effect1Time + 1000) / 30000, cg.scItems[cg.numSCItems].color);
	cg.scItems[cg.numSCItems].color[3] = 1 - ((float)(cg.time - cent->lastSeenTime) / 1000.f);

	// Center text
	cg.scItems[cg.numSCItems].text = va("%i", ((30000 - cg.time + cent->currentState.effect1Time) / 1000) + 1);
	cg.scItems[cg.numSCItems].position[0] -= CG_Text_Width_Ext(cg.scItems[cg.numSCItems].text, cg.scItems[cg.numSCItems].scale, 0, &cgDC.Assets.textFont) / 2;

	// Paint the text.
	//CG_Text_Paint_Ext( position[0], position[1], scale, scale, color, str, 0, 0, ITEM_TEXTSTYLE_NORMAL, &cgs.media.limboFont1 );

	// Increment number of items
	cg.numSCItems++;
}

