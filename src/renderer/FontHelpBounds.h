#pragma once

#ifdef FIX_BUGS
#include <vector>

// A measurement pass follows the real renderer, including glyph bearings and
// controller icons. Atlas readbacks live only for that pass, not for every frame.
class CHelpTextBounds
{
	struct AtlasImage {
		RwRaster *raster;
		RwImage *image;
	};
	std::vector<AtlasImage> images;

	RwImage *GetImage(RwRaster *raster)
	{
		for(const auto &entry : images)
			if(entry.raster == raster) return entry.image;
		RwImage *image = nil;
		if(raster) {
#ifdef LIBRW
			image = raster->toImage();
			if(image) image->convertTo32();
#else
			image = RwImageCreate(RwRasterGetWidth(raster), RwRasterGetHeight(raster), 32);
			if(image) {
				RwImageAllocatePixels(image);
				if(!RwImageSetFromRaster(image, raster)) {
					RwImageDestroy(image);
					image = nil;
				}
			}
#endif
		}
		images.push_back({raster, image});
		return image;
	}

public:
	bool active = false;
	bool valid = false;
	uint32 generation = 0;
	CRect bounds;

	void Begin()
	{
		active = true;
		valid = false;
	}
	void End()
	{
		active = false;
		for(const auto &entry : images)
			if(entry.image) RwImageDestroy(entry.image);
		images.clear();
	}
	void Reset()
	{
		End();
		generation++;
	}

	void Capture(RwRaster *raster, const CRect &rect, float u0, float v0, float u1, float v1, float shear = 0.0f)
	{
		CRect ink = rect;
		RwImage *image = GetImage(raster);
		if(image && RwImageGetPixels(image) && RwImageGetDepth(image) == 32 && u1 > u0 && v1 > v0) {
			const int width = RwImageGetWidth(image), height = RwImageGetHeight(image);
			const int stride = RwImageGetStride(image);
			const uint8 *pixels = RwImageGetPixels(image);
			const int x0 = Max(0, (int)floorf(u0 * width));
			const int y0 = Max(0, (int)floorf(v0 * height));
			const int x1 = Min(width, (int)ceilf(u1 * width));
			const int y1 = Min(height, (int)ceilf(v1 * height));
			int left = x1, right = x0, top = y1, bottom = y0;
			for(int y = y0; y < y1; y++)
				for(int x = x0; x < x1; x++)
					if(pixels[y * stride + x * 4 + 3]) {
						left = Min(left, x);
						right = Max(right, x + 1);
						top = Min(top, y);
						bottom = Max(bottom, y + 1);
					}
			if(right <= left || bottom <= top) return;
			const float scaleX = (rect.right - rect.left) / ((u1 - u0) * width);
			const float scaleY = (rect.bottom - rect.top) / ((v1 - v0) * height);
			ink.left = Max(rect.left, rect.left + (left - u0 * width) * scaleX);
			ink.right = Min(rect.right, rect.left + (right - u0 * width) * scaleX);
			ink.top = Max(rect.top, rect.top + (top - v0 * height) * scaleY);
			ink.bottom = Min(rect.bottom, rect.top + (bottom - v0 * height) * scaleY);
		}
		if(shear != 0.0f && rect.bottom > rect.top) {
			const float shiftTop = shear * (ink.top - rect.top) / (rect.bottom - rect.top);
			const float shiftBottom = shear * (ink.bottom - rect.top) / (rect.bottom - rect.top);
			ink.left -= Max(shiftTop, shiftBottom);
			ink.right -= Min(shiftTop, shiftBottom);
		}
		if(!valid) {
			bounds = ink;
			valid = true;
		} else {
			bounds.left = Min(bounds.left, ink.left);
			bounds.right = Max(bounds.right, ink.right);
			bounds.top = Min(bounds.top, ink.top);
			bounds.bottom = Max(bounds.bottom, ink.bottom);
		}
	}
};

static CHelpTextBounds gHelpTextBounds;
#endif
