#include "chip8.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <SDL2/SDL.h>

#define APP_NAME              "Chip 8"
#define DEFAULT_SCREEN_WIDTH  1024
#define DEFAULT_SCREEN_HEIGHT 512

#define FPS  50
#define MSPS (1000/FPS)

struct state {
	struct chip8 chip8;

	SDL_Window   *window;
	SDL_Renderer *renderer;
	SDL_Texture  *texture;

	char *romfile;

	int width, height, fullscreen;
	int quit;
};

static void
usage ( const char *progname ) {
	fprintf( stdout, "usage: %s ROM\n", progname );
	exit(0);
}

static void
parse_args ( struct state *state, int argc, char *argv[] ) {
	memset( state, 0, sizeof(struct state) );

	state->width = DEFAULT_SCREEN_WIDTH;
	state->height = DEFAULT_SCREEN_HEIGHT;

	for ( int i = 1; i < argc; i++ ) {
		if ( strcmp( "-W", argv[i] ) == 0 ) {
			state->width = atoi(argv[++i]);
		} else if ( strcmp( "-H", argv[i] ) == 0 ) {
			state->height = atoi(argv[++i]);
		} else if ( strcmp( "-f", argv[i] ) == 0 ) {
			state->fullscreen = 1;
		} else if ( strcmp( "-h", argv[i] ) == 0 ) {
			usage(argv[0]);
		} else {
			state->romfile = argv[i];	
		}
	}

	if ( state->romfile == 0 ) { usage(argv[0]); }
}

static int
init ( struct state *state ) {
	srand(time(NULL));

	chip8_init( &state->chip8 );
	if ( chip8_load( &state->chip8, state->romfile ) == 0 ) {
		fprintf( stderr, "unable to load rom \"%s\"\n", state->romfile );
		return 0;
	}

	if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 ) {
		fprintf( stderr, "SDL_Init : %s\n", SDL_GetError() );
		return 0;
	}

	int status = SDL_CreateWindowAndRenderer( 
		state->width, 
		state->height,
		state->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0,
		&state->window,
		&state->renderer
	);

	if ( status < 0 ) {
		fprintf( stderr, "SDL_CreateWindowAndRenderer : %s\n", SDL_GetError() );
		return 0;
	}

	/* nearest interpolation method looks best when scaling texture */
	SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );

	/* set the renderer's logical size to actual size of the Chip-8 display */
	/* SDL will figure out how scale it up to the current resolution */
	SDL_RenderSetLogicalSize( 
		state->renderer, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT
	);

	state->texture = SDL_CreateTexture(
		state->renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING,
		CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT
	);

	return 1;
}

static void
update ( struct state *state, Sint32 dt ) {
	static Sint32 elapsed = 0;
	elapsed += dt;
	
	/* fiddle with the numbers in this loop to affect processor speed */	
	while ( elapsed >= 15 ) {
		chip8_step( &state->chip8 );
		if ( state->chip8.beep ) { 
			fprintf( stdout, "\a" ); 
			state->chip8.beep = 0;
		}
		elapsed -= 4;
	}
}

static void
update_keyboard ( struct state *s, int key, int dir ) {
	/* change chip-8 keyboard mappings here */
	switch (key) {		
		case SDLK_1: chip8_key_set_state( &s->chip8, CHIP8_KEY_1, dir ); break;		
		case SDLK_2: chip8_key_set_state( &s->chip8, CHIP8_KEY_2, dir ); break;
		case SDLK_3: chip8_key_set_state( &s->chip8, CHIP8_KEY_3, dir ); break;		
		case SDLK_4: chip8_key_set_state( &s->chip8, CHIP8_KEY_C, dir ); break;
		case SDLK_q: chip8_key_set_state( &s->chip8, CHIP8_KEY_4, dir ); break;		
		case SDLK_w: chip8_key_set_state( &s->chip8, CHIP8_KEY_5, dir ); break;
		case SDLK_e: chip8_key_set_state( &s->chip8, CHIP8_KEY_6, dir ); break;		
		case SDLK_r: chip8_key_set_state( &s->chip8, CHIP8_KEY_D, dir ); break;
		case SDLK_a: chip8_key_set_state( &s->chip8, CHIP8_KEY_7, dir ); break;
		case SDLK_s: chip8_key_set_state( &s->chip8, CHIP8_KEY_8, dir ); break;
		case SDLK_d: chip8_key_set_state( &s->chip8, CHIP8_KEY_9, dir ); break;
		case SDLK_f: chip8_key_set_state( &s->chip8, CHIP8_KEY_E, dir ); break;
		case SDLK_z: chip8_key_set_state( &s->chip8, CHIP8_KEY_A, dir ); break;
		case SDLK_x: chip8_key_set_state( &s->chip8, CHIP8_KEY_0, dir ); break;
		case SDLK_c: chip8_key_set_state( &s->chip8, CHIP8_KEY_B, dir ); break;
		case SDLK_v: chip8_key_set_state( &s->chip8, CHIP8_KEY_F, dir ); break;
		default: break;
	}
}

