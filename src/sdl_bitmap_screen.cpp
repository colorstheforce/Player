/////////////////////////////////////////////////////////////////////////////
// This file is part of EasyRPG Player.
//
// EasyRPG Player is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// EasyRPG Player is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "system.h"
#include "sdl_bitmap_screen.h"
#include "sdl_bitmap.h"
#include "sdl_ui.h"
#include "util_macro.h"

////////////////////////////////////////////////////////////
#ifdef USE_ALPHA
	#define SETALPHA_FLAGS SDL_SRCALPHA
#else
	#define SETALPHA_FLAGS SDL_SRCALPHA | SDL_RLEACCEL
#endif

////////////////////////////////////////////////////////////
SdlBitmapScreen::SdlBitmapScreen(Bitmap* bitmap) :
	bitmap(bitmap),
	bitmap_effects(NULL),
	delete_bitmap(bitmap != NULL) {

	ClearEffects();

	if (delete_bitmap) {
		src_rect_effect = bitmap->GetRect();

		bitmap->AttachBitmapScreen(this);
	}
}

////////////////////////////////////////////////////////////
SdlBitmapScreen::SdlBitmapScreen(bool delete_bitmap) :
	bitmap(NULL),
	bitmap_effects(NULL),
	delete_bitmap(delete_bitmap) {

	ClearEffects();
}

////////////////////////////////////////////////////////////
SdlBitmapScreen::~SdlBitmapScreen() {
	if (bitmap_effects != NULL && bitmap_effects != bitmap) {
		delete bitmap_effects;
	} else if (delete_bitmap && bitmap != NULL) {
		delete bitmap;
	} else if (bitmap != NULL) {
		bitmap->DetachBitmapScreen(this);
	}
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::SetDirty() {
	needs_refresh = true;
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::SetBitmap(Bitmap* source) {
	if (bitmap_effects != NULL && bitmap_effects != bitmap) {
		delete bitmap_effects;
		bitmap_effects = NULL;
	}

	if (delete_bitmap && bitmap != NULL)
		delete bitmap;
	else if (bitmap != NULL)
		bitmap->DetachBitmapScreen(this);

	bitmap = source;
	needs_refresh = true;

	bitmap->AttachBitmapScreen(this);
}

////////////////////////////////////////////////////////////
Bitmap* SdlBitmapScreen::GetBitmap() {
	return bitmap;
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::BlitScreen(int x, int y) {
	if (bitmap == NULL || opacity_effect <= 0) return;

	Refresh();

	if (bitmap_effects == NULL) return;

	SDL_Surface* surface = ((SdlBitmap*)bitmap_effects)->bitmap;

	if (bush_effect < surface->h) {
		Rect src_rect(0, 0, surface->w, surface->h - bush_effect);

		BlitScreenIntern(surface, x, y, src_rect, opacity_effect);
	}

	if (bush_effect > 0) {
		Rect src_rect(0, 0, surface->w, bush_effect);

		BlitScreenIntern(surface, x, y + bush_effect, src_rect, opacity_effect);
	}
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::BlitScreen(int x, int y, Rect src_rect) {
	if (bitmap == NULL || opacity_effect <= 0) return;

	Refresh();

	if (bitmap_effects == NULL) return;

	SDL_Surface* surface = ((SdlBitmap*)bitmap_effects)->bitmap;

	if (bush_effect < surface->h) {
		Rect blit_rect = src_rect;
		blit_rect.y	-= bush_effect;

		BlitScreenIntern(surface, x, y, blit_rect, opacity_effect);
	}

	if (bush_effect > 0) {
		Rect blit_rect = src_rect;
		blit_rect.y	= bush_effect;

		BlitScreenIntern(surface, x, y + bush_effect, blit_rect, opacity_effect);
	}
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::BlitScreenTiled(Rect src_rect, Rect dst_rect) {
	if (bitmap == NULL || opacity_effect <= 0) return;

	Refresh();

	if (bitmap_effects == NULL) return;

	SDL_Surface* surface = ((SdlBitmap*)bitmap_effects)->bitmap;

	int y_blits = 1;
	if (src_rect.height < surface->h && src_rect.height != 0) {
		y_blits = (int)ceil((float)surface->h / (float)src_rect.height);
	}

	int x_blits = 1;
	if (src_rect.width < surface->w && src_rect.width != 0) {
		x_blits = (int)ceil((float)surface->w / (float)src_rect.width);
	}

	for (int j = 0; j < y_blits; j++) {
		for (int i = 0; i < x_blits; i++) {
			BlitScreenIntern(surface, i * src_rect.width, j * src_rect.height, src_rect, opacity_effect);
		}
	}
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::BlitScreenIntern(SDL_Surface* surface, int x, int y, Rect src_rect, int opacity) {
	#ifdef USE_ALPHA
		if (opacity == 255) {
			SDL_Rect src_r = {src_rect.x, src_rect.y, src_rect.width, src_rect.height};
			SDL_Rect dst_r = {x, y, 0, 0};

			SDL_BlitSurface(surface, &src_r, DisplaySdlUi->GetDisplaySurface(), &dst_r);
		} else {
			src_rect.Adjust(surface->w, surface->h);
			if (src_rect.IsOutOfBounds(surface->w, surface->h) )return;

			SDL_Surface* display = DisplaySdlUi->GetDisplaySurface();

			int bpp = display->format->BytesPerPixel;

			if SDL_MUSTLOCK(display) SDL_LockSurface(display);
			if SDL_MUSTLOCK(surface) SDL_LockSurface(surface);

			if (bpp == 2) {
				const uint16* src_pixel = ((uint16*)surface->pixels) + src_rect.x + src_rect.y * surface->pitch / bpp;
				uint16* dst_pixel = ((uint16*)DisplaySdlUi->GetDisplaySurface()->pixels) + x + y * display->pitch / bpp;

				/*for (int i = 0; i < src_rect.height; i++) {
					for (int j = 0; j < src_rect.width; j++) {
						uint8 srca = src_pixel[3] * opacity / 255;
						dst_pixel[0] = (dst_pixel[0] * (255 - srca) + src_pixel[0] * srca) / 255;
						dst_pixel[1] = (dst_pixel[1] * (255 - srca) + src_pixel[1] * srca) / 255;
						dst_pixel[2] = (dst_pixel[2] * (255 - srca) + src_pixel[2] * srca) / 255;

						src_pixel += 1;
						dst_pixel += 1;
					}
					src_pixel += surface->pitch / bpp - src_rect.width;
					dst_pixel += display->pitch / bpp - src_rect.width;
				}*/
			} else if (bpp == 4) {
				const uint8* src_pixel = (uint8*)surface->pixels + src_rect.x * bpp + src_rect.y * surface->pitch;
				uint8* dst_pixel = (uint8*)display->pixels + x * bpp + y * display->pitch;

				for (int i = 0; i < src_rect.height; i++) {
					for (int j = 0; j < src_rect.width; j++) {
						uint8 srca = src_pixel[3] * opacity / 255;
						dst_pixel[0] = (dst_pixel[0] * (255 - srca) + src_pixel[0] * srca) / 255;
						dst_pixel[1] = (dst_pixel[1] * (255 - srca) + src_pixel[1] * srca) / 255;
						dst_pixel[2] = (dst_pixel[2] * (255 - srca) + src_pixel[2] * srca) / 255;

						src_pixel += bpp;
						dst_pixel += bpp;
					}
					src_pixel += surface->pitch - src_rect.width * bpp;
					dst_pixel += display->pitch - src_rect.width * bpp;
				}
			}

			if SDL_MUSTLOCK(display) SDL_UnlockSurface(display);
			if SDL_MUSTLOCK(surface) SDL_UnlockSurface(surface);
		}
	#else
		SDL_Rect src_r = {(int16)src_rect.x, (int16)src_rect.y, (uint16)src_rect.width, (uint16)src_rect.height};
		SDL_Rect dst_r = {(int16)x, (int16)y, 0, 0};

		SDL_SetAlpha(surface, SETALPHA_FLAGS, (uint8)opacity);

		SDL_BlitSurface(surface, &src_r, DisplaySdlUi->GetDisplaySurface(), &dst_r);

		SDL_SetAlpha(surface, SETALPHA_FLAGS, 255);
	#endif
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::Refresh() {
	if (!needs_refresh) return;

	needs_refresh = false;

	if (delete_bitmap) {
		bitmap_effects = bitmap;
		return;
	}

	if (bitmap_effects != NULL && bitmap_effects != bitmap) {
		delete bitmap_effects;
		bitmap_effects = NULL;
	}

	src_rect_effect.Adjust(bitmap->GetWidth(), bitmap->GetHeight());

	if (bitmap == NULL || src_rect_effect.IsOutOfBounds(bitmap->GetWidth(), bitmap->GetHeight())) {
		return;
	}

	if (src_rect_effect == Rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight()) &&
		tone_effect == Tone() && angle_effect == 0.0 &&
		flipx_effect == false && flipy_effect == false &&
		zoom_x_effect == 1.0 && zoom_y_effect == 1.0) {

		bitmap_effects = bitmap;

	} else {
		int new_width = src_rect_effect.width;
		int new_height = src_rect_effect.height;

		CalcZoomedSize(new_width, new_height);

		int zoomed_width = new_width;
		int zoomed_height = new_height;

		CalcRotatedSize(new_width, new_height);

		if (new_width > 0 && new_height > 0) {
			bitmap_effects = Bitmap::CreateBitmap(bitmap, src_rect_effect, bitmap->GetTransparent());

			bitmap_effects->ToneChange(tone_effect);
			bitmap_effects->Flip(flipx_effect, flipy_effect);

			if (zoom_x_effect != 1.0 || zoom_y_effect != 1.0) {
				Bitmap* temp = bitmap_effects->Resample(zoomed_width, zoomed_height, bitmap_effects->GetRect());
				delete bitmap_effects;
				bitmap_effects = temp;
			}

			// TODO: Rotate
		} else {
			bitmap_effects = NULL;
		}
	}
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::CalcRotatedSize(int &width, int &height) {
	if (angle_effect != 0.0) {
		double radians=(2 * 3.1416 * angle_effect) / 360;

		double cosine = (double)cos(radians);
		double sine = (double)sin(radians);

		double p1x = -height * sine;
		double p1y = height * cosine;
		double p2x = width * cosine - height * sine;
		double p2y = height * cosine + width * sine;
		double p3x = width * cosine;
		double p3y = width * sine;

		double minx = min(0.0, min(p1x, min(p2x, p3x)));
		double miny = min(0.0, min(p1y, min(p2y, p3y)));
		double maxx = max(p1x, max(p2x, p3x));
		double maxy = max(p1y, max(p2y, p3y));

		width = (int)ceil(fabs(maxx) - minx);
		height = (int)ceil(fabs(maxy) - miny);
	}
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::CalcZoomedSize(int &width, int &height) {
	width = (int)(width * zoom_x_effect);
	height = (int)(height * zoom_y_effect);
}

////////////////////////////////////////////////////////////
void SdlBitmapScreen::ClearEffects() {
	needs_refresh = true;

	opacity_effect = 255;
	bush_effect = 0;
	tone_effect = Tone();
	src_rect_effect = Rect(0, 0, 0, 0);
	flipx_effect = false;
	flipy_effect = false;
	zoom_x_effect = 1.0;
	zoom_y_effect = 1.0;
	angle_effect = 0.0;
}

void SdlBitmapScreen::SetFlashEffect(const Color &color, int duration) {
	// TODO
}

void SdlBitmapScreen::UpdateFlashEffect(int frame) {
	// TODO
}

void SdlBitmapScreen::SetSrcRect(Rect src_rect) {
	if (src_rect_effect != src_rect) {
		src_rect_effect = src_rect;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetOpacityEffect(int opacity) {
	if (opacity_effect != opacity) {
		opacity_effect = opacity;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetBushDepthEffect(int bush_depth) {
	if (bush_effect != bush_depth) {
		bush_effect = bush_depth;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetToneEffect(Tone tone) {
	if (tone_effect != tone) {
		tone_effect = tone;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetFlipXEffect(bool flipx) {
	if (flipx_effect != flipx) {
		flipx_effect = flipx;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetFlipYEffect(bool flipy) {
	if (flipy_effect != flipy) {
		flipy_effect = flipy;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetZoomXEffect(double zoom_x) {
	if (zoom_x_effect != zoom_x) {
		zoom_x_effect = zoom_x;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetZoomYEffect(double zoom_y) {
	if (zoom_y_effect != zoom_y) {
		zoom_y_effect = zoom_y;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetAngleEffect(double angle) {
	if (angle_effect != angle) {
		angle_effect = angle;
		needs_refresh = true;
	}
}

void SdlBitmapScreen::SetBlendType(int blend_type) {
	blend_type_effect = blend_type;
}

void SdlBitmapScreen::SetBlendColor(Color blend_color) {
	blend_color_effect = blend_color;
}

Rect SdlBitmapScreen::GetSrcRect() const {
	return src_rect_effect;
}

int SdlBitmapScreen::GetOpacityEffect() const {
	return opacity_effect;
}

int SdlBitmapScreen::GetBushDepthEffect() const {
	return bush_effect;
}

Tone SdlBitmapScreen::GetToneEffect() const {
	return tone_effect;
}

bool SdlBitmapScreen::GetFlipXEffect() const {
	return flipx_effect;
}

bool SdlBitmapScreen::GetFlipYEffect() const {
	return flipy_effect;
}

double SdlBitmapScreen::GetZoomXEffect() const {
	return zoom_x_effect;
}

double SdlBitmapScreen::GetZoomYEffect() const {
	return zoom_y_effect;
}

double SdlBitmapScreen::GetAngleEffect() const {
	return angle_effect;
}

int SdlBitmapScreen::GetBlendType() const {
	return blend_type_effect;
}

Color SdlBitmapScreen::GetBlendColor() const {
	return blend_color_effect;
}