static void
handle_events ( struct state *state ) {
	SDL_Event e;

	while ( SDL_PollEvent(&e) ) {
		switch ( e.type ) {
			case SDL_QUIT:
				state->quit = 1;
				break;
			case SDL_KEYDOWN:
				if ( e.key.keysym.sym == SDLK_ESCAPE ) { state->quit = 1; }
				update_keyboard( state, e.key.keysym.sym, CHIP8_KEY_DOWN );
				break;
			case SDL_KEYUP:
				update_keyboard( state, e.key.keysym.sym, CHIP8_KEY_UP );
				break;
		}
	}
}

static void
render ( struct state *state ) {
	Uint8 *pixels; /* PIXELFORMAT in init is RGB332 so Uint8 should be OK? */
	int pitch;     /* should really use this to error check, just in case */

	/* lock the texture and retrieve the pixel buffer */
	SDL_LockTexture( state->texture, NULL, (void **) &pixels, &pitch );

	/* draw the chip-8 display buffer to the texture */
	for ( int i = 0, pi = 0; i < CHIP8_DISPLAY_BUF_SIZE; i++, pi += 8 ) {
		pixels[pi + 7] = (state->chip8.display[i] & 0x01)? 255 : 0;
		pixels[pi + 6] = (state->chip8.display[i] & 0x02)? 255 : 0;
		pixels[pi + 5] = (state->chip8.display[i] & 0x04)? 255 : 0;
		pixels[pi + 4] = (state->chip8.display[i] & 0x08)? 255 : 0;
		pixels[pi + 3] = (state->chip8.display[i] & 0x10)? 255 : 0;
		pixels[pi + 2] = (state->chip8.display[i] & 0x20)? 255 : 0;
		pixels[pi + 1] = (state->chip8.display[i] & 0x40)? 255 : 0;
		pixels[pi + 0] = (state->chip8.display[i] & 0x80)? 255 : 0;		
	}
	
	/* unlock the texture and draw to the screen */
	SDL_UnlockTexture( state->texture );
	SDL_RenderClear( state->renderer );
	SDL_RenderCopy( state->renderer, state->texture, NULL, NULL );
	SDL_RenderPresent( state->renderer );
}

static void
gameloop ( struct state *state ) {
	Uint32 start = SDL_GetTicks();
	while ( !state->quit ) {
		Uint32 old = start;
		start = SDL_GetTicks();		
		update( state, start-old );
		handle_events( state );				
		render( state );
		Sint32 diff = MSPS - SDL_GetTicks() + start;
		SDL_Delay( (diff > 0)? diff : 0 );
	}
}

static void
quit ( struct state *state ) {
	if ( state->texture )  { SDL_DestroyTexture( state->texture ); }
	if ( state->renderer ) { SDL_DestroyRenderer( state->renderer ); }
	if ( state->window )   { SDL_DestroyWindow( state->window ); }
	SDL_Quit();
}

int
main ( int argc, char *argv[] ) {
	struct state state;
	
	parse_args( &state, argc, argv );

	if ( init( &state ) == 0 ) { 
		quit( &state );
		return EXIT_FAILURE; 
	}
	
	gameloop( &state );
	
	quit( &state );

    return EXIT_SUCCESS;
}
